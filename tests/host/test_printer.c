// Host harness for GBScribe's printer path, now driven by AdvancePrint.
// Verifies exact packet framing/checksums (hand-computed references), the wrap
// engine, page composition with GBScribe's own font, and that GBScribe's print
// font matches the library default. No hardware or GBA headers needed.
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "advanceprint/advanceprint_packet.h"
#include "advanceprint/advanceprint_text.h"
#include "advanceprint/advanceprint_page.h"
#include "advanceprint/advanceprint_font.h"

// GBScribe's generated print font (tools/mkfont.py), installed at startup.
extern const unsigned char font2bpp[2048];

static int fails, checks;
#define CHECK(cond, ...) do { \
    checks++; \
    if (!(cond)) { fails++; printf("FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } \
} while (0)

static void test_packet_framing(void)
{
    uint8_t pkt[AP_PACKET_MAX];

    // INIT: 88 33 01 00 00 00 [ck=0x0001] 00 00
    int n = ap_packet_build(AP_CMD_INIT, NULL, 0, pkt);
    static const uint8_t init_ref[] = {0x88,0x33,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00};
    CHECK(n == 10 && !memcmp(pkt, init_ref, 10), "INIT packet bytes");

    // INQUIRY: checksum = 0x0F
    n = ap_packet_build(AP_CMD_INQUIRY, NULL, 0, pkt);
    static const uint8_t inq_ref[] = {0x88,0x33,0x0F,0x00,0x00,0x00,0x0F,0x00,0x00,0x00};
    CHECK(n == 10 && !memcmp(pkt, inq_ref, 10), "INQUIRY packet bytes");

    // PRINT with reference args (margins 0x13, exposure 0x40):
    // ck = 02+00+04+00 + 01+13+E4+40 = 0x13E
    uint8_t args[4];
    ap_print_args(0x13, 0x40, args);
    n = ap_packet_build(AP_CMD_PRINT, args, 4, pkt);
    CHECK(n == 14, "PRINT length");
    CHECK(pkt[4] == 0x04 && pkt[5] == 0x00, "PRINT payload len field");
    CHECK(pkt[10] == 0x3E && pkt[11] == 0x01, "PRINT checksum (got %02X %02X)", pkt[10], pkt[11]);

    // DATA packet checksum: 640 x 0xFF payload
    uint8_t ff[AP_MAX_PAYLOAD];
    memset(ff, 0xFF, sizeof ff);
    n = ap_packet_build(AP_CMD_DATA, ff, AP_MAX_PAYLOAD, pkt);
    // ck = 04 + 80 + 02 + 640*0xFF = 0x86 + 0x27D80 = 0x27E06 -> 0x7E06
    CHECK(n == 6 + 640 + 4, "DATA length");
    CHECK(pkt[6 + 640] == 0x06 && pkt[7 + 640] == 0x7E,
          "DATA checksum (got %02X %02X)", pkt[6+640], pkt[7+640]);
}

static void test_status_error(void)
{
    CHECK(!ap_status_error(0x00), "clean status is not an error");
    CHECK(!ap_status_error(AP_STATUS_PRINTING), "busy is not an error");
    CHECK(ap_status_error(AP_STATUS_PAPER_JAM), "paper jam is an error");
    CHECK(ap_status_error(AP_STATUS_CHECKSUM_ERR), "checksum is an error");
}

static void test_wrap(void)
{
    static const char t[] = "abcd efgh ijkl";
    ApText txt;
    ap_text_reflow(&txt, t, (int)strlen(t), 4, AP_WRAP_POST_WORD);
    CHECK(txt.count == 3, "post-word wrap line count (got %d)", txt.count);
    CHECK(ap_text_line_len(&txt, t, 0) == 5, "line 0 length (got %d)",
          ap_text_line_len(&txt, t, 0));

    ap_text_reflow(&txt, t, (int)strlen(t), 4, AP_WRAP_MID_WORD);
    CHECK(txt.count == 4, "mid-word wrap line count (got %d)", txt.count);
}

static void test_font_matches_library(void)
{
    CHECK(!memcmp(font2bpp, ap_default_font, AP_FONT_GLYPHS * AP_FONT_GLYPH_BYTES),
          "GBScribe print font matches the AdvancePrint default font");
}

static void test_page_compose(void)
{
    ap_page_set_font(font2bpp, AP_FONT_GLYPHS);

    static const char t[] = "AB";
    ApText txt;
    ap_text_reflow(&txt, t, 2, AP_TILES_PER_ROW, AP_WRAP_POST_WORD);

    static uint8_t page[AP_PAGE_BYTES];
    ap_page_compose_text(page, t, &txt, 0);

    // first two tiles are the 'A' and 'B' glyphs from GBScribe's font
    CHECK(!memcmp(page, font2bpp + ('A' & 0x7F) * 16, 16), "tile 0 = 'A' glyph");
    CHECK(!memcmp(page + 16, font2bpp + ('B' & 0x7F) * 16, 16), "tile 1 = 'B' glyph");

    // remaining tiles on the line are blank (shade 0 = paper)
    int blank_tail = 1;
    for (int i = 2; i < AP_TILES_PER_ROW; i++)
        for (int b = 0; b < 16; b++)
            if (page[i * 16 + b]) blank_tail = 0;
    CHECK(blank_tail, "tail tiles on the text line are blank");

    // every row below the single text line is blank
    int blank_rows = 1;
    for (int i = AP_ROW_BYTES; i < AP_PAGE_BYTES; i++)
        if (page[i]) blank_rows = 0;
    CHECK(blank_rows, "rows past the text are blank");
}

int main(void)
{
    test_packet_framing();
    test_status_error();
    test_wrap();
    test_font_matches_library();
    test_page_compose();
    printf("%d checks, %d failures\n", checks, fails);
    return fails ? 1 : 0;
}
