#include <tonc.h>
#include "app.h"
#include "ui/editor.h"
#include "ui/screen.h"
#include "ui/draw.h"
#include "storage/save.h"
#include "printer/gbp_link.h"
#include "sound.h"

static AppState state;

// scenes other than ST_DRAW use the standard two-layer display
static void default_dispcnt(void)
{
    REG_DISPCNT = DCNT_MODE0 | DCNT_BG0 | DCNT_BG1 | DCNT_OBJ | DCNT_OBJ_1D;
}

void app_goto(AppState s)
{
    state = s;
    // music must not run while bit-banging the printer (IRQ timing)
    if (s == ST_PRINT)
        sound_pause();
    else
        sound_resume();
    switch (s) {
    case ST_DRAW:           draw_enter();          break;
    case ST_DRAWMENU:       drawmenu_enter();      break;
    default:
        default_dispcnt();
        obj_hide(&oam_mem[1]);      // drawing cursor
        switch (s) {
        case ST_TITLE:          title_enter();         break;
        case ST_EDITOR:         editor_enter();        break;
        case ST_MENU:           menu_enter();          break;
        case ST_SLOTS_SAVE:     slots_enter2(1, 0);    break;
        case ST_SLOTS_LOAD:     slots_enter2(0, 0);    break;
        case ST_SLOTS_SAVE_ART: slots_enter2(1, 1);    break;
        case ST_SLOTS_LOAD_ART: slots_enter2(0, 1);    break;
        case ST_SLOTS_LOAD_ANY: slots_enter2(0, 2);    break;
        case ST_SETTINGS:       settings_enter();      break;
        case ST_PRINT:          print_enter();         break;
        case ST_CREDITS:        credits_enter();       break;
        case ST_DEBUG:          debug_enter();         break;
        default: break;
        }
    }
}

void app_init(void)
{
    save_init();
    gbp_link_init();
    editor_init();
    app_goto(ST_TITLE);
}

void app_frame(int frame)
{
    switch (state) {
    case ST_TITLE:          title_frame(frame);    break;
    case ST_EDITOR:         editor_frame(frame);   break;
    case ST_MENU:           menu_frame(frame);     break;
    case ST_SLOTS_SAVE:
    case ST_SLOTS_LOAD:
    case ST_SLOTS_SAVE_ART:
    case ST_SLOTS_LOAD_ART:
    case ST_SLOTS_LOAD_ANY: slots_frame(frame);    break;
    case ST_SETTINGS:       settings_frame(frame); break;
    case ST_PRINT:          print_frame(frame);    break;
    case ST_DRAW:           draw_frame(frame);     break;
    case ST_DRAWMENU:       drawmenu_frame(frame); break;
    case ST_CREDITS:        credits_frame(frame);  break;
    case ST_DEBUG:          debug_frame(frame);    break;
    }
}
