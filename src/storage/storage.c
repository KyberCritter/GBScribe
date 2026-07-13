#include "storage.h"

static u32 size_;
static const char *name_ = "none";
static void (*read_)(u32, void *, u32);
static bool (*write_)(u32, const void *, u32);

void storage_init(void)
{
    u32 fs = flash_probe_size();
    if (fs) {
        read_ = flash_read;
        write_ = flash_write;
        size_ = fs;
        name_ = (fs >= 128 * 1024) ? "flash128" : "flash64";
        return;
    }
    u32 ss = sram_probe_size();
    read_ = sram_read;
    write_ = sram_write;
    if (ss) {
        size_ = ss;
        name_ = (ss >= 64 * 1024) ? "sram64" : "sram32";
    } else {
        size_ = 32 * 1024;   // keep the app functional; writes will report failure
        name_ = "none";
    }
}

u32 storage_size(void)         { return size_; }
const char *storage_name(void) { return name_; }

void storage_read(u32 off, void *dst, u32 len)        { read_(off, dst, len); }
bool storage_write(u32 off, const void *src, u32 len) { return write_(off, src, len); }
