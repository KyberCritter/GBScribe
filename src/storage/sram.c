// 32KB SRAM fallback backend (build with -DSTORAGE_SRAM).
#ifdef STORAGE_SRAM

#include "storage.h"

const char sram_save_id[] __attribute__((used)) = "SRAM_V113";

#define SRAM_BASE ((vu8 *)0x0E000000)

void storage_init(void)
{
    volatile const char *keep = sram_save_id;
    (void)*keep;
}

void storage_read(u32 off, void *dst, u32 len)
{
    u8 *d = dst;
    for (u32 i = 0; i < len; i++)
        d[i] = SRAM_BASE[off + i];
}

bool storage_write(u32 off, const void *src, u32 len)
{
    const u8 *s = src;
    for (u32 i = 0; i < len; i++)
        SRAM_BASE[off + i] = s[i];
    for (u32 i = 0; i < len; i++)
        if (SRAM_BASE[off + i] != s[i])
            return false;
    return true;
}

#endif // STORAGE_SRAM
