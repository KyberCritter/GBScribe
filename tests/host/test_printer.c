// Host harness: verify the exact printer packet bytes against hand-computed
// references. Proves INIT/INQUIRY/PRINT/DATA framing, length fields, and
// little-endian checksums without hardware.
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "printer/gbp_packet.h"
#include "printer/gbp_print.h"

static int fails, checks;
#define CHECK(cond, ...) do { \
    checks++; \
    if (!(cond)) { fails++; printf("FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } \
} while (0)

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

int main(void)
{
    test_packet_framing();
    printf("%d checks, %d failures\n", checks, fails);
    return fails ? 1 : 0;
}
