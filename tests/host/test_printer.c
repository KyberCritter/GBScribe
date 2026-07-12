// M4 host harness: build the real printer byte stream for a document page,
// decode it with mofosyne's independent decoder, and verify the BMP pixels
// match a reference render of the same page. Proves packet framing, checksums,
// payload order, and tile packing without hardware.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "core/document.h"
#include "core/layout.h"
#include "printer/gbp_packet.h"
#include "printer/gbp_print.h"

extern const unsigned char font2bpp[128 * 16];

static int fails, checks;
#define CHECK(cond, ...) do { \
    checks++; \
    if (!(cond)) { fails++; printf("FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } \
} while (0)

// ---- unit checks -----------------------------------------------------------

static void test_packet_framing(void)
{
    uint8_t pkt[GBP_PACKET_MAX];

    // INIT: 88 33 01 00 00 00 [ck=0x0001] 00 00
    int n = gbp_packet_build(GBP_CMD_INIT, NULL, 0, pkt);
    static const uint8_t init_ref[] = {0x88,0x33,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00};
    CHECK(n == 10 && !memcmp(pkt, init_ref, 10), "INIT packet bytes");

    // INQUIRY: checksum = 0x0F
    n = gbp_packet_build(GBP_CMD_INQUIRY, NULL, 0, pkt);
    static const uint8_t inq_ref[] = {0x88,0x33,0x0F,0x00,0x00,0x00,0x0F,0x00,0x00,0x00};
    CHECK(n == 10 && !memcmp(pkt, inq_ref, 10), "INQUIRY packet bytes");

    // PRINT with reference args (margins 0x13, exposure 0x40):
    // ck = 02+00+04+00 + 01+13+E4+40 = 0x13E
    uint8_t args[4];
    gbp_print_args(0x13, 0x40, args);
    n = gbp_packet_build(GBP_CMD_PRINT, args, 4, pkt);
    CHECK(n == 14, "PRINT length");
    CHECK(pkt[4] == 0x04 && pkt[5] == 0x00, "PRINT payload len field");
    CHECK(pkt[10] == 0x3E && pkt[11] == 0x01, "PRINT checksum (got %02X %02X)", pkt[10], pkt[11]);

    // DATA packet checksum: 640 x 0xFF payload
    uint8_t ff[GBP_MAX_PAYLOAD];
    memset(ff, 0xFF, sizeof ff);
    n = gbp_packet_build(GBP_CMD_DATA, ff, GBP_MAX_PAYLOAD, pkt);
    // ck = 04 + 80 + 02 + 640*0xFF = 0x86 + 0x27D80 = 0x27E06 -> 0x7E06
    CHECK(n == 6 + 640 + 4, "DATA length");
    CHECK(pkt[6 + 640] == 0x06 && pkt[7 + 640] == 0x7E,
          "DATA checksum (got %02X %02X)", pkt[6+640], pkt[7+640]);
}

// ---- end-to-end decode harness ---------------------------------------------

static Layout lo;
static uint8_t page[GBP_PAGE_BYTES];

static void emit_hex(FILE *f, const uint8_t *b, int n)
{
    for (int i = 0; i < n; i++)
        fprintf(f, "%02X ", b[i]);
    fprintf(f, "\n");
}

// reference pixel render: GB 2bpp tile page -> 160x144 color indices
static void reference_render(const uint8_t *pg, uint8_t *px /*160*144*/)
{
    for (int row = 0; row < GBP_ROWS_PER_PAGE; row++)
        for (int t = 0; t < GBP_TILES_PER_ROW; t++) {
            const uint8_t *tile = pg + (row * GBP_TILES_PER_ROW + t) * 16;
            for (int y = 0; y < 8; y++) {
                uint8_t p0 = tile[y * 2], p1 = tile[y * 2 + 1];
                for (int x = 0; x < 8; x++) {
                    int bit = 7 - x;
                    int c = ((p0 >> bit) & 1) | (((p1 >> bit) & 1) << 1);
                    px[(row * 8 + y) * 160 + t * 8 + x] = (uint8_t)c;
                }
            }
        }
}

// minimal BMP reader (24bpp bottom-up, as written by the decoder)
static uint8_t *read_bmp(const char *path, int *w, int *h)
{
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    uint8_t hdr[54];
    if (fread(hdr, 1, 54, f) != 54) { fclose(f); return NULL; }
    uint32_t off = hdr[10] | hdr[11] << 8 | hdr[12] << 16 | (uint32_t)hdr[13] << 24;
    int32_t bw = hdr[18] | hdr[19] << 8 | hdr[20] << 16 | hdr[21] << 24;
    int32_t bh = hdr[22] | hdr[23] << 8 | hdr[24] << 16 | hdr[25] << 24;
    uint16_t bpp = hdr[28] | hdr[29] << 8;
    if (bpp != 24) { fclose(f); return NULL; }
    int down = bh < 0;                 // negative height = top-down
    if (down) bh = -bh;
    int stride = (bw * 3 + 3) & ~3;
    uint8_t *rows = malloc((size_t)stride * bh);
    fseek(f, (long)off, SEEK_SET);
    if (fread(rows, 1, (size_t)stride * bh, f) != (size_t)stride * bh) {
        free(rows); fclose(f); return NULL;
    }
    fclose(f);
    uint8_t *out = malloc((size_t)bw * bh * 3);   // top-down RGB
    for (int y = 0; y < bh; y++) {
        const uint8_t *src = rows + (size_t)stride * (down ? y : bh - 1 - y);
        for (int x = 0; x < bw; x++) {
            out[(y * bw + x) * 3 + 0] = src[x * 3 + 2]; // R (BMP stores BGR)
            out[(y * bw + x) * 3 + 1] = src[x * 3 + 1];
            out[(y * bw + x) * 3 + 2] = src[x * 3 + 0];
        }
    }
    free(rows);
    *w = bw; *h = bh;
    return out;
}

// palette passed to the decoder: index -> unique gray
static const uint8_t gray[4] = {0xFF, 0xAA, 0x55, 0x00};

static void test_decode_roundtrip(void)
{
    static const char sample[] =
        "GBScribe test page.\n"
        "The quick brown fox jumps over the lazy dog 0123456789\n"
        "\n"
        "wrap check: supercalifragilisticexpialidocious\n"
        "symbols !@#$%^&*()_+-=[]{};:'\",.<>/?~`|\\\n"
        "\x05\x06\x07\x08\x09\x0B\x0C\x0D \x01\x02\x03\x04\n"
        "\x10\x0E\x0E\x16\x0E\x0E\x11\n"
        "\x0F  \x0F  \x0F\n"
        "\x12\x0E\x0E\x18\x0E\x0E\x13";
    int slen = (int)strlen(sample);

    layout_reflow(&lo, sample, slen, GBP_TILES_PER_ROW, WRAP_POST_WORD);
    gbp_compose_text_page(sample, slen, &lo, 0, page);

    // build the full print job byte stream as hex
    FILE *f = fopen("bin/stream.txt", "w");
    CHECK(f != NULL, "open stream.txt");
    if (!f) return;

    uint8_t pkt[GBP_PACKET_MAX];
    int n = gbp_packet_build(GBP_CMD_INIT, NULL, 0, pkt);
    emit_hex(f, pkt, n);
    for (int i = 0; i < GBP_PKTS_PER_PAGE; i++) {
        n = gbp_packet_build(GBP_CMD_DATA, page + i * GBP_DATA_PER_PKT,
                             GBP_DATA_PER_PKT, pkt);
        emit_hex(f, pkt, n);
    }
    n = gbp_packet_build(GBP_CMD_DATA, NULL, 0, pkt);   // empty DATA arms PRINT
    emit_hex(f, pkt, n);
    uint8_t args[4];
    gbp_print_args(0x13, 0x40, args);
    n = gbp_packet_build(GBP_CMD_PRINT, args, 4, pkt);
    emit_hex(f, pkt, n);
    fclose(f);

    // decode with the independent third-party decoder
    int rc = system("./decoder/gpbdecoder -i bin/stream.txt "
                    "-p \"#ffffff#aaaaaa#555555#000000\" -o bin/page.bmp > /dev/null");
    CHECK(rc == 0, "gpbdecoder run (rc=%d)", rc);

    int w = 0, h = 0;
    uint8_t *rgb = read_bmp("bin/page0.bmp", &w, &h);
    CHECK(rgb != NULL, "decoded BMP readable");
    if (!rgb) return;
    CHECK(w == 160 && h == 144, "BMP is 160x144 (got %dx%d)", w, h);

    static uint8_t ref[160 * 144];
    reference_render(page, ref);

    int bad = 0;
    if (w == 160 && h == 144) {
        for (int i = 0; i < 160 * 144; i++) {
            uint8_t g = gray[ref[i]];
            if (rgb[i*3] != g || rgb[i*3+1] != g || rgb[i*3+2] != g) {
                if (bad < 3)
                    printf("  pixel %d,%d: ref idx %d, bmp %02X%02X%02X\n",
                           i % 160, i / 160, ref[i], rgb[i*3], rgb[i*3+1], rgb[i*3+2]);
                bad++;
            }
        }
    }
    CHECK(bad == 0, "decoded pixels match reference (%d mismatches)", bad);
    free(rgb);
    printf("  (page BMP written to tests/host/bin/ for visual inspection)\n");
}

int main(void)
{
    test_packet_framing();
    test_decode_roundtrip();
    printf("%d checks, %d failures\n", checks, fails);
    return fails ? 1 : 0;
}
