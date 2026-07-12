#include "gbp_packet.h"

int gbp_packet_build(uint8_t cmd, const uint8_t *payload, int len, uint8_t *out)
{
    int n = 0;
    out[n++] = 0x88;
    out[n++] = 0x33;
    out[n++] = cmd;
    out[n++] = 0x00;                 // compression: none
    out[n++] = (uint8_t)(len & 0xFF);
    out[n++] = (uint8_t)(len >> 8);
    for (int i = 0; i < len; i++)
        out[n++] = payload[i];

    // checksum: sum of command..payload (everything after the magic bytes)
    uint16_t sum = 0;
    for (int i = 2; i < n; i++)
        sum += out[i];
    out[n++] = (uint8_t)(sum & 0xFF);
    out[n++] = (uint8_t)(sum >> 8);

    out[n++] = 0x00;                 // keepalive reply slot
    out[n++] = 0x00;                 // status reply slot
    return n;
}

void gbp_print_args(uint8_t margins, uint8_t exposure, uint8_t out[4])
{
    out[0] = 0x01;                   // one sheet
    out[1] = margins;
    out[2] = 0xE4;                   // palette: 3-2-1-0
    out[3] = (uint8_t)(exposure & 0x7F);
}
