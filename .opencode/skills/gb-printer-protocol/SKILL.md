---
name: gb-printer-protocol
description: How GBScribe drives a real Game Boy Printer from a GBA ROM — link-port GPIO bit-banging, packet protocol, page format, print state machine, and the hardware lessons (timer conflicts, timing rates, garbled status polls). The driver is the AdvancePrint library (lib/AdvancePrint/); read before touching it or src/ui/print.c.
---

# Driving the Game Boy Printer from a GBA ROM

The Game Boy Printer is a DMG-era serial peripheral. The GBA's link port speaks a
different native serial protocol, so GBScribe ignores the hardware serial engine
entirely and **bit-bangs the DMG protocol in software** over the link port's
general-purpose I/O mode. Since the AdvancePrint refactor all of that lives in the
standalone **AdvancePrint** library (`lib/AdvancePrint/`, a git submodule); the
GBScribe tree itself only touches the library's public API. The layers:

| Layer | File | Portability |
|---|---|---|
| Wire (GPIO bit-bang) | `lib/AdvancePrint/src/advanceprint_link.c` | GBA-only |
| Protocol (packets/checksums) | `lib/AdvancePrint/src/advanceprint_packet.c` | portable C, host-testable |
| Text wrap + page compose | `lib/AdvancePrint/src/advanceprint_text.c`, `advanceprint_page.c` | portable C, host-testable |
| Job engine (INIT..WAIT) | `lib/AdvancePrint/src/advanceprint_job.c` | portable C (calls the link) |
| Source latching + UI | `src/ui/print.c` | GBA scene |

## 1. The wire: link port as GPIO

Setting `REG_RCNT` bit 15 puts the port in general-purpose mode: pins SC/SD/SI/SO
become GPIO, data in bits 0-3, direction in bits 4-7 (1 = output).

- Config (`ap_link_init`): SC + SO outputs, SI input, **SC idles HIGH** (DMG
  clock idle state). With a GBC/Pocket cable (which crosses SO↔SI): GBA SC →
  printer clock, GBA SO → printer SIN, GBA SI ← printer SOUT.
- One byte (`ap_link_xfer`), MSB first, per bit: drive SC low + put data bit on
  SO → wait half a period → drive SC high (printer samples our bit on the rising
  edge) and immediately read SI (the printer's bit) → wait half a period. Full
  duplex: every byte sent clocks a byte back.
- **Timing**: DMG "stock" rate is 8192 Hz ⇒ 61 µs half-period. Delays are polled
  on **TM2** free-running at prescaler 1 (59.6 ns/tick): `GBP_HALF_NORMAL = 1024`
  ticks. `IME` is cleared for the 8 bits of each byte (an IRQ mid-byte would
  stretch a half-period and corrupt the transfer); interrupts run freely between
  bytes, and the printer doesn't care about inter-byte gaps.

**Hardware lesson #1 (the "status 00 + audio crackle" bug):** the first version
used TM0 for delays — but TM0/TM1 belong to Maxmod's audio FIFO. Reprogramming
TM0 corrupted both the audio AND the bit timing. Printer timing lives on TM2 only.

**Hardware lesson #2:** a ~16 kHz "fast" mode worked in emulators but returned
status 00 on the real printer. Everything now runs at the stock 8192 Hz
(one rate, no option). Emulators do not validate link timing — only hardware does.

## 2. The protocol: packets

Every command is one packet (`ap_packet_build`):

```
88 33 | cmd | 00 | len_lo len_hi | payload… | ck_lo ck_hi | 00 00
magic   1B   compr    length                   checksum     reply slots
```

Checksum = 16-bit sum of everything from `cmd` through the payload (magic
excluded). The two trailing `00` bytes exist to clock the printer's reply out:
the printer answers the first with **keepalive `0x81`** (alive) and the second
with its **status byte**. Pull-ups read `0xFF` with nothing attached — that's how
`ap_link_detect()` distinguishes "no printer" (a bare INQUIRY costs no paper;
the title screen polls it once a second).

Commands used: `INIT (0x01)`, `DATA (0x04)`, `PRINT (0x02)`, `INQUIRY (0x0F)`.

Status bits that matter: bit 1 = PRINTING, plus CHECKSUM_ERR / PACKET_ERR /
PAPER_JAM / LOW_BATTERY / OTHER_ERR (see `advanceprint_packet.h`).

## 3. The chunk (buffer-load)

Print data is 160 px wide: 20 tiles/row of **GB 2bpp** (16 bytes/tile: 2 bytes
per pixel row, plane0|plane1<<1 = shade 0..3, 0 = white paper), 320 bytes per
tile row. The printer's RAM buffer holds at most 18 rows (one GB screen,
5,760 bytes), so a document prints as a sequence of **chunks** of up to 18
rows each. Text output is **contiguous, not page-based**: the last chunk is
trimmed to the remaining layout lines (rounded up to the 2-row band the head
advances in), so no blank paper is fed after the text. Text rows are composed
from the layout engine + `font2bpp` (`ap_page_compose_text`, with GBScribe's
font installed via `ap_page_set_font` at startup); the drawing
canvas, QR page, and test page already ARE this format — raw images of any
tile-row height printed via `print_set_page` (the test page is a multi-chunk
strip; canvas and QR are single 18-row buffers).

