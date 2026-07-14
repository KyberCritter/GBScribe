#!/usr/bin/env python3
"""Pre-generate the printer test page (title chooser: "Print test page").

A variable-height contiguous strip (the print path streams it in 18-row
chunks): border, GBScribe logo at 4x, title, the four print shades as labeled
bands, alignment patterns, the credits QR code + URL, and every character on
the on-screen keyboard (all four layers, rendered with the real print font).

Emits src/gfx/testpage.c/h:
  testpage_page  - TESTPAGE_ROWS x 320 bytes of GB 2bpp tile rows (160px wide)
Plus .scratchwork/testpage_preview.png for inspection.
"""
import os, re, struct, zlib
import segno
from mklogo import logo_grid

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# ---- the app's print font: decode src/gfx/font2bpp.c (128 glyphs x 16 bytes)
# so shape/box glyphs print exactly as they do from the editor
def load_font2bpp():
    src = open(os.path.join(ROOT, "src/gfx/font2bpp.c")).read()
    data = [int(x, 16) for x in re.findall(r"0x[0-9A-Fa-f]{2}", src)]
    assert len(data) == 128 * 16
    # rows[code][y] = plane-0 byte, MSB = leftmost pixel
    return [[data[c * 16 + y * 2] for y in range(8)] for c in range(128)]

FONT = load_font2bpp()

def qr_url():
    src = open(os.path.join(ROOT, "src/gfx/qr.h")).read()
    return re.search(r'#define QR_URL "([^"]+)"', src).group(1)

