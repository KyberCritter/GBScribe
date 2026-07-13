#include <string.h>
#include "draw.h"
#include "screen.h"
#include "ui.h"
#include "app.h"
#include "core/charset.h"
#include "storage/storage.h"
#include "storage/save.h"

// ---- canvas geometry -------------------------------------------------------
#define CV_W 160
#define CV_H 144
#define CV_TW 20                      // tiles
#define CV_TH 18
#define CV_TILES (CV_TW * CV_TH)      // 360
#define CV_BYTES (CV_TILES * 16)      // 5760, GB 2bpp

// screen placement: centered (40px left, 8px top)
#define CV_X0 40
#define CV_Y0 8

// video resources (all unused by the text scenes)
#define CV_CBB 1
#define CV_SBB 29
#define PB_CANVAS 4                   // BG palette bank for the canvas shades

EWRAM_BSS static u8 canvas[CV_BYTES];        // master (2bpp GB tiles)
EWRAM_BSS static u8 snapshot[CV_BYTES];      // tool preview restore buffer
static u32 dirty[(CV_TILES + 31) / 32];

// ---- tool state -------------------------------------------------------------
enum { TOOL_PENCIL, TOOL_ERASER, TOOL_LINE, TOOL_RECT, TOOL_STAMP, TOOL_COUNT };
static const u8 tool_icons[TOOL_COUNT] = {
    CH_ICON_PENCIL, CH_ICON_ERASER, CH_ICON_LINE, CH_ICON_RECT, CH_ICON_STAMP,
};

static int cur_tool;
static int cur_color = 3;             // 0 = paper .. 3 = darkest
static int cur_stamp;
static int cx = CV_W / 2, cy = CV_H / 2;   // cursor pixel
static bool anchored;                 // line/rect first point set
static int ax, ay;
static int held_frames;

// ---- display palettes (index 0..3 = light..dark) ----------------------------
#define C15(r, g, b) ((u16)((r) | ((g) << 5) | ((b) << 10)))
typedef struct { const char *name; u16 c[4]; } DrawPal;
static const DrawPal pals[] = {
    { "paper", { C15(31,31,30), C15(21,21,21), C15(10,10,11), C15(2,2,3) } },
    { "gb",    { C15(19,23,1),  C15(13,17,3),  C15(6,11,3),   C15(1,7,3) } },
    { "amber", { C15(31,27,16), C15(28,19,6),  C15(19,10,2),  C15(8,3,0) } },
    { "ice",   { C15(28,30,31), C15(17,22,28), C15(8,12,20),  C15(2,3,8) } },
};
#define N_PALS ((int)(sizeof pals / sizeof *pals))

// 4bpp pixel value 0 is transparent on GBA BGs, so canvas pixels are stored in
// VRAM as palette indices 1..4 (shade = value-1); every canvas pixel is opaque.
void draw_apply_palette(void)
{
    int p = g_save.draw_pal % N_PALS;
    for (int i = 0; i < 4; i++)
        pal_bg_mem[16 * PB_CANVAS + 1 + i] = pals[p].c[i];
}

int draw_pal_count(void) { return N_PALS; }
const char *draw_pal_name(int i) { return pals[i % N_PALS].name; }

// ---- pixel access -----------------------------------------------------------
static inline int px_get(int x, int y)
{
    const u8 *t = canvas + (((y >> 3) * CV_TW + (x >> 3)) << 4) + ((y & 7) << 1);
    int bit = 7 - (x & 7);
    return ((t[0] >> bit) & 1) | (((t[1] >> bit) & 1) << 1);
}

static inline void px_set(int x, int y, int c)
{
    if ((unsigned)x >= CV_W || (unsigned)y >= CV_H)
        return;
    int ti = (y >> 3) * CV_TW + (x >> 3);
    u8 *t = canvas + (ti << 4) + ((y & 7) << 1);
    int bit = 7 - (x & 7);
    u8 m = (u8)(1 << bit);
    t[0] = (c & 1) ? (t[0] | m) : (t[0] & ~m);
    t[1] = (c & 2) ? (t[1] | m) : (t[1] & ~m);
    dirty[ti >> 5] |= 1u << (ti & 31);
}

