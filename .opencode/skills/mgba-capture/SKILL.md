---
name: mgba-capture
description: Boot the ROM in mGBA on Windows, send real button inputs, and screenshot the emulator window for visual verification of screens and flows. Use to check any UI change when the desktop is unlocked.
---

# mGBA capture with real inputs

mGBA 0.10.5 (Qt) at `C:\Program Files\mGBA\mGBA.exe` runs natively on Windows —
NOT in Docker (Docker is only the build toolchain). The Qt build has **no
`--script` CLI flag**, so automation is done by driving the real window:
`tools/capture_mgba.ps1` launches mGBA, waits for boot, injects keystrokes with
`keybd_event`, screenshots the client area (DPI-aware), and kills the process.

## Usage

```powershell
& tools\capture_mgba.ps1 -Rom GBScribe.gba -Out .scratchwork\title.png
& tools\capture_mgba.ps1 -Rom GBScribe.gba -Out .scratchwork\chooser.png `
    -DelaySec 6 -SendKeys "enter pause down down x"
```

Never invoke via `powershell -ExecutionPolicy Bypass -File ...` — that form is
denied by the permission policy. Always call the script directly with `&`.

View the PNG with the Read tool. Save captures under `.scratchwork/` (gitignored) —
user rule for all visual verification images.

## -SendKeys token language

Space-separated tokens, executed in order (default press = 130 ms):

- Letters/digits — mGBA default binds: `x`=A, `z`=B, `a`=L, `s`=R
- `left right up down` — D-pad; `enter`=Start, `bksp`=Select
- `pause` — 600 ms wait (use between screens so the ROM can redraw)
- `holdN:key` — hold for N tenths of a second, e.g. `hold15:right` = 1.5 s
- `key+key` chords, e.g. `a+left` (L shoulder + D-pad together)

Example — title → Start chooser → pick "Load a file":
`-SendKeys "enter pause down down x pause"`

## Behavior and pitfalls

- The script first kills stray mGBA instances (they hold a ROM file lock that
  breaks the next build), pins the window topmost at (60,60), and re-asserts
  foreground before the screenshot.
- **The desktop must be unlocked.** On a locked machine every capture returns the
  Windows lock screen. Detect it (a capture that isn't GBA content) and STOP —
  do not retry in a loop; fall back to generated previews / coordinate math and
  tell the user captures are blocked.
- `-DelaySec` (default 5) is boot wait; increase if captures show the BIOS/intro.
- Screenshots capture the whole client area at window scale `-3` (720×480).
- `tests/mgba/screenshot.lua` exists for mGBA's Tools→Scripting menu (frame-count
  screenshot + marker files) but cannot be loaded from the CLI — manual use only.
