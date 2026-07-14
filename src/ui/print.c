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
static int cur_row;         // first layout line of the chunk being sent
static int total_rows;      // real content lines in the whole job
static int chunk_pkts;      // DATA packets in the current chunk (rows/2)
static int data_idx;
static int wait_frames;
static int bad_polls;       // consecutive failed INQUIRYs during PS_WAIT
static bool saw_printing;
static bool link_lost;      // keepalive vanished mid-job (vs. printer status error)
static u8 last_status;

EWRAM_BSS static Layout plo;
EWRAM_BSS static u8 page_buf[GBP_PAGE_BYTES];
static u8 pkt_buf[GBP_PACKET_MAX];

static const Document *pdoc;
static const u8 *src_page;     // one-shot: raw image to print instead of the doc
static int src_rows;           // its height in tile rows
static const char *src_text;   // one-shot: text to print instead of the doc
static int src_len;
static AppState src_back;      // where B returns for that page/text
static const u8 *job_page;     // latched for this print job
static int job_rows;
static const char *job_text;   // latched text source (manual or editor doc)
static int job_len;
static AppState back_state;

void print_set_page(const void *page, int tile_rows, AppState back)
{
    src_page = page;
    src_rows = tile_rows;
    src_back = back;
}

void print_set_text(const char *text, int len, AppState back)
{
    src_text = text;
    src_len = len;
    src_back = back;
}

static void draw_screen(void)
{
    char line[32];
    ui_cls();
    ui_fill(4, 4, 22, 11, 3);
    ui_frame(4, 4, 22, 11);
    ui_text(11, 5, "- PRINT -", PB_UI);

    switch (ps) {
    case PS_NO_PRINTER:
        ui_text(6, 8, "No printer found.", PB_UI);
        ui_text(6, 10, "Check the cable +", PB_UI);
        ui_text(6, 11, "printer power.", PB_UI);
        ui_text(6, 13, "B: Back", PB_UI);
        break;
    case PS_EMPTY_DOC:
        ui_text(6, 9, "Document is empty.", PB_UI);
        ui_text(6, 13, "B: Back", PB_UI);
        break;
    case PS_CONFIRM:
        siprintf(line, "%d line%s of paper", total_rows, total_rows == 1 ? "" : "s");
        ui_text(6, 8, line, PB_UI);
        ui_text(6, 11, "A: Print  B: Back", PB_UI);
        break;
    case PS_DONE:
        ui_text(6, 9, "Done!", PB_UI);
        ui_text(6, 13, "B: Back", PB_UI);
        break;
    case PS_ERROR:
        if (link_lost) {
            ui_text(6, 7,  "Printer stopped", PB_UI);
            ui_text(6, 8,  "responding.", PB_UI);
            ui_text(6, 10, "Check the cable +", PB_UI);
            ui_text(6, 11, "printer batteries.", PB_UI);
        } else {
            ui_text(6, 8, "Printer error:", PB_UI);
            if (last_status & GBP_STATUS_PAPER_JAM)    ui_text(8, 9, "Paper jam", PB_UI);
            else if (last_status & GBP_STATUS_LOW_BATTERY) ui_text(8, 9, "Low battery", PB_UI);
            else if (last_status & GBP_STATUS_CHECKSUM_ERR) ui_text(8, 9, "Bad checksum", PB_UI);
            else {
                siprintf(line, "Status %02X", last_status);
                ui_text(8, 9, line, PB_UI);
            }
        }
        ui_text(6, 13, "B: Back", PB_UI);
        break;
    default:
        ui_text(6, 13, "B: Cancel", PB_UI);
        break;
    }
}

// Lines of the whole job sent to the printer so far (never past the content).
static int rows_sent(void)
{
    int sent = cur_row + ((ps == PS_WAIT) ? chunk_pkts : data_idx) * 2;
    return sent < total_rows ? sent : total_rows;
}

static void draw_progress(void)
{
    char line[32];
    int sent = rows_sent();
    siprintf(line, "Line %d/%d   ", sent, total_rows);
    ui_text(6, 8, line, PB_UI);
    // 18-cell bar spanning the whole job, by lines sent
    int filled = total_rows ? (sent * 18) / total_rows : 18;
    for (int i = 0; i < 18; i++)
        map_put(chrome_map(), 6 + i, 10, SOLID(i < filled ? 4 : 5), PB_SOLID);
    if (ps == PS_WAIT) {
        ui_text(6, 11, saw_printing ? "Printing...  " : "Waiting...   ", PB_UI);
    } else {
        ui_text(6, 11, "Sending...   ", PB_UI);
    }
}

void print_enter(void)
{
    obj_hide(&oam_mem[0]);
    pdoc = editor_doc();
    job_page = src_page;
    job_rows = src_rows;
    back_state = (src_page || src_text) ? src_back : ST_EDITOR;
    if (src_text) {
        job_text = src_text;
        job_len = src_len;
    } else {
        job_text = pdoc->text;
        job_len = pdoc->len;
    }
    src_page = NULL;
    src_text = NULL;

    if (!gbp_link_detect(NULL)) {
        ps = PS_NO_PRINTER;
    } else if (job_page) {
        total_rows = job_rows;
        cur_row = 0;
        ps = PS_CONFIRM;
    } else if (job_len == 0) {
        ps = PS_EMPTY_DOC;
    } else {
        layout_reflow(&plo, job_text, job_len, GBP_TILES_PER_ROW,
                      g_save.wrap_mode ? WRAP_MID_WORD : WRAP_POST_WORD);
        total_rows = plo.count;
        cur_row = 0;
        ps = PS_CONFIRM;
    }
    draw_screen();
}

