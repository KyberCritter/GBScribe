#include <string.h>
#include "ui.h"
#include "screen.h"
#include "core/charset.h"
#include "gfx/logo.h"

// logo tiles live in CBB0 above the font (0..135) and QR (160..328) tiles
#define LOGO_TILE_BASE 336
#define PB_LOGO 6

void ui_cls(void)
{
    for (int y = 0; y < SCREEN_TH; y++)
        for (int x = 0; x < SCREEN_TW; x++) {
            map_put(text_map(), x, y, CH_BLANK, PB_UI);
            map_put(chrome_map(), x, y, SOLID(2), PB_SOLID);
        }
}

void ui_text(int x, int y, const char *s, int pb)
{
    for (; *s; s++, x++)
        map_put(text_map(), x, y, (u8)*s, pb);
}

void ui_fill(int x, int y, int w, int h, int solid_v)
{
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            map_put(chrome_map(), x + i, y + j, SOLID(solid_v), PB_SOLID);
}

void ui_logo(int x, int y)
{
    // load-on-use is cheap (4 tiles); palette: outline/quill light, body
    // gray, screen classic GB green
    memcpy32(&tile_mem[0][LOGO_TILE_BASE], logo_tiles, 4 * 8);
    pal_bg_mem[16 * PB_LOGO + 1] = RGB15(29, 29, 31);
    pal_bg_mem[16 * PB_LOGO + 2] = RGB15(14, 15, 19);
    pal_bg_mem[16 * PB_LOGO + 3] = RGB15(19, 23, 2);
    for (int ty = 0; ty < 2; ty++)
        for (int tx = 0; tx < 2; tx++)
            map_put(text_map(), x + tx, y + ty,
                    LOGO_TILE_BASE + ty * 2 + tx, PB_LOGO);
}

void ui_frame(int x, int y, int w, int h)
{
    u16 *tm = text_map();
    map_put(tm, x, y, CH_BOX_TL, PB_UI);
    map_put(tm, x + w - 1, y, CH_BOX_TR, PB_UI);
    map_put(tm, x, y + h - 1, CH_BOX_BL, PB_UI);
    map_put(tm, x + w - 1, y + h - 1, CH_BOX_BR, PB_UI);
    for (int i = 1; i < w - 1; i++) {
        map_put(tm, x + i, y, CH_BOX_H, PB_UI);
        map_put(tm, x + i, y + h - 1, CH_BOX_H, PB_UI);
    }
    for (int j = 1; j < h - 1; j++) {
        map_put(tm, x, y + j, CH_BOX_V, PB_UI);
        map_put(tm, x + w - 1, y + j, CH_BOX_V, PB_UI);
    }
}
