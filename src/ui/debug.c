// Secret debug menu (Konami code on the title screen): print book snippets.
#include <tonc.h>
#include "app.h"
#include "ui/ui.h"
#include "ui/screen.h"
#include "core/charset.h"
#include "core/snippets.h"

static int sel;

static void draw_items(void)
{
    ui_cls();
    obj_hide(&oam_mem[0]);
    ui_fill(3, 4, 24, 6 + SNIPPET_COUNT, 3);
    ui_frame(3, 4, 24, 6 + SNIPPET_COUNT);
    ui_text(9, 5, "- DEBUG MENU -", PB_UI);
    for (int i = 0; i < SNIPPET_COUNT; i++) {
        ui_text(7, 7 + i, snippets[i].name, PB_UI);
        map_put(text_map(), 5, 7 + i, i == sel ? CH_ARR_RIGHT : CH_BLANK, PB_UI);
    }
    ui_text(5, 8 + SNIPPET_COUNT, "A: Print  B: Back", PB_UI);
}

void debug_enter(void)
{
    sel = 0;
    draw_items();
}

void debug_frame(int frame)
{
    (void)frame;
    if (key_hit(KEY_UP))   { sel = (sel + SNIPPET_COUNT - 1) % SNIPPET_COUNT; draw_items(); }
    if (key_hit(KEY_DOWN)) { sel = (sel + 1) % SNIPPET_COUNT; draw_items(); }
    if (key_hit(KEY_B) || key_hit(KEY_START)) { app_goto(ST_TITLE); return; }
    if (key_hit(KEY_A)) {
        print_set_text(snippets[sel].text, snippets[sel].len, ST_DEBUG);
        app_goto(ST_PRINT);
    }
}
