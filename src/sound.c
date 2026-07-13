#include <tonc.h>
#include <maxmod.h>
#include "sound.h"
#include "soundbank.h"

extern const u8 soundbank_bin[];   // linked from build/soundbank.bin via bin2s

static bool paused;

void sound_init(void)
{
    irq_add(II_VBLANK, mmVBlank);
    mmInitDefault((mm_addr)soundbank_bin, 8);
    mmSetModuleVolume(360);
    mmStart(MOD_THEME, MM_PLAY_LOOP);
}

void sound_frame(void)
{
    mmFrame();
}

void sound_pause(void)
{
    if (!paused) {
        mmPause();
        paused = true;
    }
}

void sound_resume(void)
{
    if (paused) {
        mmResume();
        paused = false;
    }
}

void sfx_click(void)
{
    mmEffect(SFX_CLICK);
}

void sfx_ding(void)
{
    mmEffect(SFX_DING);
}