static void mark_all_dirty(void)
{
    memset(dirty, 0xFF, sizeof dirty);
}

// convert one 2bpp master tile to an opaque 4bpp VRAM tile (values 1..4)
static void push_tile(int ti)
{
    const u8 *src = canvas + (ti << 4);
    u32 *dst = (u32 *)&tile_mem[CV_CBB][ti];
    for (int y = 0; y < 8; y++) {
        u8 p0 = src[y * 2], p1 = src[y * 2 + 1];
        u32 row = 0;
        for (int x = 0; x < 8; x++) {
            int bit = 7 - x;
            u32 c = ((p0 >> bit) & 1) | (((p1 >> bit) & 1) << 1);
            row |= (c + 1) << (x * 4);
        }
        dst[y] = row;
    }
}

static void flush_dirty(void)
{
    for (int w = 0; w < (int)(sizeof dirty / sizeof *dirty); w++) {
        u32 bits = dirty[w];
        if (!bits) continue;
        dirty[w] = 0;
        while (bits) {
            int b = 31 - __builtin_clz(bits);
            bits &= ~(1u << b);
            int ti = w * 32 + b;
            if (ti < CV_TILES)
                push_tile(ti);
        }
    }
}

// ---- shapes ------------------------------------------------------------------
static void draw_line(int x0, int y0, int x1, int y1, int c)
{
    int dx = x1 > x0 ? x1 - x0 : x0 - x1, sx = x0 < x1 ? 1 : -1;
    int dy = y1 > y0 ? y0 - y1 : y1 - y0, sy = y0 < y1 ? 1 : -1;  // dy negative
    int err = dx + dy;
    for (;;) {
        px_set(x0, y0, c);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

static void draw_rect(int x0, int y0, int x1, int y1, int c)
{
    if (x0 > x1) { int t = x0; x0 = x1; x1 = t; }
    if (y0 > y1) { int t = y0; y0 = y1; y1 = t; }
    for (int x = x0; x <= x1; x++) { px_set(x, y0, c); px_set(x, y1, c); }
    for (int y = y0; y <= y1; y++) { px_set(x0, y, c); px_set(x1, y, c); }
}

// ---- stamps (16x16, 1bpp rows) ------------------------------------------------
typedef struct { const char *name; u16 row[16]; } Stamp;
static const Stamp stamps[] = {
    { "heart", {0x0000,0x0000,0x1C38,0x3E7C,0x7FFE,0x7FFE,0x7FFE,0x3FFC,
                0x1FF8,0x0FF0,0x07E0,0x03C0,0x0180,0x0000,0x0000,0x0000} },
    { "star",  {0x0180,0x0180,0x03C0,0x03C0,0x07E0,0xFFFF,0x7FFE,0x3FFC,
                0x1FF8,0x0FF0,0x1FF8,0x3E7C,0x783E,0x6006,0x0000,0x0000} },
    { "smile", {0x07E0,0x1FF8,0x3FFC,0x7FFE,0x666E,0x666E,0xFFFF,0xFFFF,
                0xFFFF,0xDFFB,0x6FF6,0x77EE,0x39DC,0x1FF8,0x07E0,0x0000} },
    { "house", {0x0180,0x03C0,0x07E0,0x0FF0,0x1FF8,0x3FFC,0x7FFE,0xFFFF,
                0x7FFE,0x7E7E,0x7E7E,0x7E7E,0x7E7E,0x7E7E,0x7FFE,0x0000} },
    { "tree",  {0x0180,0x03C0,0x07E0,0x0FF0,0x07E0,0x0FF0,0x1FF8,0x3FFC,
                0x1FF8,0x3FFC,0x7FFE,0x0180,0x0180,0x0180,0x03C0,0x0000} },
    { "check", {0x0000,0x0001,0x0003,0x0007,0x000E,0x001C,0x8038,0xC070,
                0xE0E0,0x71C0,0x3B80,0x1F00,0x0E00,0x0400,0x0000,0x0000} },
};
#define N_STAMPS ((int)(sizeof stamps / sizeof *stamps))

static void apply_stamp(int x0, int y0, int c)
{
    for (int y = 0; y < 16; y++) {
        u16 row = stamps[cur_stamp].row[y];
        for (int x = 0; x < 16; x++)
            if (row & (0x8000 >> x))
                px_set(x0 + x - 8, y0 + y - 8, c);
    }
}

// ---- chrome / HUD -------------------------------------------------------------
static void draw_hud(void)
{
    char buf[6];
    u16 *tm = text_map();

    // left gutter: one icon per tool, selected on an accent cell
    for (int t = 0; t < TOOL_COUNT; t++) {
        int pb = (t == cur_tool) ? PB_SEL : PB_UI;
        int bg = (t == cur_tool) ? SOLID(4) : SOLID(2);
        map_put(tm, 2, 2 + t * 2, tool_icons[t], pb);
        map_put(chrome_map(), 2, 2 + t * 2, bg, PB_SOLID);
    }

    // current color: shade number 1..4 on an accent cell
    buf[0] = (char)('1' + cur_color);
    buf[1] = 0;
    ui_text(2, 14, buf, PB_SEL);
    map_put(chrome_map(), 2, 14, SOLID(4), PB_SOLID);
    map_put(tm, 3, 14, cur_color == 0 ? CH_SQ_HOLLOW : CH_SQ_FILL, PB_UI);

    // right gutter: stamp name when the stamp tool is active
    if (cur_tool == TOOL_STAMP) {
        map_put(tm, 25, 2, CH_ICON_STAMP, PB_UI);
        const char *nm = stamps[cur_stamp].name;
        for (int i = 0; i < 5; i++)
            map_put(tm, 25 + i, 3, (u8)(nm[i] ? nm[i] : ' '), PB_SEL);
    } else {
        for (int i = 0; i < 5; i++) {
            map_put(tm, 25 + i, 2, CH_BLANK, PB_UI);
            map_put(tm, 25 + i, 3, CH_BLANK, PB_UI);
        }
    }
}

static void draw_chrome_full(void)
{
    // gutters dark on BG1; canvas region BG1 transparent (tile 0 has no pixels)
    for (int y = 0; y < SCREEN_TH; y++)
        for (int x = 0; x < SCREEN_TW; x++) {
            bool in_cv = (x >= CV_X0 / 8 && x < CV_X0 / 8 + CV_TW &&
                          y >= CV_Y0 / 8 && y < CV_Y0 / 8 + CV_TH);
            map_put(text_map(), x, y, CH_BLANK, PB_UI);
            if (in_cv)
                se_mem[SBB_CHROME][y * 32 + x] = 0;   // transparent: canvas shows
            else
                map_put(chrome_map(), x, y, SOLID(2), PB_SOLID);
        }
    draw_hud();
}

// ---- cursor sprite -------------------------------------------------------------
static void init_cursor_sprite(void)
{
    // crosshair, hotspot at (3,3), color 1 of OBJ palette bank 1
    static const u32 cross[8] = {
        0x00010000, 0x00010000, 0x00010000, 0x01010101,
        0x00010000, 0x00010000, 0x00010000, 0x00000000,
    };
    memcpy32(&tile_mem[4][1], cross, 8);
    pal_obj_mem[16 + 1] = RGB15(31, 0, 8);
}

static void update_cursor_sprite(int frame)
{
    OBJ_ATTR *o = &oam_mem[1];
    // blink between red and white so it is visible on any shade
    pal_obj_mem[16 + 1] = (frame & 16) ? RGB15(31, 2, 8) : RGB15(31, 31, 31);
    obj_set_attr(o,
                 ((CV_Y0 + cy - 3) & 0x00FF),
                 ((CV_X0 + cx - 3) & 0x01FF),
                 ATTR2_PALBANK(1) | 1);
}

// ---- scene ---------------------------------------------------------------------
void draw_enter(void)
{
    REG_DISPCNT = DCNT_MODE0 | DCNT_BG0 | DCNT_BG1 | DCNT_BG2 | DCNT_OBJ | DCNT_OBJ_1D;
    REG_BG2CNT = BG_CBB(CV_CBB) | BG_SBB(CV_SBB) | BG_4BPP | BG_REG_32x32 | BG_PRIO(2);
    REG_BG2HOFS = (u16)(-CV_X0);
    REG_BG2VOFS = (u16)(-CV_Y0);

    // map: each canvas cell -> its own tile
    for (int y = 0; y < CV_TH; y++)
        for (int x = 0; x < CV_TW; x++)
            se_mem[CV_SBB][y * 32 + x] = (u16)(SE_PALBANK(PB_CANVAS) | (y * CV_TW + x));

    draw_apply_palette();
    obj_hide(&oam_mem[0]);
    init_cursor_sprite();
    anchored = false;
    mark_all_dirty();
    flush_dirty();
    draw_chrome_full();
}

u8 *draw_canvas(void)            { return canvas; }
void draw_canvas_reloaded(void)  { mark_all_dirty(); }
void draw_clear(void)            { memset(canvas, 0, sizeof canvas); mark_all_dirty(); }

static void tool_apply_at(int x, int y)
{
    switch (cur_tool) {
    case TOOL_PENCIL: px_set(x, y, cur_color); break;
    case TOOL_ERASER:
        for (int j = -1; j <= 1; j++)
            for (int i = -1; i <= 1; i++)
                px_set(x + i, y + j, 0);
        break;
    }
}

static void preview_shape(void)
{
    memcpy(canvas, snapshot, CV_BYTES);
    if (cur_tool == TOOL_LINE)
        draw_line(ax, ay, cx, cy, cur_color);
    else
        draw_rect(ax, ay, cx, cy, cur_color);
    mark_all_dirty();
}

void draw_frame(int frame)
{
    u16 held = key_curr_state();
    u16 hits = key_hit(0xFFFF);

    // cursor movement: 1px per hit, hold to glide (faster after a second)
    int mx = 0, my = 0;
    if (held & (KEY_LEFT | KEY_RIGHT | KEY_UP | KEY_DOWN))
        held_frames++;
    else
        held_frames = 0;
    bool step = (hits & (KEY_LEFT | KEY_RIGHT | KEY_UP | KEY_DOWN)) ||
                (held_frames > 10 && (held_frames > 60 || (held_frames & 1)));
    if (step) {
        if (held & KEY_LEFT)  mx = -1;
        if (held & KEY_RIGHT) mx = +1;
        if (held & KEY_UP)    my = -1;
        if (held & KEY_DOWN)  my = +1;
    }
    if (mx | my) {
        cx += mx; cy += my;
        cx = cx < 0 ? 0 : (cx >= CV_W ? CV_W - 1 : cx);
        cy = cy < 0 ? 0 : (cy >= CV_H ? CV_H - 1 : cy);
        if (anchored)
            preview_shape();
        else if (held & KEY_A)
            tool_apply_at(cx, cy);   // drag-draw for pencil/eraser
    }

    if (hits & KEY_A) {
        switch (cur_tool) {
        case TOOL_PENCIL:
        case TOOL_ERASER:
            tool_apply_at(cx, cy);
            break;
        case TOOL_LINE:
        case TOOL_RECT:
            if (!anchored) {
                memcpy(snapshot, canvas, CV_BYTES);
                ax = cx; ay = cy;
                anchored = true;
                preview_shape();
            } else {
                anchored = false;    // commit: canvas already holds the shape
            }
            break;
        case TOOL_STAMP:
            apply_stamp(cx, cy, cur_color);
            break;
        }
    }

    if (hits & KEY_B) {
        if (anchored) {              // cancel shape preview
            memcpy(canvas, snapshot, CV_BYTES);
            mark_all_dirty();
            anchored = false;
        }
    }

    if (hits & KEY_SELECT) {
        cur_tool = (cur_tool + 1) % TOOL_COUNT;
        anchored = false;
        draw_hud();
    }
    if (hits & KEY_R) {
        if (cur_tool == TOOL_STAMP) cur_stamp = (cur_stamp + 1) % N_STAMPS;
        else                        cur_color = (cur_color + 1) & 3;
        draw_hud();
    }
    if (hits & KEY_L) {
        if (cur_tool == TOOL_STAMP) cur_stamp = (cur_stamp + N_STAMPS - 1) % N_STAMPS;
        else                        cur_color = (cur_color + 3) & 3;
        draw_hud();
    }

    if (hits & KEY_START) {
        app_goto(ST_DRAWMENU);
        return;
    }

    flush_dirty();
    update_cursor_sprite(frame);
}
