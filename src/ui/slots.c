#include <string.h>
#include <stdio.h>
#include <tonc.h>
#include "app.h"
#include "ui/ui.h"
#include "ui/screen.h"
#include "ui/editor.h"
#include "ui/draw.h"
#include "core/charset.h"
#include "storage/storage.h"
#include "storage/save.h"

static int sel;
static int saving;          // 1 = save mode, 0 = load mode
static int art;             // 1 = drawing slots flow, 0 = document flow
static int msg_timer;       // frames left to show the status message

static void draw_list(void)
{
    ui_cls();
    if (art)
        ui_text(6, 1, saving ? "SAVE DRAWING TO" : "LOAD DRAWING FROM", PB_UI);
    else
        ui_text(6, 1, saving ? "SAVE TO SLOT" : "LOAD FROM SLOT", PB_UI);

    for (int i = 0; i < SLOT_COUNT; i++) {
        char line[28];
        const SlotDir *e = &g_save.dir[i];
        if (e->type == SLOT_DOC || e->type == SLOT_DRAWING) {
            char name[SLOT_NAME_LEN + 1];
            memcpy(name, e->name, SLOT_NAME_LEN);
            name[SLOT_NAME_LEN] = 0;
            if (!name[0])
                strcpy(name, "(untitled)");
            siprintf(line, "%d. %-14s %s", i + 1, name,
                     e->type == SLOT_DOC ? "doc" : "art");
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

void slots_enter2(int save_mode, int art_mode)
{
    sel = 0;
    saving = save_mode;
    art = art_mode;
    msg_timer = 0;
    obj_hide(&oam_mem[0]);
    obj_hide(&oam_mem[1]);
    draw_list();
}

void slots_frame(int frame)
{
    (void)frame;
    if (msg_timer > 0) {
        if (--msg_timer == 0) {
            if (saving)
                app_goto(art ? ST_DRAW : ST_EDITOR);
            else
                draw_list();
        }
        return;
    }

    if (key_hit(KEY_UP))   { sel = (sel + SLOT_COUNT - 1) % SLOT_COUNT; draw_list(); }
    if (key_hit(KEY_DOWN)) { sel = (sel + 1) % SLOT_COUNT; draw_list(); }
    if (key_hit(KEY_B))    { app_goto(art ? ST_DRAWMENU : ST_MENU); return; }
    if (key_hit(KEY_A)) {
        if (saving && art) {
            show_msg(save_drawing(sel, draw_canvas(), "drawing")
                     ? "SAVED" : "SAVE FAILED!");
        } else if (saving) {
            show_msg(save_doc(sel, editor_doc()) ? "SAVED" : "SAVE FAILED!");
        } else if (art) {
            if (g_save.dir[sel].type == SLOT_DRAWING) {
                load_drawing(sel, draw_canvas());
                draw_canvas_reloaded();
                app_goto(ST_DRAW);
            } else {
                show_msg(g_save.dir[sel].type == SLOT_DOC ? "that's a doc" : "empty slot");
            }
        } else {
            if (g_save.dir[sel].type == SLOT_DOC) {
                load_doc(sel, editor_doc());
                editor_reloaded();
                app_goto(ST_EDITOR);
            } else {
                show_msg(g_save.dir[sel].type == SLOT_DRAWING ? "that's art" : "empty slot");
            }
        }
    }
}
