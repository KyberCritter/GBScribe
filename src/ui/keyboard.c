#include <string.h>
#include "keyboard.h"
#include "screen.h"
#include "core/charset.h"

// ---- Layout tables --------------------------------------------------------

#define K(cx, cy, ch)      { cx, cy, 1, ch, NULL }
#define KW(cx, cy, ww, act, lab) { cx, cy, ww, act, lab }

// All layers share the same grid: key columns at x = 2,4,...,20 on rows
// 14-17 (no QWERTY stagger — up/down moves straight), the right-side column
// stacked on rows 15-17, and only the four layer switches on the bottom row.
// Letter layers add SHIFT above SPACE; the other layers hide it.
#define COMMON_KEYS \
    KW(24, 16, 5, ' ',            "SPACE"), \
    KW(24, 17, 5, KA_ENTER,       "ENTER"), \
    KW( 2, 19, 3, KA_LAYER_LOWER, "abc"), \
    KW( 6, 19, 3, KA_LAYER_UPPER, "ABC"), \
    KW(10, 19, 3, KA_LAYER_SYM,   "!@#"), \
    KW(14, 19, 3, KA_LAYER_SHAPE, "\x09\x06\x04")
#define SHIFT_KEY KW(24, 15, 5, KA_SHIFT, "SHIFT")

static const Key kb_lower[] = {
    K( 2,15,'q'), K( 4,15,'w'), K( 6,15,'e'), K( 8,15,'r'), K(10,15,'t'),
    K(12,15,'y'), K(14,15,'u'), K(16,15,'i'), K(18,15,'o'), K(20,15,'p'),
    K( 2,16,'a'), K( 4,16,'s'), K( 6,16,'d'), K( 8,16,'f'), K(10,16,'g'),
    K(12,16,'h'), K(14,16,'j'), K(16,16,'k'), K(18,16,'l'),
    K( 2,17,'z'), K( 4,17,'x'), K( 6,17,'c'), K( 8,17,'v'), K(10,17,'b'),
    K(12,17,'n'), K(14,17,'m'), K(16,17,','), K(18,17,'.'),
    SHIFT_KEY,
    COMMON_KEYS,
};

static const Key kb_upper[] = {
    K( 2,15,'Q'), K( 4,15,'W'), K( 6,15,'E'), K( 8,15,'R'), K(10,15,'T'),
    K(12,15,'Y'), K(14,15,'U'), K(16,15,'I'), K(18,15,'O'), K(20,15,'P'),
    K( 2,16,'A'), K( 4,16,'S'), K( 6,16,'D'), K( 8,16,'F'), K(10,16,'G'),
    K(12,16,'H'), K(14,16,'J'), K(16,16,'K'), K(18,16,'L'),
    K( 2,17,'Z'), K( 4,17,'X'), K( 6,17,'C'), K( 8,17,'V'), K(10,17,'B'),
    K(12,17,'N'), K(14,17,'M'), K(16,17,','), K(18,17,'.'),
    SHIFT_KEY,
    COMMON_KEYS,
};

static const Key kb_sym[] = {
    K( 2,14,'1'), K( 4,14,'2'), K( 6,14,'3'), K( 8,14,'4'), K(10,14,'5'),
    K(12,14,'6'), K(14,14,'7'), K(16,14,'8'), K(18,14,'9'), K(20,14,'0'),
    K( 2,15,'!'), K( 4,15,'@'), K( 6,15,'#'), K( 8,15,'$'), K(10,15,'%'),
    K(12,15,'^'), K(14,15,'&'), K(16,15,'*'), K(18,15,'('), K(20,15,')'),
    K( 2,16,'-'), K( 4,16,'_'), K( 6,16,'='), K( 8,16,'+'), K(10,16,'['),
    K(12,16,']'), K(14,16,'{'), K(16,16,'}'), K(18,16,'\\'), K(20,16,'|'),
    K( 2,17,';'), K( 4,17,':'), K( 6,17,'\''), K( 8,17,'"'), K(10,17,'<'),
    K(12,17,'>'), K(14,17,'/'), K(16,17,'?'), K(18,17,'~'), K(20,17,'`'),
    COMMON_KEYS,
};

static const Key kb_shape[] = {
    K( 2,14,CH_ARR_UP), K( 4,14,CH_ARR_DOWN), K( 6,14,CH_ARR_LEFT), K( 8,14,CH_ARR_RIGHT),
    K( 2,15,CH_SQ_FILL), K( 4,15,CH_SQ_HOLLOW), K( 6,15,CH_CIRC_FILL), K( 8,15,CH_CIRC_HOLLOW),
    K(10,15,CH_TRI_FILL), K(12,15,CH_TRI_HOLLOW), K(14,15,CH_DIA_FILL), K(16,15,CH_DIA_HOLLOW),
    K( 2,16,CH_BOX_H), K( 4,16,CH_BOX_V), K( 6,16,CH_BOX_TL), K( 8,16,CH_BOX_TR),
    K(10,16,CH_BOX_BL), K(12,16,CH_BOX_BR),
    K( 2,17,CH_BOX_LT), K( 4,17,CH_BOX_RT), K( 6,17,CH_BOX_TT), K( 8,17,CH_BOX_BT),
    K(10,17,CH_BOX_X),
    COMMON_KEYS,
};

