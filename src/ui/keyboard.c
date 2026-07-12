#include <string.h>
#include "keyboard.h"
#include "screen.h"
#include "core/charset.h"

// ---- Layout tables --------------------------------------------------------

#define K(cx, cy, ch)      { cx, cy, 1, ch, NULL }
#define KW(cx, cy, ww, act, lab) { cx, cy, ww, act, lab }

// Common bottom row (y=19): layer switches, space, enter.
#define COMMON_ROW \
    KW( 2, 19, 3, KA_LAYER_LOWER, "abc"), \
    KW( 6, 19, 3, KA_LAYER_UPPER, "ABC"), \
    KW(10, 19, 3, KA_LAYER_SYM,   "!@#"), \
    KW(14, 19, 3, KA_LAYER_SHAPE, "\x09\x06\x04"), \
    KW(18, 19, 5, ' ',            "SPACE"), \
    KW(24, 19, 3, KA_ENTER,       "ENT")

static const Key kb_lower[] = {
    K( 5,15,'q'), K( 7,15,'w'), K( 9,15,'e'), K(11,15,'r'), K(13,15,'t'),
    K(15,15,'y'), K(17,15,'u'), K(19,15,'i'), K(21,15,'o'), K(23,15,'p'),
    K( 6,16,'a'), K( 8,16,'s'), K(10,16,'d'), K(12,16,'f'), K(14,16,'g'),
    K(16,16,'h'), K(18,16,'j'), K(20,16,'k'), K(22,16,'l'),
    K( 7,17,'z'), K( 9,17,'x'), K(11,17,'c'), K(13,17,'v'), K(15,17,'b'),
    K(17,17,'n'), K(19,17,'m'), K(21,17,','), K(23,17,'.'),
    COMMON_ROW,
};

static const Key kb_upper[] = {
    K( 5,15,'Q'), K( 7,15,'W'), K( 9,15,'E'), K(11,15,'R'), K(13,15,'T'),
    K(15,15,'Y'), K(17,15,'U'), K(19,15,'I'), K(21,15,'O'), K(23,15,'P'),
    K( 6,16,'A'), K( 8,16,'S'), K(10,16,'D'), K(12,16,'F'), K(14,16,'G'),
    K(16,16,'H'), K(18,16,'J'), K(20,16,'K'), K(22,16,'L'),
    K( 7,17,'Z'), K( 9,17,'X'), K(11,17,'C'), K(13,17,'V'), K(15,17,'B'),
    K(17,17,'N'), K(19,17,'M'), K(21,17,','), K(23,17,'.'),
    COMMON_ROW,
};

static const Key kb_sym[] = {
    K( 5,14,'1'), K( 7,14,'2'), K( 9,14,'3'), K(11,14,'4'), K(13,14,'5'),
    K(15,14,'6'), K(17,14,'7'), K(19,14,'8'), K(21,14,'9'), K(23,14,'0'),
    K( 5,15,'!'), K( 7,15,'@'), K( 9,15,'#'), K(11,15,'$'), K(13,15,'%'),
    K(15,15,'^'), K(17,15,'&'), K(19,15,'*'), K(21,15,'('), K(23,15,')'),
    K( 5,16,'-'), K( 7,16,'_'), K( 9,16,'='), K(11,16,'+'), K(13,16,'['),
    K(15,16,']'), K(17,16,'{'), K(19,16,'}'), K(21,16,'\\'), K(23,16,'|'),
    K( 5,17,';'), K( 7,17,':'), K( 9,17,'\''), K(11,17,'"'), K(13,17,'<'),
    K(15,17,'>'), K(17,17,'/'), K(19,17,'?'), K(21,17,'~'), K(23,17,'`'),
    COMMON_ROW,
};

static const Key kb_shape[] = {
    K( 5,14,CH_ARR_UP), K( 7,14,CH_ARR_DOWN), K( 9,14,CH_ARR_LEFT), K(11,14,CH_ARR_RIGHT),
    K( 5,15,CH_SQ_FILL), K( 7,15,CH_SQ_HOLLOW), K( 9,15,CH_CIRC_FILL), K(11,15,CH_CIRC_HOLLOW),
    K(13,15,CH_TRI_FILL), K(15,15,CH_TRI_HOLLOW), K(17,15,CH_DIA_FILL), K(19,15,CH_DIA_HOLLOW),
    K( 5,16,CH_BOX_H), K( 7,16,CH_BOX_V), K( 9,16,CH_BOX_TL), K(11,16,CH_BOX_TR),
    K(13,16,CH_BOX_BL), K(15,16,CH_BOX_BR),
    K( 5,17,CH_BOX_LT), K( 7,17,CH_BOX_RT), K( 9,17,CH_BOX_TT), K(11,17,CH_BOX_BT),
    K(13,17,CH_BOX_X),
    COMMON_ROW,
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
        // nearest row in direction dy (wrapping), then nearest x within it
        int best_y = -1;
        for (int i = 0; i < n; i++) {
            int d = (ks[i].y - cur->y) * dy;
            if (d > 0 && (best_y < 0 || d < (best_y - cur->y) * dy))
                best_y = ks[i].y;
        }
        if (best_y < 0) {
            // wrap: pick the row furthest in the opposite direction
            for (int i = 0; i < n; i++) {
                int d = (ks[i].y - cur->y) * dy;
                if (d <= 0 && (best_y < 0 || (ks[i].y - best_y) * dy < 0))
                    best_y = ks[i].y;
            }
        }
        int best = sel, best_d = 1 << 30;
        for (int i = 0; i < n; i++) {
            if (ks[i].y != best_y) continue;
            int d = key_center(&ks[i]) - key_center(cur);
            if (d < 0) d = -d;
            if (d < best_d) { best = i; best_d = d; }
        }
        return best;
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
