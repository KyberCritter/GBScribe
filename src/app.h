// Top-level scene state machine.
#ifndef APP_H
#define APP_H

typedef enum {
    ST_TITLE,
    ST_EDITOR,
    ST_MENU,
    ST_SLOTS_SAVE,
    ST_SLOTS_LOAD,
    ST_SLOTS_SAVE_ART,
    ST_SLOTS_LOAD_ART,
    ST_SLOTS_LOAD_ANY,   // title-screen "Load a file": docs and drawings both
    ST_SETTINGS,
    ST_PRINT,
    ST_DRAW,
    ST_DRAWMENU,
    ST_CREDITS,
} AppState;

void app_init(void);
void app_frame(int frame);
void app_goto(AppState s);   // runs the scene's enter() (full redraw)

// scene hooks
void title_enter(void);    void title_frame(int frame);
void menu_enter(void);     void menu_frame(int frame);
void slots_enter2(int save_mode, int art_mode); void slots_frame(int frame);
void settings_enter(void); void settings_frame(int frame);
void print_enter(void);    void print_frame(int frame);
void drawmenu_enter(void); void drawmenu_frame(int frame);
void credits_enter(void);  void credits_frame(int frame);
// Where B exits the credits scene to (persists across the QR print detour).
void credits_set_back(AppState s);

// Print a raw 5,760-byte page (canvas, QR) instead of the document; `back` is
// where B returns from the print scene. One-shot: set right before ST_PRINT.
void print_set_page(const void *page5760, AppState back);

// Print arbitrary text (e.g. the manual) instead of the editor document.
// One-shot, same contract as print_set_page.
void print_set_text(const char *text, int len, AppState back);

#endif
