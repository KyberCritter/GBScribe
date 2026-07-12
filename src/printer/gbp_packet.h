// Game Boy Printer packet building. PORTABLE C — compiled for both the GBA ROM
// and the host test harness. No GBA headers allowed here (see PLAN.md).
#ifndef PRINTER_GBP_PACKET_H
#define PRINTER_GBP_PACKET_H

#include <stdint.h>

#define GBP_CMD_INIT    0x01
#define GBP_CMD_PRINT   0x02
#define GBP_CMD_DATA    0x04
#define GBP_CMD_BREAK   0x08
#define GBP_CMD_INQUIRY 0x0F

// status byte bits (printer -> GBA)
#define GBP_STATUS_CHECKSUM_ERR (1 << 0)
#define GBP_STATUS_PRINTING     (1 << 1)
#define GBP_STATUS_DATA_FULL    (1 << 2)
#define GBP_STATUS_UNPROCESSED  (1 << 3)
#define GBP_STATUS_PACKET_ERR   (1 << 4)
#define GBP_STATUS_PAPER_JAM    (1 << 5)
#define GBP_STATUS_OTHER_ERR    (1 << 6)
#define GBP_STATUS_LOW_BATTERY  (1 << 7)

#define GBP_KEEPALIVE 0x81       // printer's reply in the keepalive slot

#define GBP_MAX_PAYLOAD 0x280    // 640 bytes = 40 tiles = 160x16 px per DATA packet

// header(6) + payload + checksum(2) + keepalive/status slots(2)
#define GBP_PACKET_MAX (6 + GBP_MAX_PAYLOAD + 2 + 2)

// Build a full packet into `out` (>= GBP_PACKET_MAX bytes when payload is max).
// Returns the total number of bytes to clock out. The final two bytes are 0x00
// placeholders; the printer answers keepalive/status in those transfer slots.
int gbp_packet_build(uint8_t cmd, const uint8_t *payload, int len, uint8_t *out);

// Offset of the keepalive / status reply bytes within a built packet.
#define GBP_REPLY_KEEPALIVE(total) ((total) - 2)
#define GBP_REPLY_STATUS(total)    ((total) - 1)

// PRINT command payload: sheets=1, margins hi=before lo=after, palette 0xE4,
// exposure 0..0x7F.
void gbp_print_args(uint8_t margins, uint8_t exposure, uint8_t out[4]);

#endif
