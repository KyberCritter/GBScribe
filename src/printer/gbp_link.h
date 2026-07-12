// Game Boy Printer link driver: bit-bangs the GBA link port in general-purpose
// GPIO mode (RCNT bit 15). GBA-ONLY — everything protocol-level lives in the
// portable gbp_packet.c/gbp_print.c.
//
// Wiring (GBC/GB-Pocket cable, crosses SO<->SI): GBA SC -> printer clock,
// GBA SO -> printer SIN, GBA SI <- printer SOUT.
#ifndef PRINTER_GBP_LINK_H
#define PRINTER_GBP_LINK_H

#include <tonc.h>

void gbp_link_init(void);       // GPIO mode, SC/SO out (SC idles high), TM0 running

// Clock one byte MSB-first at `half_ticks` (TM0 ticks per half period),
// IRQs masked for the byte. Returns the byte the printer clocked back.
u8 gbp_link_xfer(u8 send, u16 half_ticks);

// Send a built packet (gbp_packet_build output). The printer's replies to the
// final two bytes land in *keepalive / *status. half_ticks: see below.
void gbp_link_send(const u8 *pkt, int n, u16 half_ticks, u8 *keepalive, u8 *status);

// One INQUIRY round trip; true if a printer answered (keepalive 0x81).
// Uses the safe 8192 Hz rate. No paper is used.
bool gbp_link_detect(u8 *status_out);

// half periods in TM0 ticks (59.6ns each): 61us ~ 1024 (8192 Hz stock rate),
// 30us ~ 503 (~16 kHz "double speed", proven by Boichot's Arduino master)
#define GBP_HALF_NORMAL 1024
#define GBP_HALF_FAST   503

// ticks for the current speed setting (g_save.print_speed: 0 = fast)
u16 gbp_half_ticks(void);

#endif
