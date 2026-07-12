// Top-level scene state machine.
#ifndef APP_H
#define APP_H

typedef enum {
    ST_TITLE,
    ST_EDITOR,
    ST_MENU,
    ST_SLOTS_SAVE,
    ST_SLOTS_LOAD,
    ST_SETTINGS,
} AppState;

void app_init(void);
void app_frame(int frame);
void app_goto(AppState s);   // runs the scene's enter() (full redraw)

// scene hooks
void title_enter(void);    void title_frame(int frame);
void menu_enter(void);     void menu_frame(int frame);
void slots_enter(int save_mode); void slots_frame(int frame);
void settings_enter(void); void settings_frame(int frame);

#endif
