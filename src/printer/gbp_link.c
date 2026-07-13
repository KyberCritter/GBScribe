#include "gbp_link.h"
#include "gbp_packet.h"
#include "storage/storage.h"
#include "storage/save.h"

// RCNT data bits: SC=0, SD=1, SI=2, SO=3; direction bits +4 (1 = output).
#define R_GPIO   (1 << 15)
#define R_SC     (1 << 0)
#define R_SI     (1 << 2)
#define R_SO     (1 << 3)
#define R_SC_OUT (1 << 4)
#define R_SO_OUT (1 << 7)

void gbp_link_init(void)
{
    REG_SIOCNT = 0;
    // general purpose mode; SC+SO outputs, SI+SD inputs; SC idles HIGH, SO low
    REG_RCNT = R_GPIO | R_SC_OUT | R_SO_OUT | R_SC;

    // TM2: free-running, prescaler 1 (59.6ns/tick), used for polled delays.
    // TM0/TM1 belong to Maxmod's audio FIFO — touching them corrupts audio AND
    // our bit timing (that was the "constant crackle + status 00" hardware bug).
    REG_TM2CNT_H = 0;
    REG_TM2CNT_L = 0;
    REG_TM2CNT_H = TM_ENABLE;
}

static inline void delay_ticks(u16 t)
{
    u16 start = REG_TM2CNT_L;
    while ((u16)(REG_TM2CNT_L - start) < t) ;
}

u8 gbp_link_xfer(u8 send, u16 half_ticks)
{
    u8 recv = 0;
    u16 base = (REG_RCNT | R_SC) & ~(u16)R_SO;   // keep mode/dir bits

    u32 ime = REG_IME;
    REG_IME = 0;                                  // no IRQ may split a byte
    for (int bit = 7; bit >= 0; bit--) {
        u16 lo = base & ~(u16)R_SC;
        if (send & (1 << bit))
            lo |= R_SO;
        REG_RCNT = lo;                            // clock low, data out
        delay_ticks(half_ticks);
        REG_RCNT = lo | R_SC;                     // rising edge: printer samples
        recv = (u8)((recv << 1) | ((REG_RCNT >> 2) & 1));  // we sample SI
        delay_ticks(half_ticks);
    }
    REG_IME = ime;                                // IRQs may run between bytes
    return recv;
}

void gbp_link_send(const u8 *pkt, int n, u16 half_ticks, u8 *keepalive, u8 *status)
{
    u8 last2[2] = {0, 0};
    for (int i = 0; i < n; i++) {
        u8 r = gbp_link_xfer(pkt[i], half_ticks);
        last2[0] = last2[1];
        last2[1] = r;
    }
    if (keepalive) *keepalive = last2[0];
    if (status)    *status = last2[1];
}

bool gbp_link_detect(u8 *status_out)
{
    u8 pkt[16];
    int n = gbp_packet_build(GBP_CMD_INQUIRY, 0, 0, pkt);
    u8 ka = 0, st = 0;
    gbp_link_send(pkt, n, GBP_HALF_NORMAL, &ka, &st);
    if (status_out)
        *status_out = st;
    // pull-ups read 0xFF with nothing attached; a real printer answers 0x81
    return ka == GBP_KEEPALIVE;
}

