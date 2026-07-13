// Small drawing helpers shared by menu-style scenes.
#ifndef UI_UI_H
#define UI_UI_H

#include <tonc.h>

void ui_cls(void);                                    // dark background, no glyphs
void ui_text(int x, int y, const char *s, int pb);    // glyph string on BG0
void ui_fill(int x, int y, int w, int h, int solid_v);// solid tiles on BG1
void ui_frame(int x, int y, int w, int h);            // box-drawing border on BG0
void ui_logo(int x, int y);                           // 2x2-tile GBScribe logo on BG0

#endif
