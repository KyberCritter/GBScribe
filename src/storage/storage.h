// Storage backend interface with RUNTIME detection: probes for a flash chip
// (128K/64K) first, then falls back to SRAM (64K/32K). The EZ-Flash Omega DE
// doesn't emulate the flash command protocol for homebrew, so compile-time
// FLASH1M-only storage failed on the real cart (mGBA emulates it fine).
#ifndef STORAGE_STORAGE_H
#define STORAGE_STORAGE_H

#include <tonc.h>

void storage_init(void);            // probe hardware, pick a backend
u32  storage_size(void);            // usable bytes (0 if nothing writable)
const char *storage_name(void);     // "flash128", "sram64", ... for the title UI
void storage_read(u32 off, void *dst, u32 len);
bool storage_write(u32 off, const void *src, u32 len);   // verified

// backend internals (storage.c wires them up)
u32  flash_probe_size(void);        // 0 = no flash protocol answered
void flash_read(u32 off, void *dst, u32 len);
bool flash_write(u32 off, const void *src, u32 len);
u32  sram_probe_size(void);         // 0 = not even SRAM is writable
void sram_read(u32 off, void *dst, u32 len);
bool sram_write(u32 off, const void *src, u32 len);

#endif
