// FLASH1M (128KB) backend: banked 2x64KB, 4KB sectors, byte program.
// Protocol per GBATEK; works on mGBA's FLASH1M emulation and the EZ-Flash
// Omega DE's FRAM-backed flash emulation.
#ifndef STORAGE_SRAM

#include <string.h>
#include "storage.h"

// mGBA / real carts detect the save type by scanning the ROM for this string.
const char flash_save_id[] __attribute__((used)) = "FLASH1M_V103";

#define FLASH_BASE  ((vu8 *)0x0E000000)
#define SECTOR_SIZE 4096
#define BANK_SIZE   0x10000

static int cur_bank = -1;

static void flash_cmd(u8 c)
{
    FLASH_BASE[0x5555] = 0xAA;
    FLASH_BASE[0x2AAA] = 0x55;
    FLASH_BASE[0x5555] = c;
}

static void flash_bank(int bank)
{
    if (bank == cur_bank)
        return;
    flash_cmd(0xB0);
    FLASH_BASE[0] = (u8)bank;
    cur_bank = bank;
}

void storage_init(void)
{
    // touch the ID string so -Wl,--gc-sections cannot drop it
    volatile const char *keep = flash_save_id;
    (void)*keep;
    cur_bank = -1;
    flash_bank(0);
}

void storage_read(u32 off, void *dst, u32 len)
{
    u8 *d = dst;
    while (len) {
        flash_bank(off / BANK_SIZE);
        u32 in_bank = off % BANK_SIZE;
        u32 n = BANK_SIZE - in_bank;
        if (n > len) n = len;
        for (u32 i = 0; i < n; i++)
            d[i] = FLASH_BASE[in_bank + i];
        d += n; off += n; len -= n;
    }
}

static bool wait_byte(vu8 *p, u8 want)
{
    for (u32 t = 0; t < 500000; t++)
        if (*p == want)
            return true;
    return false;
}

// erase the 4KB sector at byte offset `sec_off` within the current bank
static bool erase_sector(u32 in_bank)
{
    flash_cmd(0x80);
    FLASH_BASE[0x5555] = 0xAA;
    FLASH_BASE[0x2AAA] = 0x55;
    FLASH_BASE[in_bank] = 0x30;
    return wait_byte(&FLASH_BASE[in_bank], 0xFF);
}

static bool program_byte(u32 in_bank, u8 b)
{
    flash_cmd(0xA0);
    FLASH_BASE[in_bank] = b;
    return wait_byte(&FLASH_BASE[in_bank], b);
}

EWRAM_BSS static u8 sector_buf[SECTOR_SIZE];

bool storage_write(u32 off, const void *src, u32 len)
{
    const u8 *s = src;
    while (len) {
        u32 sec_base = off & ~(u32)(SECTOR_SIZE - 1);
        u32 in_sec = off - sec_base;
        u32 n = SECTOR_SIZE - in_sec;
        if (n > len) n = len;

        storage_read(sec_base, sector_buf, SECTOR_SIZE);
        if (memcmp(sector_buf + in_sec, s, n) != 0) {
            memcpy(sector_buf + in_sec, s, n);

            flash_bank(sec_base / BANK_SIZE);
            u32 in_bank = sec_base % BANK_SIZE;
            if (!erase_sector(in_bank))
                return false;
            for (u32 i = 0; i < SECTOR_SIZE; i++)
                if (!program_byte(in_bank + i, sector_buf[i]))
                    return false;
        }
        s += n; off += n; len -= n;
    }
    return true;
}

#endif // !STORAGE_SRAM
