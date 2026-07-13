#include <string.h>
#include <tonc.h>
#include "app.h"
#include "ui/ui.h"
#include "ui/screen.h"
#include "core/charset.h"
#include "gfx/font.h"
#include "gfx/qr.h"

// QR glyph tiles live in CBB0 after the font tiles
#define QR_TILE_BASE 160
#define PB_QR 5                  // palette bank: [1]=black, [2]=white

static int page;                 // 0 = about, 1 = legal acknowledgements

static void draw_about(void)
{
    ui_cls();

    // load QR tiles once per entry (cheap) and set its palette
    memcpy32(&tile_mem[0][QR_TILE_BASE], qr_tiles, QR_TILE_W * QR_TILE_W * 8);
    pal_bg_mem[16 * PB_QR + 1] = RGB15(1, 1, 2);
    pal_bg_mem[16 * PB_QR + 2] = RGB15(31, 31, 31);

    // QR block: right side, rows 1..13
    for (int ty = 0; ty < QR_TILE_W; ty++)
        for (int tx = 0; tx < QR_TILE_W; tx++)
            map_put(text_map(), 16 + tx, 1 + ty,
                    QR_TILE_BASE + ty * QR_TILE_W + tx, PB_QR);

    ui_text(1, 2,  "GBScribe", PB_UI);
    ui_logo(11, 1);

    ui_text(1, 4,  "by Scott", PB_UI);
    ui_text(1, 5,  "Ratchford", PB_UI);

    ui_text(1, 7,  "is licensed", PB_UI);
    ui_text(1, 8,  "under the", PB_UI);
    ui_text(1, 9,  "MIT License", PB_UI);

    ui_text(1, 11, "A: print QR", PB_UI);
    ui_text(1, 12, "B: back", PB_UI);
    map_put(text_map(), 1, 13, CH_ARR_DOWN, PB_UI);
    ui_text(3, 13, "legal info", PB_UI);

    ui_text(1, 16, "For more information,", PB_UI);
    ui_text(1, 17, "visit:", PB_UI);
    ui_text(1, 18, QR_URL, PB_UI);
}

static void draw_legal(void)
{
    ui_cls();

    ui_logo(1, 1);
    ui_text(4, 1,  "GBScribe", PB_UI);
    ui_text(4, 2,  "by Scott Ratchford", PB_UI);

    ui_text(1, 4,  "is licensed under the", PB_UI);
    ui_text(1, 5,  "MIT License - full text in", PB_UI);
    ui_text(1, 6,  "LICENSE.md in the repo.", PB_UI);

    ui_text(1, 8,  "QR Code is a trademark", PB_UI);
    ui_text(1, 9,  "of DENSO WAVE Inc.", PB_UI);

    ui_text(1, 11, "Not affiliated with or", PB_UI);
    ui_text(1, 12, "endorsed by Nintendo.", PB_UI);

    ui_text(1, 14, "Built with devkitARM,", PB_UI);
    ui_text(1, 15, "libtonc, libgba, Maxmod,", PB_UI);
    ui_text(1, 16, "and font8x8 (pub domain).", PB_UI);
    ui_text(1, 17, "License texts: LICENSES/", PB_UI);
    ui_text(1, 18, "in the repo.", PB_UI);

    map_put(text_map(), 1, 19, CH_ARR_UP, PB_UI);
    ui_text(3, 19, "back", PB_UI);
}

void credits_enter(void)
{
    obj_hide(&oam_mem[0]);
    page = 0;
    draw_about();
}

void credits_frame(int frame)
{
    (void)frame;
    if (page == 0) {
        if (key_hit(KEY_DOWN)) { page = 1; draw_legal(); return; }
        if (key_hit(KEY_B) || key_hit(KEY_START)) {
            app_goto(ST_MENU);
            return;
        }
        if (key_hit(KEY_A)) {
            print_set_page(qr_page, ST_CREDITS);
            app_goto(ST_PRINT);
        }
    } else {
        if (key_hit(KEY_UP) || key_hit(KEY_B)) { page = 0; draw_about(); return; }
        if (key_hit(KEY_START))
            app_goto(ST_MENU);
    }
}
