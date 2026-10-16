# GBScribe

![GBScribe logo: a Game Boy with a quill and ink pot](assets/logo.png)

A homebrew word processor and pixel-art tool for Game Boy Advance hardware that prints to a real
Game Boy Printer.

By Scott Ratchford — [scottratchford.com](https://scottratchford.com)

Licensed under the **MIT License** — see [LICENSE.md](LICENSE.md).

## Why it's a GBA app, not a Game Boy Color app

The target hardware is a Game Boy Advance running an **EZ-Flash Omega Definitive Edition**.
That cart runs `.gb`/`.gbc` files through **Goomba Color**, a software emulator on the GBA's ARM CPU, which has no serial link implementation.
This prevents the use of accessories like the printer.

This requires GBScribe to be a **native GBA application**.
It communicates with the Game Boy Printer by bit-banging the link port in general-purpose GPIO mode.

```txt
GBA ── link port (GPIO mode) ── GBC/GB-Pocket link cable ── Game Boy Printer
```

## Features

- WYSIWYG text editor with an on-screen QWERTY keyboard, rendered at true print width (the on-screen page is exactly the 160px that prints)
- Full ASCII plus arrows, shapes, and specialty characters
- Word wrap (mid-word or post-word), explicit line breaks, documents up to 16,000 characters (~44 printed pages)
- Pixel-art drawing mode: 160×144 canvas (one printed page), pencil/eraser/line/rect, stamp library, 4 shades with selectable display palettes
- 8 save slots, each holding a document or a drawing
  - The save backend (FLASH1M or plain SRAM/FRAM, as on the EZ-Flash Omega) is detected at runtime
- Prints documents, drawings, and the author's contact card
- Live printer detection on the title screen

## Controls

### Editor

| Action | Key |
| --- | --- |
| Keyboard cursor | D-Pad |
| Select character | A |
| Backspace | B |
| ? | L + D-pad |
| Space | R |
| Cycle keyboard | Select |
| Open menu | Start |

### Drawing

| Action | Key |
| --- | --- |
| Cursor | D-pad |
| Apply tool | A |
| Cancel | B |
| Cycle tool | Select |
| Color or stamp | L/R |
| Open menu | Start |

## Building

The Game Boy Printer driver is a separate library, [AdvancePrint](https://github.com/KyberCritter/AdvancePrint),
pulled in as a git submodule at `lib/AdvancePrint/`. Clone with submodules (or
initialize them after a plain clone):

```sh
git clone --recurse-submodules <repo>
# existing clone:
git submodule update --init --recursive
```

Requires Docker (uses the official `devkitpro/devkitarm` image):

```sh
./build.sh          # -> GBScribe.gba
./build.sh clean
```

Host-side tests:

```sh
make -C tests/host run
```

The printer packet framing and checksums are verified on the host against
hand-computed byte references, now exercising the AdvancePrint sources directly
(`tests/host/test_printer.c`). Generated assets (font, QR, music) are committed.
Regenerate assets with the scripts in `tools/` (Python 3; `segno` needed for the
QR).

## Layout

```txt
src/        application source
  core/     document model, layout/wrap engine, character set
  ui/       title, menus, editor, drawing, settings, credits/about, print scene
  storage/  save slots (runtime-detected FLASH1M or SRAM backend)
  gfx/      generated sources (font, QR, logo) — regenerate via tools/, don't hand-edit
lib/AdvancePrint/  GB Printer driver (git submodule): GPIO link, packets, page
            composition, and the frame-driven print-job engine the print scene uses
assets/     generated music/SFX sources + logo.png
tests/host/ host-side unit tests (layout engine + printer packet/page bytes)
tools/      build scripts: font, QR, music generation; mGBA capture; deploy
LICENSES/   dependency license texts
.opencode/  project skills: build, host tests, mGBA capture, printer protocol
```

## Legal

GBScribe is free software, licensed under the MIT License — see
[LICENSE.md](LICENSE.md).

Not affiliated with, endorsed by, or sponsored by Nintendo. Game Boy, Game Boy Color, Game
Boy Advance and Game Boy Printer are trademarks of Nintendo. "QR Code" is a registered
trademark of DENSO WAVE Incorporated. Dependency licenses are vendored in `LICENSES/`.