static const Key *layers[KB_LAYER_COUNT] = { kb_lower, kb_upper, kb_sym, kb_shape };
static const int layer_len[KB_LAYER_COUNT] = {
    sizeof kb_lower / sizeof *kb_lower,
    sizeof kb_upper / sizeof *kb_upper,
    sizeof kb_sym   / sizeof *kb_sym,
    sizeof kb_shape / sizeof *kb_shape,
};

int kb_key_count(int layer)          { return layer_len[layer]; }
const Key *kb_key(int layer, int i)  { return &layers[layer][i]; }

// ---- Rendering ------------------------------------------------------------

#define KB_Y0 14
#define KB_Y1 19

static void draw_key(const Key *k, bool sel)
{
    int pb_ink = sel ? PB_SEL : PB_UI;
    int bg     = sel ? SOLID(4) : SOLID(2);
    for (int i = 0; i < k->w; i++)
        map_put(chrome_map(), k->x + i, k->y, bg, PB_SOLID);

    if (k->w == 1) {
        map_put(text_map(), k->x, k->y, k->action, pb_ink);
    } else {
        int len = strlen(k->label);
        int off = (k->w - len) / 2;
        for (int i = 0; i < len; i++)
            map_put(text_map(), k->x + off + i, k->y, (u8)k->label[i], pb_ink);
    }
}

void kb_render(int layer, int sel)
{
    // clear keyboard region
    for (int y = KB_Y0; y <= KB_Y1; y++)
        for (int x = 0; x < SCREEN_TW; x++) {
            map_put(text_map(), x, y, CH_BLANK, PB_UI);
            map_put(chrome_map(), x, y, SOLID(2), PB_SOLID);
        }
    for (int i = 0; i < layer_len[layer]; i++)
        draw_key(&layers[layer][i], i == sel);
}

// ---- Navigation -----------------------------------------------------------

static int key_center(const Key *k) { return k->x * 2 + k->w; } // x2 to stay integral

int kb_move(int layer, int sel, int dx, int dy)
{
    const Key *cur = &layers[layer][sel];
    const Key *ks = layers[layer];
    int n = layer_len[layer];

    if (dx) {
        // nearest key on the same row in direction dx, wrapping
        int best = -1, best_d = 0;
        int wrap = -1, wrap_x = 0;
        for (int i = 0; i < n; i++) {
            if (ks[i].y != cur->y || i == sel) continue;
            int d = (ks[i].x - cur->x) * dx;
            if (d > 0 && (best < 0 || d < best_d)) { best = i; best_d = d; }
            int wx = ks[i].x * -dx;  // furthest opposite = wrap target
            if (wrap < 0 || wx > wrap_x) { wrap = i; wrap_x = wx; }
        }
        if (best >= 0) return best;
        if (wrap >= 0) return wrap;
        return sel;
    }
    if (dy) {
        // scored: row distance (x8) + column distance, so a key straight
        // below two rows away beats a far-off key one row away (SPACE->ENTER)
        int best = -1, best_s = 1 << 30;
        for (int i = 0; i < n; i++) {
            int dr = (ks[i].y - cur->y) * dy;
            if (dr <= 0) continue;
            int dc = key_center(&ks[i]) - key_center(cur);
            if (dc < 0) dc = -dc;
            int s = dr * 8 + dc;
            if (s < best_s) { best = i; best_s = s; }
        }
        if (best >= 0) return best;
        // wrap: same score over the opposite side favours the far edge row
        for (int i = 0; i < n; i++) {
            if (i == sel) continue;
            int dr = (ks[i].y - cur->y) * dy;   // <= 0 here
            int dc = key_center(&ks[i]) - key_center(cur);
            if (dc < 0) dc = -dc;
            int s = dr * 8 + dc;
            if (s < best_s) { best = i; best_s = s; }
        }
        return best >= 0 ? best : sel;
    }
    return sel;
}

int kb_nearest(int from, int sel, int to)
{
    const Key *cur = &layers[from][sel];
    int best = 0, best_d = 1 << 30;
    for (int i = 0; i < layer_len[to]; i++) {
        const Key *k = &layers[to][i];
        int dx = key_center(k) - key_center(cur);
        int dy = (k->y - cur->y) * 8;
        int d = dx * dx + dy * dy;
        if (d < best_d) { best = i; best_d = d; }
    }
    return best;
}
