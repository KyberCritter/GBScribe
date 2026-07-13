// Drawing mode: 160x144 canvas, one printed page. Master copy is GB 2bpp in
// EWRAM so saving and printing are straight memcpys (see PLAN.md).
#ifndef UI_DRAW_H
#define UI_DRAW_H

#include <tonc.h>

void draw_enter(void);
void draw_frame(int frame);
u8 *draw_canvas(void);        // 5,760-byte 2bpp master
void draw_canvas_reloaded(void); // call after loading into draw_canvas()
void draw_clear(void);
void draw_apply_palette(void);   // push g_save.draw_pal shades to the BG palette
int draw_pal_count(void);
const char *draw_pal_name(int i);

#endif
