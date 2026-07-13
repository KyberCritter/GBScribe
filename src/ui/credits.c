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

void credits_enter(void)
{
    obj_hide(&oam_mem[0]);
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

    ui_text(1, 2,  "GBScribe", PB_SEL);
    ui_text(1, 4,  "by Scott", PB_UI);
    ui_text(1, 5,  "Ratchford", PB_UI);

    ui_text(1, 8,  "scan me \x04", PB_UI);

    ui_text(1, 11, "A: print QR", PB_UI);
    ui_text(1, 12, "B: back", PB_UI);

    ui_text(1, 15, QR_URL, PB_SEL);
    ui_text(1, 16, "QR Code is a trademark", PB_UI);
    ui_text(1, 17, "of DENSO WAVE Inc.", PB_UI);
    ui_text(1, 18, "Not affiliated with or", PB_UI);
    ui_text(1, 19, "endorsed by Nintendo.", PB_UI);
}

void credits_frame(int frame)
{
    (void)frame;
    if (key_hit(KEY_B) || key_hit(KEY_START)) {
        app_goto(ST_MENU);
        return;
    }
    if (key_hit(KEY_A)) {
        print_set_page(qr_page, ST_CREDITS);
        app_goto(ST_PRINT);
    }
}
