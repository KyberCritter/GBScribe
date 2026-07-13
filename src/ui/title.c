#include <stdio.h>
#include <tonc.h>
#include "app.h"
#include "ui/ui.h"
#include "ui/screen.h"
#include "core/charset.h"
#include "printer/gbp_link.h"
#include "storage/storage.h"

static bool printer_ok;

static void draw_printer_status(void)
{
    if (printer_ok) {
        map_put(text_map(), 2, 18, CH_CIRC_FILL, PB_UI);
        ui_text(4, 18, "printer: OK          ", PB_UI);
    } else {
        map_put(text_map(), 2, 18, CH_CIRC_HOLLOW, PB_UI);
        ui_text(4, 18, "printer: disconnected", PB_UI);
    }
}

void title_enter(void)
{
    ui_cls();
    obj_hide(&oam_mem[0]);

    ui_fill(6, 5, 18, 5, 3);
    ui_frame(6, 5, 18, 5);
    ui_text(11, 7, "GBScribe", PB_UI);

    // screen is 30 columns: center = (30 - len) / 2
    ui_text(5, 12, "write \x0C draw \x0C print", PB_UI);   // 20 chars
    ui_text(9, 15, "PRESS START", PB_UI);                  // 11 chars

    ui_text(25, 19, "v1.2", PB_UI);

    char line[24];
    siprintf(line, "save: %s", storage_name());
    ui_text(4, 19, line, PB_UI);

    printer_ok = false;
    draw_printer_status();
}

void title_frame(int frame)
{
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

    if (key_hit(KEY_START | KEY_A))
        app_goto(ST_EDITOR);
}
