#include <string.h>
#include <stdio.h>
#include "editor.h"
#include "screen.h"
#include "keyboard.h"
#include "ui.h"
#include "app.h"
#include "core/charset.h"
#include "core/document.h"
#include "core/layout.h"
#include "storage/storage.h"
#include "storage/save.h"
#include "sound.h"

EWRAM_BSS static Document doc;
EWRAM_BSS static Layout lo;

static int top_line;                          // first visible layout line
static int cur_layer, cur_key;
static int shift_from = -1;   // layer to return to after one shifted keypress

// caret cell on screen (page-relative), from last render
static int caret_cx, caret_cy;

static void reflow(void)
{
    WrapMode m = g_save.wrap_mode ? WRAP_MID_WORD : WRAP_POST_WORD;
    layout_reflow(&lo, doc.text, doc.len, PAGE_W, m);
}

static void scroll_to_caret(void)
{
    int ln = layout_caret_line(&lo, doc.caret);
    if (ln < top_line)
        top_line = ln;
    if (ln >= top_line + PAGE_ROWS)
        top_line = ln - PAGE_ROWS + 1;
}

static void render_page(void)
{
    u16 *tm = text_map();
    for (int row = 0; row < PAGE_ROWS; row++) {
        int ln = top_line + row;
        int llen = (ln < lo.count) ? layout_line_len(&lo, doc.text, doc.len, ln) : 0;
        if (llen > PAGE_W) llen = PAGE_W;   // overflow spaces are clipped
        int s = (ln < lo.count) ? lo.start[ln] : 0;
        for (int col = 0; col < PAGE_W; col++) {
            u8 c = (col < llen) ? (u8)doc.text[s + col] : CH_BLANK;
            map_put(tm, PAGE_X0 + col, row, c, PB_INK);
        }
    }

    int ln = layout_caret_line(&lo, doc.caret);
    int cx = doc.caret - lo.start[ln];
    if (cx > PAGE_W - 1) cx = PAGE_W - 1;
    caret_cx = cx;
    caret_cy = ln - top_line;

    // printed-page indicator in the left gutter (18 lines per page)
    char buf[8];
    siprintf(buf, "p.%-2d", ln / 18 + 1);
    ui_text(0, 11, buf, PB_UI);
    siprintf(buf, "of%-2d", (lo.count + 17) / 18);
    ui_text(0, 12, buf, PB_UI);
}

static void update_caret_sprite(int frame)
{
    OBJ_ATTR *caret = &oam_mem[0];
    bool on_screen = caret_cy >= 0 && caret_cy < PAGE_ROWS;
    if (on_screen && ((frame >> 5) & 1))
        obj_set_attr(caret,
                     (caret_cy * 8) & 0x00FF,
                     ((PAGE_X0 + caret_cx) * 8) & 0x01FF,
                     0);
    else
        obj_hide(caret);
}

static void after_edit(void)
{
    reflow();
    scroll_to_caret();
    render_page();
}

static void type_code(int code)
{
    if (doc_insert(&doc, (char)code)) {
        sfx_click();
        after_edit();
    }
}

static void caret_vertical(int dir)
{
    int ln = layout_caret_line(&lo, doc.caret);
    int col = doc.caret - lo.start[ln];
    int to = ln + dir;
    if (to < 0 || to >= lo.count)
        return;
    int llen = layout_line_len(&lo, doc.text, doc.len, to);
    if (col > llen) col = llen;
    doc_caret_set(&doc, lo.start[to] + col);
    scroll_to_caret();
    render_page();
}

static void caret_horizontal(int dir)
{
    doc_caret_set(&doc, doc.caret + dir);
    scroll_to_caret();
    render_page();
}

static void switch_layer(int to)
{
    cur_key = kb_nearest(cur_layer, cur_key, to);
    cur_layer = to;
    kb_render(cur_layer, cur_key);
}

