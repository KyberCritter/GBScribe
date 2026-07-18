// On-screen instructions viewer: pages through the manual text and can print
// it. The same manual_text buffer feeds the screen and the printer, reflowed
// at the printed page width (20 cols), so what you read is what prints.
#include <stdio.h>
#include <string.h>
#include <tonc.h>
#include "app.h"
#include "ui/ui.h"
#include "ui/screen.h"
#include "core/layout.h"
#include "core/manual.h"

#define LINES_PER_PAGE 16

EWRAM_BSS static Layout ilo;
static int page;
static int n_pages;
static AppState back = ST_TITLE;

// Where B exits to (menu or title). Resets to page 1; the current page is
// otherwise preserved across the print detour.
void instructions_set_back(AppState s) { back = s; page = 0; }

static void draw_page(void)
{
    ui_cls();

    ui_text(9, 0, "INSTRUCTIONS", PB_UI);
    char ind[8];
    siprintf(ind, "%d/%d", page + 1, n_pages);
    ui_text(25, 0, ind, PB_UI);

    // 20-col text column centered on the 30-tile screen
    for (int i = 0; i < LINES_PER_PAGE; i++) {
        int ln = page * LINES_PER_PAGE + i;
        if (ln >= ilo.count)
            break;
        char buf[21];
        int len = layout_line_len(&ilo, manual_text, manual_len, ln);
        memcpy(buf, manual_text + ilo.start[ln], len);
        buf[len] = 0;
        ui_text(5, 2 + i, buf, PB_UI);
    }

    ui_text(2, 19, "UP/DN:Page A:Print B:Back", PB_UI);
}

void instructions_enter(void)
{
    obj_hide(&oam_mem[0]);
    // manual lines are authored at <= 20 chars, so wrap mode is moot
    layout_reflow(&ilo, manual_text, manual_len, 20, WRAP_POST_WORD);
    n_pages = (ilo.count + LINES_PER_PAGE - 1) / LINES_PER_PAGE;
    if (page >= n_pages)
        page = n_pages - 1;
    draw_page();
}

void instructions_frame(int frame)
{
    (void)frame;
    if (key_hit(KEY_UP)   && page > 0)           { page--; draw_page(); }
    if (key_hit(KEY_DOWN) && page < n_pages - 1) { page++; draw_page(); }
    if (key_hit(KEY_B) || key_hit(KEY_START)) { app_goto(back); return; }
    if (key_hit(KEY_A)) {
        print_set_text(manual_text, manual_len, ST_INSTRUCTIONS);
        app_goto(ST_PRINT);
    }
}
