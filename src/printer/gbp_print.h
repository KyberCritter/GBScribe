// Page composition: document text -> 160px-wide GB 2bpp tile page.
// PORTABLE C — shared by the ROM and the host decode harness.
#ifndef PRINTER_GBP_PRINT_H
#define PRINTER_GBP_PRINT_H

#include <stdint.h>
#include "core/layout.h"

#define GBP_TILES_PER_ROW 20
#define GBP_ROWS_PER_PAGE 18                 // 160x144 px = one printed page
#define GBP_ROW_BYTES     (GBP_TILES_PER_ROW * 16)          // 320
#define GBP_PAGE_BYTES    (GBP_ROW_BYTES * GBP_ROWS_PER_PAGE) // 5760
#define GBP_DATA_PER_PKT  0x280              // 2 tile rows per DATA packet
#define GBP_PKTS_PER_PAGE (GBP_PAGE_BYTES / GBP_DATA_PER_PKT) // 9

// Render up to `n` glyphs into one tile row (320 bytes), blank-padded.
void gbp_render_text_row(const char *chars, int n, uint8_t *out);

// Compose one printed page (5760 bytes) starting at layout line `first_line`.
// Lines past the end of the document become blank rows.
void gbp_compose_text_page(const char *text, int len, const Layout *lo,
                           int first_line, uint8_t *page);

// Number of printed pages for a laid-out document.
static inline int gbp_page_count(const Layout *lo)
{
    return (lo->count + GBP_ROWS_PER_PAGE - 1) / GBP_ROWS_PER_PAGE;
}

#endif