PAGE_W, TILES_PER_ROW = 160, 20
MAX_H = 1024
page = bytearray(TILES_PER_ROW * 16 * (MAX_H // 8))

def put_px(px, py, shade):
    """set one pixel to GB shade 0..3 (0 = white paper)"""
    ti = (py // 8) * TILES_PER_ROW + px // 8
    off = ti * 16 + (py % 8) * 2
    bit = 0x80 >> (px % 8)
    if shade & 1: page[off]     |= bit
    if shade & 2: page[off + 1] |= bit

def rect(x, y, w, h, shade):
    for yy in range(y, y + h):
        for xx in range(x, x + w):
            put_px(xx, yy, shade)

def outline(x, y, w, h, shade):
    for xx in range(x, x + w):
        put_px(xx, y, shade); put_px(xx, y + h - 1, shade)
    for yy in range(y, y + h):
        put_px(x, yy, shade); put_px(x + w - 1, yy, shade)

def glyph(code, x, y, shade=3):
    for yy in range(8):
        b = FONT[code & 0x7F][yy]
        for xx in range(8):
            if b & (0x80 >> xx):
                put_px(x + xx, y + yy, shade)

def text(s, y, shade=3):
    """centered row of glyphs; accepts str or list of charset codes"""
    codes = [ord(c) for c in s] if isinstance(s, str) else s
    x0 = (PAGE_W - len(codes) * 8) // 2
    for ci, c in enumerate(codes):
        glyph(c, x0 + ci * 8, y, shade)

y = 6  # layout cursor

# logo at 4x: 16x16 grid -> 64x64 px, top center; sketch shades ARE print shades
lg = logo_grid()
lx = (PAGE_W - 64) // 2
for gy in range(16):
    for gx in range(16):
        if lg[gy][gx]:
            rect(lx + gx * 4, y + gy * 4, 4, 4, lg[gy][gx])
y += 64 + 6

text("GBSCRIBE", y);  y += 10
text("TEST PAGE", y); y += 14

# the four print shades: labeled 20px bands, outlined so shade 0 shows
bx0 = (PAGE_W - 4 * 32) // 2
for i in range(4):
    x = bx0 + i * 32
    rect(x, y, 28, 20, i)
    outline(x, y, 28, 20, 3)
    glyph(ord('0') + i, x + 10, y + 23)
y += 20 + 3 + 8 + 6

# alignment: 1px checker, 2px checker, 4px vertical stripes
for yy in range(y, y + 8):
    for x in range(8, PAGE_W - 8):
        if (x + yy) & 1:
            put_px(x, yy, 3)
y += 11
for yy in range(y, y + 8):
    for x in range(8, PAGE_W - 8):
        if ((x >> 1) + (yy >> 1)) & 1:
            put_px(x, yy, 3)
y += 11
for yy in range(y, y + 8):
    for x in range(8, PAGE_W - 8):
        if (x >> 2) & 1:
            put_px(x, yy, 3)
y += 8 + 8

# QR code (same URL as the credits screen), 4px/module, centered
URL = qr_url()
qr = segno.make(URL, error='m', micro=False)
matrix = [list(row) for row in qr.matrix]
n = len(matrix)
qs = n * 4
ox = (PAGE_W - qs) // 2
for my in range(n):
    for mx in range(n):
        if matrix[my][mx]:
            rect(ox + mx * 4, y + my * 4, 4, 4, 3)
y += qs + 5
short = URL.split("//", 1)[-1]
text(short, y); y += 8 + 10

# every character on the keyboard, one group per layer, 18 glyphs per row
KB_LOWER = "qwertyuiopasdfghjklzxcvbnm,."
KB_UPPER = "QWERTYUIOPASDFGHJKLZXCVBNM,."
KB_SYM   = "1234567890!@#$%^&*()-_=+[]{}\\|;:'\"<>/?~`"
KB_SHAPE = [0x01, 0x02, 0x03, 0x04,              # arrows
            0x05, 0x06, 0x07, 0x08,              # squares, circles
            0x09, 0x0B, 0x0C, 0x0D,              # triangles, diamonds
            0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13,  # box drawing
            0x14, 0x15, 0x16, 0x17, 0x18]
for group in (KB_LOWER, KB_UPPER, KB_SYM, KB_SHAPE):
    codes = [ord(c) for c in group] if isinstance(group, str) else group
    for i in range(0, len(codes), 18):
        text(codes[i:i + 18], y)
        y += 9
    y += 4
y += 2

text("IF YOU CAN READ", y); y += 9
text("THIS, IT WORKS!", y); y += 8 + 6

# pad to a whole number of 16px bands (2 tile rows) and border the strip
PAGE_H = (y + 15) // 16 * 16
assert PAGE_H <= MAX_H
outline(0, 0, PAGE_W, PAGE_H, 3)
TILE_ROWS = PAGE_H // 8
page = page[:TILE_ROWS * 320]

with open(os.path.join(ROOT, "src/gfx/testpage.c"), "w") as f:
    f.write("// Generated by tools/mktestpage.py - do not edit\n#include \"testpage.h\"\n\n")
    f.write("const unsigned char testpage_page[%d] = {\n" % len(page))
    for i in range(0, len(page), 16):
        f.write("    " + ",".join("0x%02X" % b for b in page[i:i+16]) + ",\n")
    f.write("};\n")

with open(os.path.join(ROOT, "src/gfx/testpage.h"), "w") as f:
    f.write("""// Generated by tools/mktestpage.py - do not edit
#ifndef GFX_TESTPAGE_H
#define GFX_TESTPAGE_H
#define TESTPAGE_ROWS %d
extern const unsigned char testpage_page[%d];
#endif
""" % (TILE_ROWS, len(page)))

# preview PNG (grayscale, 2x) using all 4 shades
S = 2
W, H = PAGE_W * S, PAGE_H * S
SHADE_GRAY = [0xFF, 0xAA, 0x55, 0x00]
gray = bytearray(b"\xff" * (W * H))
for py in range(PAGE_H):
    for px in range(PAGE_W):
        ti = (py // 8) * TILES_PER_ROW + px // 8
        off = ti * 16 + (py % 8) * 2
        bit = 0x80 >> (px % 8)
        s = (1 if page[off] & bit else 0) | (2 if page[off + 1] & bit else 0)
        if s:
            for sy in range(S):
                for sx in range(S):
                    gray[(py * S + sy) * W + px * S + sx] = SHADE_GRAY[s]

def chunk(t, d):
    c = t + d
    return struct.pack(">I", len(d)) + c + struct.pack(">I", zlib.crc32(c))
raw = b"".join(b"\x00" + bytes(gray[yy * W:(yy + 1) * W]) for yy in range(H))
png = (b"\x89PNG\r\n\x1a\n"
       + chunk(b"IHDR", struct.pack(">IIBBBBB", W, H, 8, 0, 0, 0, 0))
       + chunk(b"IDAT", zlib.compress(raw)) + chunk(b"IEND", b""))
os.makedirs(os.path.join(ROOT, ".scratchwork"), exist_ok=True)
open(os.path.join(ROOT, ".scratchwork/testpage_preview.png"), "wb").write(png)
print("testpage.c/h (%d tile rows, %dpx) + preview written" % (TILE_ROWS, PAGE_H))
