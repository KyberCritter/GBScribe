// Editor screen: document + layout + scrolling page view + keyboard input.
#ifndef UI_EDITOR_H
#define UI_EDITOR_H

#include <tonc.h>

void editor_init(void);
void editor_frame(int frame);   // call once per VBlank with key state polled

#endif