static u8 chunk_margins(void)
{
    bool first = (cur_row == 0);
    bool last  = (cur_row + chunk_pkts * 2 >= total_rows);
    // one continuous document: feed before the first strip, after the last
    return (u8)((first ? 0x10 : 0x00) | (last ? 0x03 : 0x00));
}

static bool status_error(u8 st)
{
    return (st & (GBP_STATUS_CHECKSUM_ERR | GBP_STATUS_PACKET_ERR |
                  GBP_STATUS_PAPER_JAM | GBP_STATUS_OTHER_ERR)) != 0;
}

// Compose and start the next chunk: up to one printer buffer (18 rows) of
// content, trimmed to what's left and rounded up to the 2-row band the
// printer's head advances in. No blank page padding at the end.
static void start_chunk(void)
{
    int rows = total_rows - cur_row;
    if (rows > GBP_ROWS_PER_PAGE)
        rows = GBP_ROWS_PER_PAGE;
    chunk_pkts = (rows + 1) / 2;               // 2 rows per DATA packet
    if (job_page) {
        memset(page_buf, 0, GBP_PAGE_BYTES);   // blank-pad an odd final row
        memcpy(page_buf, job_page + cur_row * GBP_ROW_BYTES,
               rows * GBP_ROW_BYTES);
    } else {
        gbp_compose_text_page(job_text, job_len, &plo, cur_row, page_buf);
    }
    data_idx = 0;
    link_lost = false;
    ps = PS_INIT;
    draw_screen();
    draw_progress();
}

void print_frame(int frame)
{
    (void)frame;
    u8 ka = 0, st = 0;

    if (key_hit(KEY_B)) {
        // between packets it is always safe to walk away; printer resets itself
        app_goto(back_state);
        return;
    }

    switch (ps) {
    case PS_CONFIRM:
        if (key_hit(KEY_A))
            start_chunk();
        break;

    case PS_INIT: {
        int n = gbp_packet_build(GBP_CMD_INIT, 0, 0, pkt_buf);
        gbp_link_send(pkt_buf, n, GBP_HALF_NORMAL, &ka, &st);
        if (ka != GBP_KEEPALIVE) { link_lost = true; last_status = st; ps = PS_ERROR; draw_screen(); break; }
        wait_frames = 12;              // ~200ms settle after INIT
        ps = PS_DATA;
        while (wait_frames--) VBlankIntrWait();
        break;
    }

    case PS_DATA: {
        int n = gbp_packet_build(GBP_CMD_DATA, page_buf + data_idx * GBP_DATA_PER_PKT,
                                 GBP_DATA_PER_PKT, pkt_buf);
        gbp_link_send(pkt_buf, n, GBP_HALF_NORMAL, &ka, &st);
        if (ka != GBP_KEEPALIVE || status_error(st)) {
            link_lost = (ka != GBP_KEEPALIVE);
            last_status = st; ps = PS_ERROR; draw_screen(); break;
        }
        if (++data_idx >= chunk_pkts)
            ps = PS_ARM;
        draw_progress();
        break;
    }

    case PS_ARM: {
        int n = gbp_packet_build(GBP_CMD_DATA, 0, 0, pkt_buf);
        gbp_link_send(pkt_buf, n, GBP_HALF_NORMAL, &ka, &st);
        if (ka != GBP_KEEPALIVE) { link_lost = true; last_status = st; ps = PS_ERROR; draw_screen(); break; }
        ps = PS_PRINT;
        break;
    }

    case PS_PRINT: {
        u8 args[4];
        gbp_print_args(chunk_margins(), 0x40, args);
        int n = gbp_packet_build(GBP_CMD_PRINT, args, 4, pkt_buf);
        gbp_link_send(pkt_buf, n, GBP_HALF_NORMAL, &ka, &st);
        if (ka != GBP_KEEPALIVE || status_error(st)) {
            link_lost = (ka != GBP_KEEPALIVE);
            last_status = st; ps = PS_ERROR; draw_screen(); break;
        }
        saw_printing = false;
        wait_frames = 0;
        bad_polls = 0;
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
            gbp_link_send(pkt_buf, n, GBP_HALF_NORMAL, &ka, &st);
            bool finished = false;
            if (ka != GBP_KEEPALIVE) {
                // replies can garble while the motor runs — tolerate ~2s of
                // silence; after seeing it print, silence means the job is done
                if (++bad_polls < 40)
                    break;
                if (saw_printing) {
                    finished = true;
                } else {
                    link_lost = true; last_status = st;
                    ps = PS_ERROR; draw_screen(); break;
                }
            } else {
                bad_polls = 0;
                last_status = st;
                if (status_error(st)) { ps = PS_ERROR; draw_screen(); break; }
                if (st & GBP_STATUS_PRINTING)
                    saw_printing = true;
                finished = saw_printing ? !(st & GBP_STATUS_PRINTING)
                                        : (wait_frames > 60 * 30);
            }
            draw_progress();
            if (finished) {
                cur_row += chunk_pkts * 2;
                if (cur_row < total_rows)
                    start_chunk();
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
