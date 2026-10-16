---
name: gba-host-tests
description: Run GBScribe's host-side unit tests in WSL — layout engine checks plus printer packet-byte framing/checksum tests. Run after every code change to core/, the printer layer, or gfx/.
---

# Host tests (gcc in WSL): layout + printer packet protocol

The GBA-independent core (`src/core/`, the AdvancePrint sources under
`lib/AdvancePrint/src/`, and `src/gfx/font2bpp.c`) compiles with plain gcc, so
logic is tested on the host — no emulator or hardware needed.

## Run

`make` is not on the Windows PATH; run inside WSL Ubuntu:

```sh
cd /d/Coding/GBScribe/tests/host && wsl make -s run
# or individually after building:
wsl bash -c './bin/test_layout && ./bin/test_printer'
```

Expected: `201 checks, 0 failures` (layout) and `19 checks, 0 failures` (printer).
Counts grow as tests are added — the number that matters is `0 failures`.

## What each binary proves

- **test_layout** — document model + word-wrap engine: wrap modes, caret math,
  scrolling, page pagination. Links `src/core/layout.c` + `document.c` directly.
- **test_printer** — unit checks on exact packet bytes: INIT/INQUIRY/PRINT/DATA
  framing, length fields, and little-endian checksums against hand-computed
  references, plus the AdvancePrint wrap engine, page composition with GBScribe's
  font, and a check that the two fonts agree. This proves the on-the-wire
  protocol is correct without needing a printer.

## Limits

The harness cannot test the link-port GPIO bit-bang driver
(`advanceprint_link.c`), timing, or printer status handling — those need the real
GBA + printer (user test ladder in STATUS.md).
