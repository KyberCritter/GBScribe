#include <tonc.h>
#include "app.h"
#include "ui/ui.h"
#include "ui/screen.h"
#include "core/charset.h"

void title_enter(void)
{
    ui_cls();
    obj_hide(&oam_mem[0]);

    ui_fill(6, 5, 18, 5, 3);
    ui_frame(6, 5, 18, 5);
    ui_text(11, 7, "GBScribe", PB_UI);

    ui_text(7, 12, "write \x0C draw \x0C print", PB_UI);
    ui_text(9, 15, "PRESS START", PB_UI);

    // printer status placeholder — the real INQUIRY probe arrives with M5
    ui_text(2, 18, "\x08 printer: ---", PB_UI);
}

void title_frame(int frame)
{
    // blink PRESS START
    if ((frame & 63) == 0)
        ui_text(9, 15, "PRESS START", PB_UI);
    else if ((frame & 63) == 40)
        ui_text(9, 15, "           ", PB_UI);

    if (key_hit(KEY_START | KEY_A))
        app_goto(ST_EDITOR);
}
