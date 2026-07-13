// Flash backend: banked 128K (or single-bank 64K), 4KB sectors, byte program.
// Protocol per GBATEK. Presence is detected at runtime via the software-ID
// command — carts that expose plain SRAM never answer it.
#include <string.h>
#include "storage.h"

// mGBA / flash carts detect the preferred save type by scanning the ROM.
const char flash_save_id[] __attribute__((used)) = "FLASH1M_V103";

#define FLASH_BASE  ((vu8 *)0x0E000000)
#define SECTOR_SIZE 4096
#define BANK_SIZE   0x10000

static int cur_bank = -1;
static u32 flash_size;

static void flash_cmd(u8 c)
{
    FLASH_BASE[0x5555] = 0xAA;
    FLASH_BASE[0x2AAA] = 0x55;
    FLASH_BASE[0x5555] = c;
}

u32 flash_probe_size(void)
{
    volatile const char *keep = flash_save_id;   // defeat --gc-sections
    (void)*keep;

    // remember bytes our command writes would clobber if this is plain SRAM
    u8 s1 = FLASH_BASE[0x5555], s2 = FLASH_BASE[0x2AAA];
    u8 n0 = FLASH_BASE[0], n1 = FLASH_BASE[1];

    flash_cmd(0x90);                     // enter software ID mode
    u8 man = FLASH_BASE[0], dev = FLASH_BASE[1];
    flash_cmd(0xF0);                     // exit ID mode

    if (man == n0 && dev == n1) {
        // nothing answered: restore possible SRAM bytes and report no flash
        FLASH_BASE[0x5555] = s1;
        FLASH_BASE[0x2AAA] = s2;
        return 0;
    }

    switch ((man << 8) | dev) {
    case 0x6213:                         // Sanyo 128K
    case 0xC209:                         // Macronix 128K
        flash_size = 128 * 1024;
        break;
    case 0xBFD4:                         // SST 64K
    case 0x1F3D:                         // Atmel 64K
    case 0x321B:                         // Panasonic 64K
    case 0xC21C:                         // Macronix 64K
        flash_size = 64 * 1024;
        break;
    default:                             // unknown chip: don't trust the protocol
        FLASH_BASE[0x5555] = s1;
        FLASH_BASE[0x2AAA] = s2;
        return 0;
    }
    cur_bank = -1;
    return flash_size;
}

static void flash_bank(int bank)
{
    if (flash_size <= BANK_SIZE || bank == cur_bank)
        return;
    flash_cmd(0xB0);
    FLASH_BASE[0] = (u8)bank;
    cur_bank = bank;
}

void flash_read(u32 off, void *dst, u32 len)
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

bool flash_write(u32 off, const void *src, u32 len)
{
    const u8 *s = src;
    while (len) {
        u32 sec_base = off & ~(u32)(SECTOR_SIZE - 1);
        u32 in_sec = off - sec_base;
        u32 n = SECTOR_SIZE - in_sec;
        if (n > len) n = len;

        flash_read(sec_base, sector_buf, SECTOR_SIZE);
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
