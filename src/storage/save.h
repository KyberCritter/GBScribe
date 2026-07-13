// Save format: 1KB header + 8 equal slots filling the rest of storage.
// Slot size depends on the runtime-detected backend:
//   flash128: 16,256 B — a full 16,000-char doc fits
//   sram64:    8,064 B — drawings (5,760) fit, docs truncate
//   sram32/flash64: 3,968/8,064 B — small
#ifndef STORAGE_SAVE_H
#define STORAGE_SAVE_H

#include <tonc.h>
#include "core/document.h"

#define SLOT_COUNT     8
#define SLOT_NAME_LEN  14
#define SAVE_HDR_SIZE  1024

u32 save_slot_size(void);     // payload bytes per slot for the active backend

enum { SLOT_EMPTY = 0, SLOT_DOC = 1, SLOT_DRAWING = 2 };

typedef struct {
    u8  type;
    u8  rsvd;
    u16 len;                  // doc: chars; drawing: bytes
    char name[SLOT_NAME_LEN];
} SlotDir;

typedef struct {
    u32 magic;
    u16 version;
    u8  wrap_mode;            // 0 = post-word, 1 = mid-word
    u8  print_speed;          // 0 = fast (16kHz), 1 = normal (8kHz)
    u8  draw_pal;             // drawing display palette (M6)
    u8  rsvd[7];
    SlotDir dir[SLOT_COUNT];
} SaveHeader;

extern SaveHeader g_save;     // in-RAM copy; save_flush_header() persists it

#define DRAWING_BYTES 5760            // 160x144 GB 2bpp canvas

void save_init(void);         // load header (format defaults if magic missing)
bool save_flush_header(void);
bool save_doc(int slot, const Document *d);
bool load_doc(int slot, Document *d);
bool save_drawing(int slot, const u8 *canvas, const char *name);
bool load_drawing(int slot, u8 *canvas);
bool save_delete(int slot);

#endif
