// SRAM backend (the EZ-Flash Omega DE path: its FRAM behaves as plain SRAM).
// Size is probed at runtime: 64K if offset 0x8000 is distinct storage, 32K if
// it mirrors offset 0, 0 if nothing holds a written byte.
#include "storage.h"

const char sram_save_id[] __attribute__((used)) = "SRAM_V113";

#define SRAM_BASE ((vu8 *)0x0E000000)

u32 sram_probe_size(void)
{
    volatile const char *keep = sram_save_id;
    (void)*keep;

    vu8 *S = SRAM_BASE;
    u8 a = S[16];
    S[16] = 0x5A;
    if (S[16] != 0x5A) {
        S[16] = a;
        return 0;                        // nothing writable here
    }

    u8 b = S[0x8000 + 16];
    S[0x8000 + 16] = 0xA5;
    bool mirrored = (S[16] == 0xA5);
    bool upper_ok = (S[0x8000 + 16] == 0xA5);
    if (!mirrored)
        S[0x8000 + 16] = b;              // restore upper copy
    S[16] = a;                           // restore lower copy

    return (mirrored || !upper_ok) ? 32 * 1024 : 64 * 1024;
}

void sram_read(u32 off, void *dst, u32 len)
{
    u8 *d = dst;
    for (u32 i = 0; i < len; i++)
        d[i] = SRAM_BASE[off + i];
}

bool sram_write(u32 off, const void *src, u32 len)
{
    const u8 *s = src;
    for (u32 i = 0; i < len; i++)
        SRAM_BASE[off + i] = s[i];
    for (u32 i = 0; i < len; i++)
        if (SRAM_BASE[off + i] != s[i])
            return false;
    return true;
}
