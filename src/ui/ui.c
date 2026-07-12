#include "ui.h"
#include "screen.h"
#include "core/charset.h"

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
