#!/usr/bin/env python3
"""Generate GBScribe's audio assets: an original ProTracker MOD theme and two
WAV sound effects. Everything is synthesized here — no third-party audio.

Outputs: assets/theme.mod, assets/click.wav, assets/ding.wav
"""
import os, struct, math, wave

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ASSETS = os.path.join(ROOT, "assets")
os.makedirs(ASSETS, exist_ok=True)

# ---- samples (signed 8-bit) --------------------------------------------------
def square(n=32, duty=0.5, amp=90):
    return bytes((amp if i < n * duty else 256 - amp) & 0xFF for i in range(n))

def triangle(n=64, amp=100):
    out = []
    for i in range(n):
        ph = i / n
        v = 4 * amp * (ph - 0.5 if ph > 0.5 else 0.5 - ph) - amp
        out.append(int(v) & 0xFF)
    return bytes(out)

def noise(n=600, amp=70, seed=0xACE1):
    out, lfsr = [], seed
    for _ in range(n):
        bit = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5)) & 1
        lfsr = (lfsr >> 1) | (bit << 15)
        out.append((amp if lfsr & 1 else 256 - amp) & 0xFF)
    return bytes(out)

# name, data, volume, loop (start/2, len/2) in words
samples = [
    ("lead square", square(32, 0.5), 40, (0, 16)),
    ("soft square", square(32, 0.25), 36, (0, 16)),
    ("bass tri",    triangle(64), 52, (0, 32)),
    ("hat",         noise(400), 26, (0, 0)),
]

# ---- note periods (ProTracker, finetune 0) ------------------------------------
P = {
    'C1':856,'D1':762,'E1':678,'F1':640,'G1':570,'A1':508,'B1':453,
    'C2':428,'D2':381,'E2':339,'F2':320,'G2':285,'A2':254,'B2':226,
    'C3':214,'D3':190,'E3':170,'F3':160,'G3':143,'A3':127,'B3':113,
}

def cell(note=None, smp=0, eff=0, prm=0):
    period = P[note] if note else 0
    b0 = (smp & 0xF0) | (period >> 8)
    b1 = period & 0xFF
    b2 = ((smp & 0x0F) << 4) | (eff & 0x0F)
    return bytes((b0, b1, b2, prm))

EMPTY = cell()

def pattern(rows):
    """rows: dict row -> [cell,cell,cell,cell] (channels)"""
    out = b""
    for r in range(64):
        chans = rows.get(r, [EMPTY] * 4)
        out += b"".join(chans)
    return out

# channels: 0 = lead, 1 = bass, 2 = hat, 3 = echo/harmony
L, S, B, H = 1, 2, 3, 4          # sample numbers (1-based in MOD)

def bar(rows, row0, melody, bass, harmony=None):
    """8-row bar helper: melody/harmony = (note or None) per 2 rows"""
    for i, n in enumerate(melody):
        if n:
            rows.setdefault(row0 + i * 2, [EMPTY]*4)[0] = cell(n, L)
    if bass:
        rows.setdefault(row0, [EMPTY]*4)[1] = cell(bass, B)
    for i in (0, 4):
        rows.setdefault(row0 + i + 2, [EMPTY]*4)[2] = cell('C3', H)
    if harmony:
        for i, n in enumerate(harmony):
            if n:
                rows.setdefault(row0 + i * 2, [EMPTY]*4)[3] = cell(n, S)

# pattern 0: gentle C - Am - F - G loop
r0 = {}
bar(r0,  0, ['C2','E2','G2','E2'], 'C1', [None,'C2',None,None])
bar(r0,  8, ['A2','G2','E2','C2'], 'A1', [None,'E2',None,None])
bar(r0, 16, ['F2','A2','C3','A2'], 'F1', [None,'F2',None,None])
bar(r0, 24, ['G2','B2','D3','B2'], 'G1', [None,'G2',None,None])
bar(r0, 32, ['C2','E2','G2','C3'], 'C1', [None,'C2',None,None])
bar(r0, 40, ['A2','C3','E3','C3'], 'A1', [None,'E2',None,None])
bar(r0, 48, ['F2','A2','G2','F2'], 'F1', [None,'F2',None,None])
bar(r0, 56, ['G2','D2','G2','B2'], 'G1', [None,'G2',None,None])

# pattern 1: answer phrase, ends back on C
r1 = {}
bar(r1,  0, ['E3','D3','C3','G2'], 'C1', [None,'G2',None,None])
bar(r1,  8, ['A2','B2','C3','E3'], 'A1', [None,'A2',None,None])
bar(r1, 16, ['D3','C3','B2','A2'], 'F1', [None,'F2',None,None])
bar(r1, 24, ['G2','A2','B2','D3'], 'G1', [None,'G2',None,None])
bar(r1, 32, ['C3','G2','E2','G2'], 'C1', [None,'E2',None,None])
bar(r1, 40, ['A2','C3','E3','G3'], 'A1', [None,'A2',None,None])
bar(r1, 48, ['F3','E3','D3','B2'], 'G1', [None,'G2',None,None])
bar(r1, 56, ['C3',None,'G2',None], 'C1', [None,'E2',None,None])

patterns = [pattern(r0), pattern(r1)]
order = [0, 0, 1, 0]

# ---- write MOD -----------------------------------------------------------------
mod = bytearray()
mod += b"GBScribe theme".ljust(20, b"\0")
for name, data, vol, (ls, ll) in samples:
    mod += name.encode().ljust(22, b"\0")
    mod += struct.pack(">H", len(data) // 2)
    mod += bytes((0, vol))
    mod += struct.pack(">HH", ls, ll if ll > 1 else 1)
for _ in range(31 - len(samples)):
    mod += b"\0" * 22 + struct.pack(">H", 0) + bytes((0, 0)) + struct.pack(">HH", 0, 1)
mod += bytes((len(order), 127))
mod += bytes(order).ljust(128, b"\0")
mod += b"M.K."
for p in patterns:
    mod += p
for _, data, _, _ in samples:
    mod += data
open(os.path.join(ASSETS, "theme.mod"), "wb").write(mod)

# ---- SFX WAVs (8-bit mono 16 kHz) ----------------------------------------------
def write_wav(path, samples8, rate=16000):
    w = wave.open(path, "wb")
    w.setnchannels(1)
    w.setsampwidth(1)
    w.setframerate(rate)
    w.writeframes(bytes(samples8))
    w.close()

# click: 6ms filtered tick
click = []
for i in range(96):
    env = max(0.0, 1.0 - i / 96)
    v = (60 if (i // 8) % 2 == 0 else -60) * env * env
    click.append(int(v) + 128)
write_wav(os.path.join(ASSETS, "click.wav"), click)

# ding: 880 Hz sine, 140ms exponential decay
ding = []
for i in range(2240):
    t = i / 16000
    env = math.exp(-t * 22)
    v = 90 * env * math.sin(2 * math.pi * 880 * t)
    ding.append(int(v) + 128)
write_wav(os.path.join(ASSETS, "ding.wav"), ding)

print("theme.mod (%d bytes), click.wav, ding.wav written" % len(mod))
