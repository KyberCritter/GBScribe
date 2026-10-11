#include <stdio.h>
#include <tonc.h>
#include "app.h"
#include "ui/ui.h"
#include "ui/screen.h"
#include "ui/editor.h"
#include "ui/draw.h"
#include "core/charset.h"
#include <advanceprint/advanceprint_link.h>
#include "storage/storage.h"
#include "gfx/testpage.h"

static bool printer_ok;
static bool choosing;        // Start pressed: new-doc / new-drawing / load chooser
static int  choice;

// Konami code -> secret debug menu (snippet prints)
static const u16 konami[] = {
    KEY_UP, KEY_UP, KEY_DOWN, KEY_DOWN,
    KEY_LEFT, KEY_RIGHT, KEY_LEFT, KEY_RIGHT,
    KEY_B, KEY_A,
};
#define KONAMI_LEN 10
static int konami_idx;

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

#define N_CHOICES 6

static void draw_chooser(void)
{
    static const char *opts[] = {
        "New text document", "New drawing", "Load a file",
        "Print test page", "Instructions", "Credits",
    };
    ui_fill(5, 9, 21, 9, 3);
    ui_frame(5, 9, 21, 9);
    // blank the text layer inside (BG0 sits above the chrome fill)
    for (int y = 10; y <= 16; y++)
        for (int x = 6; x <= 24; x++)
            map_put(text_map(), x, y, CH_BLANK, PB_UI);
    for (int i = 0; i < N_CHOICES; i++) {
        ui_text(8, 11 + i, opts[i], PB_UI);
        map_put(text_map(), 7, 11 + i, i == choice ? CH_ARR_RIGHT : CH_BLANK, PB_UI);
    }
}

void title_enter(void)
{
    ui_cls();
    obj_hide(&oam_mem[0]);
    choosing = false;
    choice = 0;
    konami_idx = 0;

    ui_fill(6, 5, 18, 5, 3);
    ui_frame(6, 5, 18, 5);
    ui_logo(9, 6);
    ui_text(12, 7, "GBScribe", PB_UI);

    // screen is 30 columns: center = (30 - len) / 2
    ui_text(5, 12, "Write \x0C Draw \x0C Print", PB_UI);   // 20 chars
    ui_text(9, 15, "PRESS START", PB_UI);                  // 11 chars

    ui_text(25, 19, "v0.3", PB_UI);   // latest release tag; bump on release

    char line[24];
    siprintf(line, "Save: %s", storage_name());
    ui_text(4, 19, line, PB_UI);

    printer_ok = false;
    draw_printer_status();
}

void title_frame(int frame)
{
    if (choosing) {
        if (key_hit(KEY_UP))   { choice = (choice + N_CHOICES - 1) % N_CHOICES; draw_chooser(); }
        if (key_hit(KEY_DOWN)) { choice = (choice + 1) % N_CHOICES; draw_chooser(); }
        if (key_hit(KEY_B)) { title_enter(); return; }
        if (key_hit(KEY_A) || key_hit(KEY_START)) {
            switch (choice) {
            case 0: editor_new(); app_goto(ST_EDITOR); break;
            case 1: draw_clear(); app_goto(ST_DRAW);   break;
            case 2: app_goto(ST_SLOTS_LOAD_ANY);       break;
            case 3:
                print_set_page(testpage_page, TESTPAGE_ROWS, ST_TITLE);
                app_goto(ST_PRINT);
                break;
            case 4: instructions_set_back(ST_TITLE); app_goto(ST_INSTRUCTIONS); break;
            case 5: credits_set_back(ST_TITLE); app_goto(ST_CREDITS); break;
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
        bool ok = ap_link_detect(NULL);
        if (ok != printer_ok) {
            printer_ok = ok;
            draw_printer_status();
        }
    }

    // Konami code: exact key per step; the final A is consumed before it can
    // open the chooser. Any wrong key restarts the sequence.
    u16 hit = key_hit(KEY_FULL);
    if (hit) {
        if (hit == konami[konami_idx]) {
            if (++konami_idx == KONAMI_LEN) {
                app_goto(ST_DEBUG);
                return;
            }
            if (hit & (KEY_START | KEY_A))
                return;        // partial match: don't also open the chooser
        } else {
            konami_idx = (hit == konami[0]) ? 1 : 0;
        }
    }

    if (key_hit(KEY_START | KEY_A)) {
        choosing = true;
        choice = 0;
        draw_chooser();
    }
}
