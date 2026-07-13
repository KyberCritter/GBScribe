#include <tonc.h>
#include "app.h"
#include "ui/ui.h"
#include "ui/screen.h"
#include "ui/editor.h"
#include "core/charset.h"

static const char *items[] = {
    "Resume",
    "Print document",
    "Save document",
    "Load document",
    "New document",
    "Drawing mode",
    "Settings",
    "About",
};
#define N_ITEMS 8

static int sel;

static void draw(void)
{
    // frame spans cols 6..24, rows 1..19; 8 items at rows 4..18 stay inside,
    // and the longest item "Print document" (14) ends at col 21.
    ui_cls();
    ui_fill(6, 1, 19, 19, 3);
    ui_frame(6, 1, 19, 19);
    ui_text(11, 2, "- MENU -", PB_UI);
    for (int i = 0; i < N_ITEMS; i++) {
        ui_text(8, 4 + i * 2, items[i], PB_UI);
        map_put(text_map(), 7, 4 + i * 2, i == sel ? CH_ARR_RIGHT : CH_BLANK, PB_UI);
    }
}

void menu_enter(void)
{
    sel = 0;
    obj_hide(&oam_mem[0]);
    draw();
}

void menu_frame(int frame)
{
    (void)frame;
    if (key_hit(KEY_UP))   { sel = (sel + N_ITEMS - 1) % N_ITEMS; draw(); }
    if (key_hit(KEY_DOWN)) { sel = (sel + 1) % N_ITEMS; draw(); }
    if (key_hit(KEY_B) || key_hit(KEY_START)) { app_goto(ST_EDITOR); return; }
    if (key_hit(KEY_A)) {
        switch (sel) {
        case 0: app_goto(ST_EDITOR); break;
        case 1: app_goto(ST_PRINT); break;
        case 2: app_goto(ST_SLOTS_SAVE); break;
        case 3: app_goto(ST_SLOTS_LOAD); break;
        case 4: editor_new(); app_goto(ST_EDITOR); break;
        case 5: app_goto(ST_DRAW); break;
        case 6: app_goto(ST_SETTINGS); break;
        case 7: app_goto(ST_CREDITS); break;
        }
    }
}
