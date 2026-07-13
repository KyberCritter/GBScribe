#include <tonc.h>
#include "sound.h"

// Audio is disabled for now: on real hardware there is a constant crackle
// (Maxmod's TM0/TM1 FIFO timing) that we haven't fully solved. Set to 1 to
// re-enable Maxmod once the crackle is fixed — see STATUS.md "Known issues".
#define AUDIO_ENABLED 0

#if AUDIO_ENABLED

#include <maxmod.h>
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

#else

void sound_init(void)   {}
void sound_frame(void)  {}
void sound_pause(void)  {}
void sound_resume(void) {}
void sfx_click(void)    {}
void sfx_ding(void)     {}

#endif
