// Shared screen layout constants and BG map helpers.
#ifndef UI_SCREEN_H
#define UI_SCREEN_H

#include <tonc.h>
#include "gfx/font.h"

// BG0 = glyph layer (text + keyboard labels), BG1 = chrome solids behind it.
#define SBB_TEXT    30
#define SBB_CHROME  31

// Palette banks
#define PB_INK      0   // [1]=black ink (document text on paper)
#define PB_UI       1   // [1]=light ink (keyboard labels on dark)
#define PB_SOLID    2   // [1]=paper [2]=key bg [3]=gutter [4]=accent [5]=separator
#define PB_SEL      3   // [1]=black ink (selected key label on accent)

// Page geometry (tiles): the printed page is 20 tiles wide, centered.
#define PAGE_X0     5
#define PAGE_W      20
#define PAGE_ROWS   13  // rows 0..12 are the page view; 13 separator; 14+ keyboard

#define SCREEN_TW   30
#define SCREEN_TH   20

INLINE u16 *text_map(void)   { return se_mem[SBB_TEXT]; }
INLINE u16 *chrome_map(void) { return se_mem[SBB_CHROME]; }

INLINE void map_put(u16 *map, int x, int y, int tile, int pb)
{ map[y * 32 + x] = SE_PALBANK(pb) | tile; }

#define SOLID(v) FONT_SOLID_TILE(v)

#endif
