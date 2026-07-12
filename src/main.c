// GBScribe — word processor + pixel art for GBA, printing to the Game Boy Printer.

#include <string.h>
#include <tonc.h>
#include "gfx/font.h"
#include "ui/screen.h"
#include "app.h"

static void init_video(void)
{
    REG_DISPCNT = DCNT_MODE0 | DCNT_BG0 | DCNT_BG1 | DCNT_OBJ | DCNT_OBJ_1D;
    REG_BG0CNT = BG_CBB(0) | BG_SBB(SBB_TEXT)   | BG_4BPP | BG_REG_32x32 | BG_PRIO(0);
    REG_BG1CNT = BG_CBB(0) | BG_SBB(SBB_CHROME) | BG_4BPP | BG_REG_32x32 | BG_PRIO(1);

    memcpy32(&tile_mem[0][0], font_tiles, FONT_TILE_COUNT * 8);

    pal_bg_mem[16*PB_INK   + 1] = RGB15( 2,  2,  3);   // document ink
    pal_bg_mem[16*PB_UI    + 1] = RGB15(26, 27, 29);   // keyboard label ink
    pal_bg_mem[16*PB_SEL   + 1] = RGB15( 1,  1,  2);   // selected key label ink
    pal_bg_mem[16*PB_SOLID + 1] = RGB15(31, 31, 30);   // paper
    pal_bg_mem[16*PB_SOLID + 2] = RGB15( 4,  5,  7);   // keyboard bg
    pal_bg_mem[16*PB_SOLID + 3] = RGB15( 6,  7, 10);   // gutter
    pal_bg_mem[16*PB_SOLID + 4] = RGB15(31, 24,  6);   // accent (selected key)
    pal_bg_mem[16*PB_SOLID + 5] = RGB15(10, 12, 16);   // separator
    pal_bg_mem[0] = RGB15(2, 2, 3);                    // backdrop

    // Caret sprite: 8x8 thick underscore, obj palette 0 color 1
    static const u32 caret_tile[8] = {0,0,0,0,0,0,0x11111111,0x11111111};
    memcpy32(&tile_mem[4][0], caret_tile, 8);
    pal_obj_mem[1] = RGB15(4, 8, 24);

    oam_init(oam_mem, 128);
}

int main(void)
{
    irq_init(NULL);
    irq_enable(II_VBLANK);

    init_video();
    app_init();

    int frame = 0;
    while (1) {
        VBlankIntrWait();
        key_poll();
        frame++;
        app_frame(frame);
    }
    return 0;
}
