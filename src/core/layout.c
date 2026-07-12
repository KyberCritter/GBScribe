#include "layout.h"

#define NL 0x0A

void layout_reflow(Layout *lo, const char *text, int len, int width, WrapMode mode)
{
    int n = 0;
    lo->start[n++] = 0;

    int col = 0;          // chars on current line so far
    int last_space = -1;  // index AFTER which we may wrap (space position)

    for (int i = 0; i < len; i++) {
        char c = text[i];
        if (c == NL) {
            lo->start[n++] = (uint16_t)(i + 1);
            col = 0;
            last_space = -1;
            continue;
        }
        if (col >= width) {
            if (c == ' ' && mode == WRAP_POST_WORD) {
                // spaces may overflow past the margin; renderers clip them
                last_space = i;
                continue;
            }
            int brk;
            if (mode == WRAP_POST_WORD && last_space >= 0) {
                brk = last_space + 1;   // break after the last space
            } else {
                brk = i;                // hard break before this char
            }
            lo->start[n++] = (uint16_t)brk;
            col = i - brk;
            last_space = -1;
        }
        if (c == ' ')
            last_space = i;
        col++;
    }
    lo->count = n;
}

int layout_caret_line(const Layout *lo, int caret)
{
    // last line with start <= caret (binary search)
    int lostart = 0, hi = lo->count - 1;
    while (lostart < hi) {
        int mid = (lostart + hi + 1) / 2;
        if (lo->start[mid] <= caret)
            lostart = mid;
        else
            hi = mid - 1;
    }
    return lostart;
}

int layout_line_len(const Layout *lo, const char *text, int len, int ln)
{
    int s = lo->start[ln];
    int e = (ln + 1 < lo->count) ? lo->start[ln + 1] : len;
    // a trailing newline terminates the line but isn't part of its content
    if (e > s && e - 1 < len && text[e - 1] == NL)
        e--;
    return e - s;
}
