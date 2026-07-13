#include <stdio.h>
#include <tonc.h>
#include "app.h"
#include "ui/ui.h"
#include "ui/screen.h"
#include "ui/draw.h"
#include "core/charset.h"
#include "storage/storage.h"
#include "storage/save.h"

static int sel;
#define N_ITEMS 7

static void draw_items(void)
{
    char line[24];
    // frame spans cols 6..24, rows 1..18; last item row 17 stays inside.
    ui_cls();
    ui_fill(6, 1, 19, 18, 3);
    ui_frame(6, 1, 19, 18);
    ui_text(9, 2, "- DRAWING -", PB_UI);

    ui_text(8, 5,  "Resume drawing", PB_UI);
    ui_text(8, 7,  "Print drawing", PB_UI);
    ui_text(8, 9,  "Save drawing", PB_UI);
    ui_text(8, 11, "Load drawing", PB_UI);
    siprintf(line, "Palette: %s", draw_pal_name(g_save.draw_pal));
    ui_text(8, 13, line, PB_UI);
    ui_text(8, 15, "Clear canvas", PB_UI);
    ui_text(8, 17, "Main menu", PB_UI);

    for (int i = 0; i < N_ITEMS; i++)
        map_put(text_map(), 7, 5 + i * 2, i == sel ? CH_ARR_RIGHT : CH_BLANK, PB_UI);
}

void drawmenu_enter(void)
{
    sel = 0;
    REG_DISPCNT = DCNT_MODE0 | DCNT_BG0 | DCNT_BG1 | DCNT_OBJ | DCNT_OBJ_1D;
    obj_hide(&oam_mem[0]);
    obj_hide(&oam_mem[1]);
    draw_items();
}

void drawmenu_frame(int frame)
{
    (void)frame;
    if (key_hit(KEY_UP))   { sel = (sel + N_ITEMS - 1) % N_ITEMS; draw_items(); }
    if (key_hit(KEY_DOWN)) { sel = (sel + 1) % N_ITEMS; draw_items(); }
    if (key_hit(KEY_B) || key_hit(KEY_START)) { app_goto(ST_DRAW); return; }
    if (key_hit(KEY_A)) {
        switch (sel) {
        case 0: app_goto(ST_DRAW); break;
        case 1: print_set_page(draw_canvas(), ST_DRAWMENU); app_goto(ST_PRINT); break;
        case 2: app_goto(ST_SLOTS_SAVE_ART); break;
        case 3: app_goto(ST_SLOTS_LOAD_ART); break;
        case 4:
            g_save.draw_pal = (u8)((g_save.draw_pal + 1) % draw_pal_count());
            save_flush_header();
            draw_items();
            break;
        case 5: draw_clear(); app_goto(ST_DRAW); break;
        case 6: app_goto(ST_TITLE); break;
        }
    }
}
