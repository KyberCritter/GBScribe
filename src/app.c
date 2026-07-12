#include <tonc.h>
#include "app.h"
#include "ui/editor.h"
#include "storage/save.h"

static AppState state;

void app_goto(AppState s)
{
    state = s;
    switch (s) {
    case ST_TITLE:      title_enter();     break;
    case ST_EDITOR:     editor_enter();    break;
    case ST_MENU:       menu_enter();      break;
    case ST_SLOTS_SAVE: slots_enter(1);    break;
    case ST_SLOTS_LOAD: slots_enter(0);    break;
    case ST_SETTINGS:   settings_enter();  break;
    }
}

void app_init(void)
{
    save_init();
    editor_init();
    app_goto(ST_TITLE);
}

void app_frame(int frame)
{
    switch (state) {
    case ST_TITLE:      title_frame(frame);    break;
    case ST_EDITOR:     editor_frame(frame);   break;
    case ST_MENU:       menu_frame(frame);     break;
    case ST_SLOTS_SAVE:
    case ST_SLOTS_LOAD: slots_frame(frame);    break;
    case ST_SETTINGS:   settings_frame(frame); break;
    }
}
