# GBScribe

A homebrew word processor and pixel-art tool for Game Boy hardware that prints to a real
Game Boy Printer.

By Scott Ratchford — [scottratchford.com](https://scottratchford.com)

> Design rationale, protocol details, and timing constraints live in **[PLAN.md](PLAN.md)**;
> implementation status and handoff notes live in **[STATUS.md](STATUS.md)**.

## Why it's a GBA app, not a Game Boy Color app

The target hardware is a Game Boy Advance running an **EZ-Flash Omega Definitive Edition**.
That cart runs `.gb`/`.gbc` files through **Goomba Color**, a software emulator on the GBA's
ARM CPU, which has no serial link implementation — so a Game Boy Color ROM on this cart
**can never drive the printer**, the one feature the whole project exists for.

GBScribe is therefore a **native GBA application**, and it talks to the Game Boy Printer by
bit-banging the link port in general-purpose GPIO mode. Full reasoning, protocol details and
timing constraints are in [PLAN.md](PLAN.md).

```
GBA ── link port (GPIO mode) ── GBC/GB-Pocket link cable ── Game Boy Printer
```

## Features

- WYSIWYG text editor with an on-screen QWERTY keyboard, rendered at true print width
  (the on-screen page is exactly the 160px that prints)
- Full ASCII plus arrows, geometric shapes, and box-drawing characters
- Word wrap (mid-word or post-word), explicit line breaks, documents up to 16,000
  characters (~44 printed pages)
- Pixel-art drawing mode: 160×144 canvas (one printed page), pencil/eraser/line/rect,
  stamp library, 4 shades with selectable display palettes
- 8 save slots on cart flash (FLASH1M), each holding a document or a drawing
- Prints documents, drawings, and a QR code; live printer detection on the title screen
  (a bare INQUIRY round trip — burns no paper); 8192 Hz or ~16 kHz link speed
- Original chiptune theme + UI sounds (Maxmod); music pauses during printing to
  protect link timing

## Controls

| Context | Keys |
|---|---|
| Editor | D-pad: keyboard cursor · A: type (hold to repeat) · B: backspace · **L+D-pad: move caret** · R: space · Select: cycle glyph layer · Start: menu |
| Drawing | D-pad: cursor · A: apply tool (drag for pencil/eraser) · B: cancel shape · Select: cycle tool · L/R: color or stamp · Start: drawing menu |
| Print | A: confirm · B: cancel/back |

## Building

Requires Docker (uses the official `devkitpro/devkitarm` image):

```sh
./build.sh          # -> GBScribe.gba
./build.sh clean
```

Host-side tests (any POSIX box with gcc/g++; WSL works):

```sh
make -C tests/host run
```

The printer byte stream is verified without hardware by decoding it with mofosyne's
independent `GameBoyPrinterDecoderC` and pixel-comparing the result
(`tests/host/test_printer.c`). Generated sources (font, QR, music) are committed;
regenerate with the scripts in `tools/` (Python 3; `segno` needed for the QR).

## Layout

```
src/        application source
  printer/  GB Printer driver — gbp_packet.c/gbp_print.c are portable C and MUST
            stay free of GBA headers, so they compile for the host harness too
  core/     document model, layout/wrap engine, character set
  ui/       title, menus, editor, drawing, settings, credits/about
  storage/  save slots (FLASH1M, with an SRAM fallback build flag)
  gfx/      generated sources (font, QR) — regenerate via tools/, don't hand-edit
assets/     generated music/SFX sources
tests/host/ host-side unit tests and the printer decode harness
tools/      build scripts: font, QR, music generation; mGBA capture; deploy
LICENSES/   dependency license texts
.claude/    Claude Code config; resources/ holds vendored technical documentation
```

## Legal

Not affiliated with, endorsed by, or sponsored by Nintendo. Game Boy, Game Boy Color, Game
Boy Advance and Game Boy Printer are trademarks of Nintendo. "QR Code" is a registered
trademark of DENSO WAVE Incorporated. Dependency licenses are vendored in `LICENSES/`.
