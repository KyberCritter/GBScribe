// Print scene: drives an AdvancePrint job (INIT/DATA/ARM/PRINT/WAIT) with
// progress UI. All printer protocol work lives in the AdvancePrint library
// (lib/AdvancePrint); this file only latches the source (document/manual/raw
// page), advances the job once per frame, and renders the screen.
#include <string.h>
#include <stdio.h>
#include <tonc.h>
#include "app.h"
#include "ui/ui.h"
#include "ui/screen.h"
#include "ui/editor.h"
#include "core/charset.h"
#include <advanceprint/advanceprint.h>
#include "storage/storage.h"
#include "storage/save.h"

enum {
    PS_NO_PRINTER,
    PS_EMPTY_DOC,
    PS_CONFIRM,
    PS_RUNNING,     // ap_job_update drives INIT..WAIT internally
    PS_DONE,
    PS_ERROR,
};

EWRAM_BSS static ApJob pjob;
EWRAM_BSS static ApText ptext;      // wrapped line table for text jobs

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

static int ps;
static int total_rows;         // content lines for the confirm prompt
static ApError last_error;
static u8 last_status;

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
        if (last_error == AP_ERR_LINK_LOST) {
            ui_text(6, 7,  "Printer stopped", PB_UI);
            ui_text(6, 8,  "responding.", PB_UI);
            ui_text(6, 10, "Check the cable +", PB_UI);
            ui_text(6, 11, "printer batteries.", PB_UI);
        } else {
            ui_text(6, 8, "Printer error:", PB_UI);
            if (last_status & AP_STATUS_PAPER_JAM)         ui_text(8, 9, "Paper jam", PB_UI);
            else if (last_status & AP_STATUS_LOW_BATTERY)  ui_text(8, 9, "Low battery", PB_UI);
            else if (last_status & AP_STATUS_CHECKSUM_ERR) ui_text(8, 9, "Bad checksum", PB_UI);
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

static void draw_progress(void)
{
    char line[32];
    int sent = pjob.rows_sent;
    int tot = pjob.rows_total;
    siprintf(line, "Line %d/%d   ", sent, tot);
    ui_text(6, 8, line, PB_UI);
    // 18-cell bar spanning the whole job, by lines sent
    int filled = tot ? (sent * 18) / tot : 18;
    for (int i = 0; i < 18; i++)
        map_put(chrome_map(), 6 + i, 10, SOLID(i < filled ? 4 : 5), PB_SOLID);
    ui_text(6, 11, pjob.printing ? "Printing...  " : "Sending...   ", PB_UI);
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

    if (!ap_job_probe(NULL)) {
        ps = PS_NO_PRINTER;
    } else if (job_page) {
        total_rows = job_rows;
        ps = PS_CONFIRM;
    } else if (job_len == 0) {
        ps = PS_EMPTY_DOC;
    } else {
        ap_text_reflow(&ptext, job_text, job_len, AP_TILES_PER_ROW,
                       g_save.wrap_mode ? AP_WRAP_MID_WORD : AP_WRAP_POST_WORD);
        total_rows = ptext.count;
        ps = PS_CONFIRM;
    }
    draw_screen();
}

static void start_job(void)
{
    ApJobConfig cfg;
    memset(&cfg, 0, sizeof cfg);
    if (job_page) {
        cfg.image = job_page;
        cfg.image_rows = job_rows;
    } else {
        cfg.text = job_text;
        cfg.lines = &ptext;
    }
    cfg.exposure = 0x40;

    if (!ap_job_begin(&pjob, &cfg)) {
        last_error = pjob.error;
        last_status = pjob.status;
        ps = (last_error == AP_ERR_NO_PRINTER) ? PS_NO_PRINTER : PS_ERROR;
        draw_screen();
        return;
    }
    last_error = AP_OK;
    ps = PS_RUNNING;
    draw_screen();
    draw_progress();
}

void print_frame(int frame)
{
    (void)frame;

    if (key_hit(KEY_B)) {
        // between packets it is always safe to walk away; printer resets itself
        app_goto(back_state);
        return;
    }

    switch (ps) {
    case PS_CONFIRM:
        if (key_hit(KEY_A))
            start_job();
        break;

    case PS_RUNNING:
        // AdvancePrint emits at most one packet per update: the UI stays live
        // and B can always cancel between packets.
        ap_job_update(&pjob);
        if (pjob.phase == AP_PHASE_DONE) {
            ps = PS_DONE;
            draw_screen();
        } else if (pjob.phase != AP_PHASE_RUNNING) {
            last_error = pjob.error;
            last_status = pjob.status;
            ps = PS_ERROR;
            draw_screen();
        } else {
            draw_progress();
        }
        break;

    default:
        break;
    }
}
