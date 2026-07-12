#include <tonc.h>
#include "app.h"
#include "ui/ui.h"
#include "ui/screen.h"
#include "ui/editor.h"
#include "core/charset.h"

static const char *items[] = {
    "Resume",
    "Save document",
    "Load document",
    "New document",
    "Settings",
};
#define N_ITEMS 5

static int sel;

static void draw(void)
{
    ui_cls();
    ui_fill(7, 3, 16, 13, 3);
    ui_frame(7, 3, 16, 13);
    ui_text(11, 4, "- MENU -", PB_UI);
    for (int i = 0; i < N_ITEMS; i++) {
        ui_text(9, 6 + i * 2, items[i], PB_UI);
        map_put(text_map(), 8, 6 + i * 2, i == sel ? CH_ARR_RIGHT : CH_BLANK, PB_UI);
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
        case 1: app_goto(ST_SLOTS_SAVE); break;
        case 2: app_goto(ST_SLOTS_LOAD); break;
        case 3: editor_new(); app_goto(ST_EDITOR); break;
        case 4: app_goto(ST_SETTINGS); break;
        }
    }
}
