// On-screen QWERTY keyboard: data-driven layout, BG rendering, D-pad navigation.
#ifndef UI_KEYBOARD_H
#define UI_KEYBOARD_H

#include <tonc.h>

// Key action codes >= 0x80 are specials; below that = charset code to type.
enum {
    KA_LAYER_LOWER = 0x80,
    KA_LAYER_UPPER,
    KA_LAYER_SYM,
    KA_LAYER_SHAPE,
    KA_ENTER,
    KA_BACKSPACE,
};

#define KB_LAYER_COUNT 4

typedef struct {
    u8 x, y, w;          // tile position and width on screen
    u8 action;           // charset code or KA_*
    const char *label;   // rendered label (charset codes as bytes)
} Key;

// Initialise/re-render the keyboard area for a layer, with key `sel` highlighted.
void kb_render(int layer, int sel);
// Move the selection with a D-pad direction; returns new selection index.
int kb_move(int layer, int sel, int dx, int dy);
// Number of keys in a layer / key lookup.
int kb_key_count(int layer);
const Key *kb_key(int layer, int idx);
// Find nearest key in `to` layer to key `sel` of layer `from` (for layer switches).
int kb_nearest(int from, int sel, int to);

#endif
