---
name: gba-asset-pipeline
description: Regenerate GBScribe's generated pixel assets (font, logo, QR page, printer test page) from the Python tools and visually verify them via PNG previews. Use when changing the logo, QR/URL, test page, or font.
---

# Generated asset pipeline + visual verification

All pixel assets are generated into checked-in C sources by scripts in `tools/`.
Never hand-edit `src/gfx/*.c` generated files — edit the generator and re-run it.

## The generators (run from `tools/`, they import each other)

```sh
cd /d/Coding/GBScribe/tools
python3 mklogo.py       # ART strings -> src/gfx/logo.c/h + assets/logo.png + preview
python3 mkqr.py         # URL -> src/gfx/qr.c/h (screen tiles + printed QR page)
python3 mktestpage.py   # -> src/gfx/testpage.c/h (printer test page)
python3 mkfont.py       # font8x8 -> font tiles (rarely needed)
```

- `mklogo.py` is the logo's single source of truth: a 16×16 `ART` grid,
  `.`=0 blank `/`=1 gray `X`=2 near-white `O`=3 darkest (green on screen, black
  on paper). `mkqr.py` and `mktestpage.py` import `logo_grid()` from it, so after
  a logo edit re-run all three.
- On-screen logo colors are separate from pixel data: GBA palette in `ui_logo()`
  (`src/ui/ui.c`), PNG render colors near the bottom of `mklogo.py`.
- Scripts must run from `tools/` (sibling imports). WSL/Git Bash `python3` works;
  **PIL is not installed** — previews are written with a built-in minimal
  zlib/struct PNG encoder, don't add a Pillow dependency.

## Visual verification (user rule)

Every visual change must be checked by exporting an image and LOOKING at it:

- Previews land in `.scratchwork/` (gitignored): `logo_preview.png` (16×),
  `qr_preview.png`, `testpage_preview.png` (2×).
- View with the Read tool. If a preview looks wrong, verify by decoding pixel
  values programmatically before concluding — small thumbnails mislead.
- Show the user significant results with SendUserFile.

## Print-page format (shared by QR page, test page, drawing canvas)

Printed pages are 5,760 bytes: 160×144 px as 20×18 tiles of GB 2bpp (16 bytes per
8×8 tile, 2 bytes per row, bit 0x80 = leftmost, plane0|plane1<<1 = shade 0..3,
0 = white paper). The `put_px` helper in `mkqr.py`/`mktestpage.py` encodes this.
Printed text uses font8x8 glyphs (LSB = leftmost); max 20 chars per 160 px row.

## After regenerating

Rebuild the ROM (`gba-build` skill) — generated .c files compile in automatically —
and commit the regenerated sources together with the generator change.
