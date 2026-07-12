#include <string.h>
#include <stdio.h>
#include <tonc.h>
#include "app.h"
#include "ui/ui.h"
#include "ui/screen.h"
#include "ui/editor.h"
#include "core/charset.h"
#include "storage/storage.h"
#include "storage/save.h"

static int sel;
static int saving;          // 1 = save mode, 0 = load mode
static int msg_timer;       // frames left to show the status message

static void draw(void)
{
    ui_cls();
    ui_text(6, 1, saving ? "SAVE TO SLOT" : "LOAD FROM SLOT", PB_UI);

    for (int i = 0; i < SLOT_COUNT; i++) {
        char line[28];
        const SlotDir *e = &g_save.dir[i];
        if (e->type == SLOT_DOC) {
            char name[SLOT_NAME_LEN + 1];
            memcpy(name, e->name, SLOT_NAME_LEN);
            name[SLOT_NAME_LEN] = 0;
            if (!name[0])
                strcpy(name, "(untitled)");
            siprintf(line, "%d. %-14s doc", i + 1, name);
        } else if (e->type == SLOT_DRAWING) {
            siprintf(line, "%d. %-14s art", i + 1, "(drawing)");
        } else {
            siprintf(line, "%d. --empty--", i + 1);
        }
        ui_text(5, 3 + i * 2, line, PB_UI);
        map_put(text_map(), 3, 3 + i * 2, i == sel ? CH_ARR_RIGHT : CH_BLANK, PB_UI);
    }
    ui_text(3, 19, "A:ok  B:back", PB_UI);
}

static void show_msg(const char *m)
{
    ui_fill(5, 9, 20, 3, 3);
    ui_frame(5, 9, 20, 3);
    int x = 5 + (20 - (int)strlen(m)) / 2;
    ui_text(x, 10, m, PB_UI);
    msg_timer = 60;
}

void slots_enter(int save_mode)
{
    sel = 0;
    saving = save_mode;
    msg_timer = 0;
    obj_hide(&oam_mem[0]);
    draw();
}

void slots_frame(int frame)
{
    (void)frame;
    if (msg_timer > 0) {
        if (--msg_timer == 0) {
            if (saving)
                app_goto(ST_EDITOR);   // after a save confirmation, back to writing
            else
                draw();
        }
        return;
    }

    if (key_hit(KEY_UP))   { sel = (sel + SLOT_COUNT - 1) % SLOT_COUNT; draw(); }
    if (key_hit(KEY_DOWN)) { sel = (sel + 1) % SLOT_COUNT; draw(); }
    if (key_hit(KEY_B))    { app_goto(ST_MENU); return; }
    if (key_hit(KEY_A)) {
        if (saving) {
            show_msg(save_doc(sel, editor_doc()) ? "SAVED" : "SAVE FAILED!");
        } else {
            if (g_save.dir[sel].type == SLOT_DOC) {
                load_doc(sel, editor_doc());
                editor_reloaded();
                app_goto(ST_EDITOR);
            } else {
                show_msg("empty slot");
            }
        }
    }
}
