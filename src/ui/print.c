// Print scene: drives a full print job, one packet per frame, with progress UI.
#include <string.h>
#include <stdio.h>
#include <tonc.h>
#include "app.h"
#include "ui/ui.h"
#include "ui/screen.h"
#include "ui/editor.h"
#include "core/charset.h"
#include "core/layout.h"
#include "printer/gbp_packet.h"
#include "printer/gbp_print.h"
#include "printer/gbp_link.h"
#include "storage/storage.h"
#include "storage/save.h"

enum {
    PS_NO_PRINTER,
    PS_EMPTY_DOC,
    PS_CONFIRM,
    PS_INIT,        // send INIT, then 200ms settle
    PS_DATA,        // one DATA packet per frame
    PS_ARM,         // empty DATA
    PS_PRINT,       // PRINT command
    PS_WAIT,        // poll INQUIRY until printing finishes
    PS_DONE,
    PS_ERROR,
};

static int ps;
static int cur_page, n_pages;
static int data_idx;
static int wait_frames;
static bool saw_printing;
static u8 last_status;

EWRAM_BSS static Layout plo;
EWRAM_BSS static u8 page_buf[GBP_PAGE_BYTES];
static u8 pkt_buf[GBP_PACKET_MAX];

static const Document *pdoc;

static void draw_screen(void)
{
    char line[32];
    ui_cls();
    ui_fill(4, 4, 22, 11, 3);
    ui_frame(4, 4, 22, 11);
    ui_text(11, 5, "- PRINT -", PB_UI);

    switch (ps) {
    case PS_NO_PRINTER:
        ui_text(6, 8, "no printer found", PB_UI);
        ui_text(6, 10, "check the cable +", PB_UI);
        ui_text(6, 11, "printer power", PB_UI);
        ui_text(6, 13, "B: back", PB_UI);
        break;
    case PS_EMPTY_DOC:
        ui_text(6, 9, "document is empty", PB_UI);
        ui_text(6, 13, "B: back", PB_UI);
        break;
    case PS_CONFIRM:
        siprintf(line, "%d page%s of paper", n_pages, n_pages == 1 ? "" : "s");
        ui_text(6, 8, line, PB_UI);
        ui_text(6, 11, "A: print  B: back", PB_UI);
        break;
    case PS_DONE:
        ui_text(6, 9, "done!", PB_UI);
        ui_text(6, 13, "B: back", PB_UI);
        break;
    case PS_ERROR:
        ui_text(6, 8, "printer error:", PB_UI);
        if (last_status & GBP_STATUS_PAPER_JAM)    ui_text(8, 9, "paper jam", PB_UI);
        else if (last_status & GBP_STATUS_LOW_BATTERY) ui_text(8, 9, "low battery", PB_UI);
        else if (last_status & GBP_STATUS_CHECKSUM_ERR) ui_text(8, 9, "checksum", PB_UI);
        else {
            siprintf(line, "status %02X", last_status);
            ui_text(8, 9, line, PB_UI);
        }
        ui_text(6, 13, "B: back", PB_UI);
        break;
    default:
        siprintf(line, "page %d/%d", cur_page + 1, n_pages);
        ui_text(6, 8, line, PB_UI);
        ui_text(6, 13, "B: cancel", PB_UI);
        break;
    }
}

static void draw_progress(void)
{
    // 18-cell bar: 2 cells per DATA packet sent
    for (int i = 0; i < 18; i++) {
        int filled = (ps == PS_WAIT) ? 18 : data_idx * 2;
        map_put(chrome_map(), 6 + i, 10, SOLID(i < filled ? 4 : 5), PB_SOLID);
    }
    if (ps == PS_WAIT) {
        ui_text(6, 11, saw_printing ? "printing...  " : "waiting...   ", PB_UI);
    }
}

void print_enter(void)
{
    obj_hide(&oam_mem[0]);
    pdoc = editor_doc();

    if (!gbp_link_detect(NULL)) {
        ps = PS_NO_PRINTER;
    } else if (pdoc->len == 0) {
        ps = PS_EMPTY_DOC;
    } else {
        layout_reflow(&plo, pdoc->text, pdoc->len, GBP_TILES_PER_ROW,
                      g_save.wrap_mode ? WRAP_MID_WORD : WRAP_POST_WORD);
        n_pages = gbp_page_count(&plo);
        cur_page = 0;
        ps = PS_CONFIRM;
    }
    draw_screen();
}

