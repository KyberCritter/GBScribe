#include <string.h>
#include "storage/storage.h"
#include "storage/save.h"
#include "core/charset.h"

#define SAVE_MAGIC   0x43534247u   // "GBSC"
#define SAVE_VERSION 1

SaveHeader g_save;

static u32 slot_off(int slot) { return SAVE_HDR_SIZE + (u32)slot * SLOT_SIZE; }

void save_init(void)
{
    storage_init();
    storage_read(0, &g_save, sizeof g_save);
    if (g_save.magic != SAVE_MAGIC || g_save.version != SAVE_VERSION) {
        memset(&g_save, 0, sizeof g_save);
        g_save.magic = SAVE_MAGIC;
        g_save.version = SAVE_VERSION;
        // defaults: post-word wrap, fast print
        // header is persisted lazily on first save/settings change
    }
}

bool save_flush_header(void)
{
    return storage_write(0, &g_save, sizeof g_save);
}

static void derive_name(const Document *d, char *out)
{
    int n = 0;
    for (int i = 0; i < d->len && n < SLOT_NAME_LEN; i++) {
        char c = d->text[i];
        if (c == CH_NEWLINE)
            break;
        out[n++] = c;
    }
    while (n < SLOT_NAME_LEN)
        out[n++] = 0;
}

bool save_doc(int slot, const Document *d)
{
    int len = d->len;
    if (len > (int)SLOT_SIZE)
        len = SLOT_SIZE;                       // sram fallback truncates

    if (!storage_write(slot_off(slot), d->text, len))
        return false;

    SlotDir *e = &g_save.dir[slot];
    e->type = SLOT_DOC;
    e->len = (u16)len;
    derive_name(d, e->name);
    return save_flush_header();
}

bool load_doc(int slot, Document *d)
{
    const SlotDir *e = &g_save.dir[slot];
    if (e->type != SLOT_DOC)
        return false;
    int len = e->len;
    if (len > DOC_MAX)
        len = DOC_MAX;
    storage_read(slot_off(slot), d->text, len);
    d->len = len;
    d->caret = len;
    return true;
}

bool save_drawing(int slot, const u8 *canvas, const char *name)
{
    if (DRAWING_BYTES > SLOT_SIZE)
        return false;                          // sram fallback can't hold one

    if (!storage_write(slot_off(slot), canvas, DRAWING_BYTES))
        return false;

    SlotDir *e = &g_save.dir[slot];
    e->type = SLOT_DRAWING;
    e->len = DRAWING_BYTES;
    for (int i = 0; i < SLOT_NAME_LEN; i++)
        e->name[i] = (name && name[i]) ? name[i] : 0;
    return save_flush_header();
}

bool load_drawing(int slot, u8 *canvas)
{
    const SlotDir *e = &g_save.dir[slot];
    if (e->type != SLOT_DRAWING)
        return false;
    storage_read(slot_off(slot), canvas, DRAWING_BYTES);
    return true;
}

bool save_delete(int slot)
{
    memset(&g_save.dir[slot], 0, sizeof(SlotDir));
    return save_flush_header();
}
