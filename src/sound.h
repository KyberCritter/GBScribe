// Maxmod wrapper: module playback + UI sound effects.
#ifndef SOUND_H
#define SOUND_H

void sound_init(void);     // after irq_init; starts the looping theme
void sound_frame(void);    // once per frame, after VBlankIntrWait
void sound_pause(void);    // stop music during printing (IRQ timing)
void sound_resume(void);
void sfx_click(void);
void sfx_ding(void);

#endif
