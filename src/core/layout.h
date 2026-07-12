// Layout engine: text -> line-start table for a fixed column width.
// Portable C — no GBA headers. Shared by the editor, the printer path, and host tests.
#ifndef CORE_LAYOUT_H
#define CORE_LAYOUT_H

#include <stdint.h>
#include "document.h"

#define LAYOUT_MAX_LINES (DOC_MAX + 1)

typedef enum {
    WRAP_MID_WORD,   // hard break exactly at the column limit
    WRAP_POST_WORD,  // break after the last full word that fits
} WrapMode;

typedef struct {
    uint16_t start[LAYOUT_MAX_LINES]; // index of first char of each line
    int count;                        // number of lines (>= 1)
} Layout;

// Recompute the whole line table. width = columns per line (e.g. 20).
void layout_reflow(Layout *lo, const char *text, int len, int width, WrapMode mode);

// Line the caret index sits on (caret at a wrap boundary belongs to the next line).
int layout_caret_line(const Layout *lo, int caret);

// Chars on line `ln` excluding the trailing newline (for rendering / caret columns).
int layout_line_len(const Layout *lo, const char *text, int len, int ln);

#endif
