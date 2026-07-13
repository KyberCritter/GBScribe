// GBScribe internal character set (see tools/mkfont.py for the glyph source).
// Codes 0x20..0x7E are ASCII; specials live in the C0 range.
#ifndef CORE_CHARSET_H
#define CORE_CHARSET_H

enum {
    CH_BLANK    = 0x00,
    CH_ARR_UP   = 0x01,
    CH_ARR_DOWN = 0x02,
    CH_ARR_LEFT = 0x03,
    CH_ARR_RIGHT= 0x04,
    CH_SQ_FILL  = 0x05,
    CH_SQ_HOLLOW= 0x06,
    CH_CIRC_FILL= 0x07,
    CH_CIRC_HOLLOW=0x08,
    CH_TRI_FILL = 0x09,
    CH_NEWLINE  = 0x0A,     // control: line break (blank glyph)
    CH_TRI_HOLLOW=0x0B,
    CH_DIA_FILL = 0x0C,
    CH_DIA_HOLLOW=0x0D,
    CH_BOX_H    = 0x0E,     // ─
    CH_BOX_V    = 0x0F,     // │
    CH_BOX_TL   = 0x10,     // ┌
    CH_BOX_TR   = 0x11,     // ┐
    CH_BOX_BL   = 0x12,     // └
    CH_BOX_BR   = 0x13,     // ┘
    CH_BOX_LT   = 0x14,     // ├
    CH_BOX_RT   = 0x15,     // ┤
    CH_BOX_TT   = 0x16,     // ┬
    CH_BOX_BT   = 0x17,     // ┴
    CH_BOX_X    = 0x18,     // ┼
    CH_ICON_PENCIL = 0x19,
    CH_ICON_ERASER = 0x1A,
    CH_ICON_LINE   = 0x1B,
    CH_ICON_RECT   = 0x1C,
    CH_ICON_STAMP  = 0x1D,
};

#endif