static void activate_key(const Key *k)
{
    switch (k->action) {
    case KA_LAYER_LOWER: case KA_LAYER_UPPER:
    case KA_LAYER_SYM:   case KA_LAYER_SHAPE:
        shift_from = -1;                 // explicit switch cancels a pending shift
        switch_layer(k->action - KA_LAYER_LOWER);
        break;
    case KA_SHIFT:
        if (shift_from >= 0) {           // second press cancels
            int to = shift_from;
            shift_from = -1;
            switch_layer(to);
        } else {
            shift_from = cur_layer;
            switch_layer(cur_layer == 1 ? 0 : 1);
        }
        break;
    case KA_ENTER:
        type_code(CH_NEWLINE);
        break;
    case KA_BACKSPACE:
        if (doc_backspace(&doc))
            after_edit();
        break;
    default:
        type_code(k->action);
        if (shift_from >= 0) {           // shift is one-shot: pop back
            int to = shift_from;
            shift_from = -1;
            switch_layer(to);
        }
    }
}

// D-pad/A/B auto-repeat: initial delay 16 frames, then every 4.
static u16 rep_keys, rep_timer;

static u16 repeat_hits(void)
{
    const u16 mask = KEY_LEFT | KEY_RIGHT | KEY_UP | KEY_DOWN | KEY_A | KEY_B;
    u16 held = key_curr_state() & mask;
    u16 hits = key_hit(0xFFFF);

    if (held && held == rep_keys) {
        if (++rep_timer >= 16) {
            rep_timer = 12;  // 4-frame repeat period
            hits |= held;
        }
    } else {
        rep_keys = held;
        rep_timer = 0;
    }
    return hits;
}

static void draw_chrome(void)
{
    u16 *cm = chrome_map();
    u16 *tm = text_map();
    for (int y = 0; y < PAGE_ROWS; y++)
        for (int x = 0; x < SCREEN_TW; x++) {
            bool page = (x >= PAGE_X0 && x < PAGE_X0 + PAGE_W);
            map_put(cm, x, y, page ? SOLID(1) : SOLID(3), PB_SOLID);
            map_put(tm, x, y, CH_BLANK, PB_INK);   // clear stale scene glyphs
        }
    for (int x = 0; x < SCREEN_TW; x++) {
        map_put(cm, x, PAGE_ROWS, SOLID(5), PB_SOLID);
        map_put(tm, x, PAGE_ROWS, CH_BLANK, PB_INK);
    }
}

void editor_init(void)
{
    doc_init(&doc);
    top_line = 0;
    cur_layer = 0;
    cur_key = 0;
    shift_from = -1;
}

void editor_enter(void)
{
    draw_chrome();
    reflow();
    scroll_to_caret();
    render_page();
    kb_render(cur_layer, cur_key);
}

void editor_new(void)
{
    doc_init(&doc);
    top_line = 0;
}

void editor_reloaded(void)
{
    top_line = 0;
}

Document *editor_doc(void)
{
    return &doc;
}

void editor_frame(int frame)
{
    u16 hits = repeat_hits();
    bool caret_mod = (key_curr_state() & KEY_L) != 0;

    int dx = 0, dy = 0;
    if (hits & KEY_LEFT)  dx = -1;
    if (hits & KEY_RIGHT) dx = +1;
    if (hits & KEY_UP)    dy = -1;
    if (hits & KEY_DOWN)  dy = +1;

    if (caret_mod) {
        if (dx) caret_horizontal(dx);
        if (dy) caret_vertical(dy);
    } else if (dx || dy) {
        int nk = kb_move(cur_layer, cur_key, dx, dy);
        if (nk != cur_key) {
            cur_key = nk;
            kb_render(cur_layer, cur_key);
        }
    }

    if (hits & KEY_A)
        activate_key(kb_key(cur_layer, cur_key));
    if (hits & KEY_B) {
        if (doc_backspace(&doc))
            after_edit();
    }
    if (key_hit(KEY_SELECT)) {
        shift_from = -1;
        switch_layer((cur_layer + 1) % KB_LAYER_COUNT);
    }
    if (key_hit(KEY_R))
        type_code(' ');
    if (key_hit(KEY_START)) {
        app_goto(ST_MENU);
        return;
    }

    update_caret_sprite(frame);
}