static u8 page_margins(void)
{
    if (n_pages == 1) return 0x13;              // feed before + after
    if (cur_page == 0) return 0x10;             // first: feed before only
    if (cur_page == n_pages - 1) return 0x03;   // last: feed after
    return 0x00;                                // middle: continuous
}

static bool status_error(u8 st)
{
    return (st & (GBP_STATUS_CHECKSUM_ERR | GBP_STATUS_PACKET_ERR |
                  GBP_STATUS_PAPER_JAM | GBP_STATUS_OTHER_ERR)) != 0;
}

static void start_page(void)
{
    gbp_compose_text_page(pdoc->text, pdoc->len, &plo,
                          cur_page * GBP_ROWS_PER_PAGE, page_buf);
    data_idx = 0;
    ps = PS_INIT;
    draw_screen();
}

void print_frame(int frame)
{
    (void)frame;
    u8 ka = 0, st = 0;

    if (key_hit(KEY_B)) {
        // between packets it is always safe to walk away; printer resets itself
        app_goto(ps == PS_CONFIRM || ps == PS_NO_PRINTER || ps == PS_EMPTY_DOC ||
                 ps == PS_DONE || ps == PS_ERROR ? ST_EDITOR : ST_MENU);
        return;
    }

    switch (ps) {
    case PS_CONFIRM:
        if (key_hit(KEY_A))
            start_page();
        break;

    case PS_INIT: {
        int n = gbp_packet_build(GBP_CMD_INIT, 0, 0, pkt_buf);
        gbp_link_send(pkt_buf, n, gbp_half_ticks(), &ka, &st);
        if (ka != GBP_KEEPALIVE) { last_status = st; ps = PS_ERROR; draw_screen(); break; }
        wait_frames = 12;              // ~200ms settle after INIT
        ps = PS_DATA;
        while (wait_frames--) VBlankIntrWait();
        break;
    }

    case PS_DATA: {
        int n = gbp_packet_build(GBP_CMD_DATA, page_buf + data_idx * GBP_DATA_PER_PKT,
                                 GBP_DATA_PER_PKT, pkt_buf);
        gbp_link_send(pkt_buf, n, gbp_half_ticks(), &ka, &st);
        if (ka != GBP_KEEPALIVE || status_error(st)) {
            last_status = st; ps = PS_ERROR; draw_screen(); break;
        }
        if (++data_idx >= GBP_PKTS_PER_PAGE)
            ps = PS_ARM;
        draw_progress();
        break;
    }

    case PS_ARM: {
        int n = gbp_packet_build(GBP_CMD_DATA, 0, 0, pkt_buf);
        gbp_link_send(pkt_buf, n, gbp_half_ticks(), &ka, &st);
        if (ka != GBP_KEEPALIVE) { last_status = st; ps = PS_ERROR; draw_screen(); break; }
        ps = PS_PRINT;
        break;
    }

    case PS_PRINT: {
        u8 args[4];
        gbp_print_args(page_margins(), 0x40, args);
        int n = gbp_packet_build(GBP_CMD_PRINT, args, 4, pkt_buf);
        gbp_link_send(pkt_buf, n, gbp_half_ticks(), &ka, &st);
        if (ka != GBP_KEEPALIVE || status_error(st)) {
            last_status = st; ps = PS_ERROR; draw_screen(); break;
        }
        saw_printing = false;
        wait_frames = 0;
        ps = PS_WAIT;
        draw_progress();
        break;
    }

    case PS_WAIT:
        // poll roughly every 3 frames (50ms) — keeps the printer awake
        if (++wait_frames % 3)
            break;
        {
            int n = gbp_packet_build(GBP_CMD_INQUIRY, 0, 0, pkt_buf);
            gbp_link_send(pkt_buf, n, gbp_half_ticks(), &ka, &st);
            last_status = st;
            if (ka != GBP_KEEPALIVE || status_error(st)) { ps = PS_ERROR; draw_screen(); break; }
            if (st & GBP_STATUS_PRINTING)
                saw_printing = true;
            bool finished = saw_printing ? !(st & GBP_STATUS_PRINTING)
                                         : (wait_frames > 60 * 30);
            draw_progress();
            if (finished) {
                if (++cur_page < n_pages)
                    start_page();
                else {
                    ps = PS_DONE;
                    draw_screen();
                }
            }
        }
        break;

    default:
        break;
    }
}
