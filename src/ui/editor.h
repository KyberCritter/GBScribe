// Editor screen: document + layout + scrolling page view + keyboard input.
#ifndef UI_EDITOR_H
#define UI_EDITOR_H

#include <tonc.h>
#include "core/document.h"

void editor_init(void);        // once at boot
void editor_enter(void);       // scene entry: full redraw (reads wrap setting)
void editor_frame(int frame);  // once per VBlank with key state polled
void editor_new(void);         // clear the document
void editor_reloaded(void);    // call after loading into editor_doc()
Document *editor_doc(void);

#endif