Each chunk is sent as **up to 9 DATA packets × 640 bytes** (2 tile rows each),
then one **empty DATA packet** arms the buffer, then **PRINT** with 4 args:
`{sheets=1, margins, palette=0xE4, exposure=0x40}`. Margins make the chunks
join seamlessly into one document: feed-before (`0x10`) on the first chunk,
feed-after (`0x03`) on the last, `0x00` between (`0x13` when a single chunk
is both).

## 4. The job engine (`advanceprint_job.c`) + UI (`src/ui/print.c`)

`ap_job_update()` emits at most one packet per 60 Hz frame, so the UI stays live
and B can always cancel (between packets the printer just times out and resets):

```
CONFIRM → ap_job_begin → INIT → SETTLE (~200ms) → DATA ×≤9 → ARM (empty DATA) → PRINT → WAIT
                                                                                          ↓
                                                        next chunk → INIT …  or DONE / ERROR
```

The print scene latches the source, probes with `ap_job_probe` on entry, then
drives `ap_job_begin` / `ap_job_update` and renders `rows_sent` / `rows_total` /
`printing`.

Progress UI is line-based: `Line X/Y` plus an 18-cell bar spanning the whole
job by rows sent.

The WAIT phase polls INQUIRY every 3 frames (~50 ms — doubles as keepalive so the
printer doesn't time out mid-motor-spin) and decides completion:

- status PRINTING seen → `saw_printing = true`; done when PRINTING clears.
- **Hardware lesson #3:** replies garble or go silent while the motor runs.
  Up to 40 consecutive bad polls (~2 s) are tolerated (`bad_polls`). Persistent
  silence AFTER `saw_printing` counts as **success** (the printer finished and
  went quiet) — before it, it's a real "Printer stopped responding." error.
  Without this tolerance, successful prints ended in a false error popup.
- Any error status bit → error phase with a decoded message (jam / battery /
  checksum / raw hex); the scene maps it to a screen.

Print sources: editor document (default), any text buffer via
`print_set_text(text, len, back)` (the built-in manual), or a raw 2bpp image
via `print_set_page(page, tile_rows, back)` — 320 bytes per tile row, any
height (canvas and QR are 18 rows; the test page is taller and streams in
chunks). Both are one-shot latches set right before `app_goto(ST_PRINT)`;
`back` is where B exits.

## 5. How it was validated without hardware

The protocol layer was proven before the driver existed and is now owned by
AdvancePrint: the host harness (`tests/host/test_printer.c`, see the
`gba-host-tests` skill) compiles the AdvancePrint sources directly and checks
exact packet bytes against hand-computed checksums, the wrap engine, and page
composition. So when the first hardware test failed, the byte stream was already
trusted — debugging focused (correctly) on timing and GPIO, which is where all
three real bugs were.

Field debugging aid: the title chooser's "Print test page" prints shade bands +
alignment patterns (`src/gfx/testpage.c`) to verify the full chain on paper.
