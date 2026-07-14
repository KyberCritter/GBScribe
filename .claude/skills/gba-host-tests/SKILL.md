---
name: gba-host-tests
description: Run GBScribe's host-side unit tests in WSL — layout engine checks plus the Game Boy Printer end-to-end decode harness that proves the byte stream without hardware. Run after every code change to core/, printer/, or gfx/.
---

# Host tests (gcc in WSL): layout + printer protocol harness

The GBA-independent core (`src/core/`, `src/printer/gbp_packet.c`, `gbp_print.c`,
`src/gfx/font2bpp.c`) compiles with plain gcc, so logic is tested on the host —
no emulator or hardware needed.

## Run

`make` is not on the Windows PATH; run inside WSL Ubuntu:

```sh
cd /d/Coding/GBScribe/tests/host && wsl make -s run
# or individually after building:
wsl bash -c './bin/test_layout && ./bin/test_printer'
```

Expected: `56 checks, 0 failures` (layout) and `12 checks, 0 failures` (printer).
Counts grow as tests are added — the number that matters is `0 failures`.

## What each binary proves

- **test_layout** — document model + word-wrap engine: wrap modes, caret math,
  scrolling, page pagination. Links `src/core/layout.c` + `document.c` directly.
- **test_printer** — two layers:
  1. Unit checks on exact packet bytes: INIT/INQUIRY/PRINT/DATA framing, length
     fields, and little-endian checksums against hand-computed references.
  2. **End-to-end decode harness**: composes a real document page to the 5,760-byte
     GB 2bpp format, builds the full printer byte stream, then feeds it to
     `tests/host/decoder/gpbdecoder` — mofosyne's INDEPENDENT third-party Game Boy
     Printer decoder (C++, built by the same Makefile) — and compares the decoded
     BMP pixels against a reference render. This proves packet framing, payload
     order, and tile packing round-trip correctly without a printer.

## Visual inspection

test_printer writes the decoded page BMPs to `tests/host/bin/` — view them with the
Read tool when a printer-path change might alter page composition (fonts, margins,
shade mapping). This is the closest thing to "what the paper will look like" short
of real hardware.

## Limits

The harness cannot test the link-port GPIO bit-bang driver (`gbp_link.c`), timing,
or printer status handling — those need the real GBA + printer (user test ladder in
STATUS.md).
