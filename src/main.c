// GBScribe — word processor + pixel art for GBA, printing to the Game Boy Printer.
// M1: font, text page rendering, on-screen keyboard, basic editing.

#include <string.h>
#include <tonc.h>
#include "gfx/font.h"
#include "core/charset.h"
#include "ui/screen.h"
#include "ui/keyboard.h"

// ---- M1 editor buffer (real document model arrives in M2) ----
#define BUF_MAX 1024
static char buf[BUF_MAX];
static int buf_len = 0;

static int cur_layer = 0;
static int cur_key = 0;

// caret cell (page-relative), updated by render_text
static int caret_cx, caret_cy;

static void init_video(void)
{
    REG_DISPCNT = DCNT_MODE0 | DCNT_BG0 | DCNT_BG1 | DCNT_OBJ | DCNT_OBJ_1D;
    REG_BG0CNT = BG_CBB(0) | BG_SBB(SBB_TEXT)   | BG_4BPP | BG_REG_32x32 | BG_PRIO(0);
    REG_BG1CNT = BG_CBB(0) | BG_SBB(SBB_CHROME) | BG_4BPP | BG_REG_32x32 | BG_PRIO(1);

    memcpy32(&tile_mem[0][0], font_tiles, FONT_TILE_COUNT * 8);

    // Palettes
    pal_bg_mem[16*PB_INK   + 1] = RGB15( 2,  2,  3);   // document ink
    pal_bg_mem[16*PB_UI    + 1] = RGB15(26, 27, 29);   // keyboard label ink
    pal_bg_mem[16*PB_SEL   + 1] = RGB15( 1,  1,  2);   // selected key label ink
    pal_bg_mem[16*PB_SOLID + 1] = RGB15(31, 31, 30);   // paper
    pal_bg_mem[16*PB_SOLID + 2] = RGB15( 4,  5,  7);   // keyboard bg
    pal_bg_mem[16*PB_SOLID + 3] = RGB15( 6,  7, 10);   // gutter
    pal_bg_mem[16*PB_SOLID + 4] = RGB15(31, 24,  6);   // accent (selected key)
    pal_bg_mem[16*PB_SOLID + 5] = RGB15(10, 12, 16);   // separator
    pal_bg_mem[0] = RGB15(2, 2, 3);                    // backdrop

    // Caret sprite: 8x8, thick underscore, obj palette 0 color 1
    static const u32 caret_tile[8] = {0,0,0,0,0,0,0x11111111,0x11111111};
    memcpy32(&tile_mem[4][0], caret_tile, 8);
    pal_obj_mem[1] = RGB15(4, 8, 24);

    oam_init(oam_mem, 128);
}

static void draw_chrome(void)
{
    u16 *cm = chrome_map();
    for (int y = 0; y < PAGE_ROWS; y++)
        for (int x = 0; x < SCREEN_TW; x++) {
            bool page = (x >= PAGE_X0 && x < PAGE_X0 + PAGE_W);
            map_put(cm, x, y, page ? SOLID(1) : SOLID(3), PB_SOLID);
        }
    for (int x = 0; x < SCREEN_TW; x++)
        map_put(cm, x, PAGE_ROWS, SOLID(5), PB_SOLID);
}

static void render_text(void)
{
    u16 *tm = text_map();
    for (int y = 0; y < PAGE_ROWS; y++)
        for (int x = PAGE_X0; x < PAGE_X0 + PAGE_W; x++)
            map_put(tm, x, y, CH_BLANK, PB_INK);

    int cx = 0, cy = 0;
    for (int i = 0; i < buf_len && cy < PAGE_ROWS; i++) {
        char c = buf[i];
        if (c == CH_NEWLINE) {
            cx = 0; cy++;
            continue;
        }
        map_put(tm, PAGE_X0 + cx, cy, (u8)c, PB_INK);
        if (++cx >= PAGE_W) { cx = 0; cy++; }
    }
    caret_cx = cx; caret_cy = cy;
}

static void update_caret(int frame)
{
    OBJ_ATTR *caret = &oam_mem[0];
    bool visible = (caret_cy < PAGE_ROWS) && ((frame >> 5) & 1);
    if (visible)
        obj_set_attr(caret,
                     (caret_cy * 8) & 0x00FF,             // square 8x8, y
                     ((PAGE_X0 + caret_cx) * 8) & 0x01FF, // x
                     0);                                  // tile 0, palbank 0
    else
        obj_hide(caret);
}

static void type_code(int code)
{
    if (buf_len < BUF_MAX) {
        buf[buf_len++] = (char)code;
        render_text();
    }
}

static void backspace(void)
{
    if (buf_len > 0) {
        buf_len--;
        render_text();
    }
}

static void activate_key(const Key *k)
{
    switch (k->action) {
    case KA_LAYER_LOWER: case KA_LAYER_UPPER:
    case KA_LAYER_SYM:   case KA_LAYER_SHAPE: {
        int to = k->action - KA_LAYER_LOWER;
        cur_key = kb_nearest(cur_layer, cur_key, to);
        cur_layer = to;
        kb_render(cur_layer, cur_key);
        break;
    }
    case KA_ENTER:
        type_code(CH_NEWLINE);
        break;
    case KA_BACKSPACE:
        backspace();
        break;
    default:
        type_code(k->action);
    }
}

// D-pad/B auto-repeat: initial delay 16 frames, then every 4.
static u16 rep_keys, rep_timer;

static u16 repeat_hits(void)
{
    const u16 mask = KEY_LEFT | KEY_RIGHT | KEY_UP | KEY_DOWN | KEY_B;
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

int main(void)
{
    irq_init(NULL);
    irq_enable(II_VBLANK);

    init_video();
    draw_chrome();
    render_text();
    kb_render(cur_layer, cur_key);

    int frame = 0;
    while (1) {
        VBlankIntrWait();
        key_poll();
        frame++;

        u16 hits = repeat_hits();

        int dx = 0, dy = 0;
        if (hits & KEY_LEFT)  dx = -1;
        if (hits & KEY_RIGHT) dx = +1;
        if (hits & KEY_UP)    dy = -1;
        if (hits & KEY_DOWN)  dy = +1;
        if (dx || dy) {
            int nk = kb_move(cur_layer, cur_key, dx, dy);
            if (nk != cur_key) {
                cur_key = nk;
                kb_render(cur_layer, cur_key);
            }
        }

        if (hits & KEY_A)
            activate_key(kb_key(cur_layer, cur_key));
        if (hits & KEY_B)
            backspace();
        if (key_hit(KEY_SELECT)) {
            int to = (cur_layer + 1) % KB_LAYER_COUNT;
            cur_key = kb_nearest(cur_layer, cur_key, to);
            cur_layer = to;
            kb_render(cur_layer, cur_key);
        }
        if (key_hit(KEY_R))
            type_code(' ');

        update_caret(frame);
    }
    return 0;
}
