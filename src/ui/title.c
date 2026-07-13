#include <stdio.h>
#include <tonc.h>
#include "app.h"
#include "ui/ui.h"
#include "ui/screen.h"
#include "ui/editor.h"
#include "ui/draw.h"
#include "core/charset.h"
#include "printer/gbp_link.h"
#include "storage/storage.h"

static bool printer_ok;
static bool choosing;        // Start pressed: new-doc / new-drawing / load chooser
static int  choice;

static void draw_printer_status(void)
{
    if (printer_ok) {
        map_put(text_map(), 2, 18, CH_CIRC_FILL, PB_UI);
        ui_text(4, 18, "Printer: OK          ", PB_UI);
    } else {
        map_put(text_map(), 2, 18, CH_CIRC_HOLLOW, PB_UI);
        ui_text(4, 18, "Printer: disconnected", PB_UI);
    }
}

static void draw_chooser(void)
{
    static const char *opts[] = { "New text document", "New drawing", "Load a file" };
    ui_fill(5, 10, 21, 8, 3);
    ui_frame(5, 10, 21, 8);
    for (int i = 0; i < 3; i++) {
        ui_text(8, 12 + i * 2, opts[i], PB_UI);
        map_put(text_map(), 7, 12 + i * 2, i == choice ? CH_ARR_RIGHT : CH_BLANK, PB_UI);
    }
}

void title_enter(void)
{
    ui_cls();
    obj_hide(&oam_mem[0]);
    choosing = false;
    choice = 0;

    ui_fill(6, 5, 18, 5, 3);
    ui_frame(6, 5, 18, 5);
    ui_logo(9, 6);
    ui_text(12, 7, "GBScribe", PB_UI);

    // screen is 30 columns: center = (30 - len) / 2
    ui_text(5, 12, "Write \x0C Draw \x0C Print", PB_UI);   // 20 chars
    ui_text(9, 15, "PRESS START", PB_UI);                  // 11 chars

    ui_text(25, 19, "v1.4", PB_UI);

    char line[24];
    siprintf(line, "Save: %s", storage_name());
    ui_text(4, 19, line, PB_UI);

    printer_ok = false;
    draw_printer_status();
}

void title_frame(int frame)
{
    if (choosing) {
        if (key_hit(KEY_UP))   { choice = (choice + 2) % 3; draw_chooser(); }
        if (key_hit(KEY_DOWN)) { choice = (choice + 1) % 3; draw_chooser(); }
        if (key_hit(KEY_B)) { title_enter(); return; }
        if (key_hit(KEY_A) || key_hit(KEY_START)) {
            switch (choice) {
            case 0: editor_new(); app_goto(ST_EDITOR); break;
            case 1: draw_clear(); app_goto(ST_DRAW);   break;
            case 2: app_goto(ST_SLOTS_LOAD_ANY);       break;
            }
        }
        return;
    }

    // blink PRESS START
    if ((frame & 63) == 0)
        ui_text(9, 15, "PRESS START", PB_UI);
    else if ((frame & 63) == 40)
        ui_text(9, 15, "           ", PB_UI);

    // probe for the printer once a second (bare INQUIRY, ~10ms, no paper)
    if ((frame % 60) == 30) {
        bool ok = gbp_link_detect(NULL);
        if (ok != printer_ok) {
            printer_ok = ok;
            draw_printer_status();
        }
    }

    if (key_hit(KEY_START | KEY_A)) {
        choosing = true;
        choice = 0;
        draw_chooser();
    }
}
