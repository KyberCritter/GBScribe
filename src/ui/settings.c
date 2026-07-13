#include <tonc.h>
#include "app.h"
#include "ui/ui.h"
#include "ui/screen.h"
#include "core/charset.h"
#include "storage/storage.h"
#include "storage/save.h"

static int sel;
#define N_OPTS 1

static void draw(void)
{
    // frame spans cols 3..26; the wrap value ends at col 13+12-1 = 24, inside.
    ui_cls();
    ui_fill(3, 4, 24, 9, 3);
    ui_frame(3, 4, 24, 9);
    ui_text(9, 5, "- SETTINGS -", PB_UI);

    ui_text(5, 7, "Wrap:", PB_UI);
    ui_text(13, 7, g_save.wrap_mode ? "\x03 mid-word \x04" : "\x03 word     \x04", PB_UI);

    for (int i = 0; i < N_OPTS; i++)
        map_put(text_map(), 4, 7 + i * 2, i == sel ? CH_ARR_RIGHT : CH_BLANK, PB_UI);

    ui_text(5, 11, "B: back (saves)", PB_UI);
}

void settings_enter(void)
{
    sel = 0;
    obj_hide(&oam_mem[0]);
    draw();
}

void settings_frame(int frame)
{
    (void)frame;
    if (key_hit(KEY_UP))   { sel = (sel + N_OPTS - 1) % N_OPTS; draw(); }
    if (key_hit(KEY_DOWN)) { sel = (sel + 1) % N_OPTS; draw(); }
    if (key_hit(KEY_LEFT) || key_hit(KEY_RIGHT)) {
        if (sel == 0) g_save.wrap_mode ^= 1;
        draw();
    }
    if (key_hit(KEY_B) || key_hit(KEY_START)) {
        save_flush_header();
        app_goto(ST_MENU);
    }
}
