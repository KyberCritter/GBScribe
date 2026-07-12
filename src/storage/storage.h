// Storage backend interface. Default: FLASH1M (128KB). Build with -DSTORAGE_SRAM
// for the 32KB SRAM fallback (see PLAN.md "Risks").
#ifndef STORAGE_STORAGE_H
#define STORAGE_STORAGE_H

#include <tonc.h>

#ifdef STORAGE_SRAM
#define STORAGE_SIZE (32 * 1024)
#else
#define STORAGE_SIZE (128 * 1024)
#endif

void storage_init(void);
void storage_read(u32 off, void *dst, u32 len);
// Arbitrary offset/length; flash backend does sector read-merge-erase-program.
// Returns true on success (verified).
bool storage_write(u32 off, const void *src, u32 len);

#endif
