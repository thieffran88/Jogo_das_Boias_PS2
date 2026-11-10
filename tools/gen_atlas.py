#!/usr/bin/env python3
"""Gera src/atlas.h: um atlas 512x256 (somente alfa, 8 bits) com a fonte
e as formas (disco, boia, listras, dado...) usadas pelo jogo.

Uso: python3 tools/gen_atlas.py [caminho/da/fonte.ttf] > src/atlas.h
"""
import math
import sys
from PIL import Image, ImageDraw, ImageFont

W, H = 512, 256
CELL_W, CELL_H = 20, 28
COLS = 16
FONT_SIZE = 20
SHAPE = 64
SHAPE_X0 = 320

font_path = sys.argv[1] if len(sys.argv) > 1 else \
    "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
font = ImageFont.truetype(font_path, FONT_SIZE)

img = Image.new("L", (W, H), 0)
d = ImageDraw.Draw(img)

# ASCII 32..126 + 1 reservado + acentos do portugues
EXTRA = "áàâãéêíóôõúçÁÉÇÃÂÊÍÓÔÕÚÀ"
CHARS = [chr(c) for c in range(32, 127)] + ["?"] + list(EXTRA)
advances = []
for i, ch in enumerate(CHARS):
    cx = (i % COLS) * CELL_W
    cy = (i // COLS) * CELL_H
    adv = font.getlength(ch)
    adv = min(CELL_W - 1, int(round(adv)))
    advances.append(adv)
    d.text((cx + 1, cy + 3), ch, fill=255, font=font)

SS = 4  # supersampling para formas suaves


def shape(fn):
    big = Image.new("L", (SHAPE * SS, SHAPE * SS), 0)
    fn(ImageDraw.Draw(big), SHAPE * SS)
    return big.resize((SHAPE, SHAPE), Image.LANCZOS)


def disc(dr, s):
    m = 2 * SS
    dr.ellipse((m, m, s - m, s - m), fill=255)


def ring(dr, s):
    m = 2 * SS
    dr.ellipse((m, m, s - m, s - m), fill=255)
    r = s * 0.20
    c = s / 2
    dr.ellipse((c - r, c - r, c + r, c + r), fill=0)


def stripes(dr, s):
    # quatro faixas brancas na boia (desenhadas por cima da cor do jogador)
    m = 2 * SS
    c = s / 2
    for k in range(4):
        a0 = k * 90 + 20
        dr.pieslice((m, m, s - m, s - m), a0, a0 + 45, fill=255)
    r = s * 0.20
    dr.ellipse((c - r, c - r, c + r, c + r), fill=0)


def roundrect(dr, s):
    m = 2 * SS
    dr.rounded_rectangle((m, m, s - m, s - m), radius=s * 0.22, fill=255)


def glow(dr, s):
    c = s / 2
    for r in range(int(c), 0, -SS):
        v = int(255 * (1 - r / c) ** 1.5)
        dr.ellipse((c - r, c - r, c + r, c + r), fill=v)


def ring_outline(dr, s):
    m = 2 * SS
    dr.ellipse((m, m, s - m, s - m), outline=255, width=4 * SS)


def tower(dr, s):
    # torre de salva-vidas estilizada (original)
    w = s
    dr.polygon([(w * .30, w * .95), (w * .38, w * .45), (w * .44, w * .45),
                (w * .38, w * .95)], fill=255)
    dr.polygon([(w * .70, w * .95), (w * .62, w * .45), (w * .56, w * .45),
                (w * .62, w * .95)], fill=255)
    dr.rectangle((w * .36, w * .70, w * .64, w * .74), fill=255)
    dr.rectangle((w * .30, w * .38, w * .70, w * .46), fill=255)
    dr.rectangle((w * .33, w * .20, w * .37, w * .38), fill=255)
    dr.rectangle((w * .63, w * .20, w * .67, w * .38), fill=255)
    dr.polygon([(w * .22, w * .22), (w * .50, w * .05), (w * .78, w * .22)],
               fill=255)


def wave(dr, s):
    pts = []
    for x in range(0, s + 1, SS):
        y = s / 2 + math.sin(x / s * math.pi * 4) * s * 0.12
        pts.append((x, y))
    dr.line(pts, fill=255, width=5 * SS)


def star(dr, s):
    c = s / 2
    pts = []
    for k in range(10):
        r = c * (0.95 if k % 2 == 0 else 0.42)
        a = -math.pi / 2 + k * math.pi / 5
        pts.append((c + r * math.cos(a), c + r * math.sin(a)))
    dr.polygon(pts, fill=255)


shapes = [disc, ring, stripes, roundrect, glow, ring_outline, tower, wave, star]
names = ["DISC", "RING", "STRIPES", "ROUNDRECT", "GLOW", "RING_OUTLINE",
         "TOWER", "WAVE", "STAR"]
for i, fn in enumerate(shapes):
    x = SHAPE_X0 + (i % 3) * SHAPE
    y = (i // 3) * SHAPE
    img.paste(shape(fn), (x, y))

data = img.tobytes()
out = []
out.append("/* Gerado por tools/gen_atlas.py - nao edite a mao. */")
out.append("#ifndef ATLAS_H\n#define ATLAS_H\n")
out.append("#define ATLAS_W %d\n#define ATLAS_H_PX %d" % (W, H))
out.append("#define FONT_CELL_W %d\n#define FONT_CELL_H %d\n#define FONT_COLS %d"
           % (CELL_W, CELL_H, COLS))
out.append("#define SHAPE_SIZE %d\n#define SHAPE_X0 %d\n" % (SHAPE, SHAPE_X0))
out.append("enum {")
for i, n in enumerate(names):
    out.append("    SHAPE_%s = %d," % (n, i))
out.append("    SHAPE_COUNT = %d\n};\n" % len(names))
out.append("static const unsigned char font_advance[%d] = {" % len(CHARS))
out.append("    " + ",".join(str(a) for a in advances))
out.append("};\n")
out.append("#define FONT_GLYPHS %d" % len(CHARS))
out.append("/* code points Unicode dos glifos extras (indice 96+) */")
out.append("static const unsigned short font_extra[%d] = {" % len(EXTRA))
out.append("    " + ",".join(str(ord(c)) for c in EXTRA))
out.append("};\n")
out.append("static const unsigned char atlas_alpha[%d] = {" % len(data))
row = []
for i, b in enumerate(data):
    row.append(str(b))
    if len(row) == 32:
        out.append(",".join(row) + ",")
        row = []
if row:
    out.append(",".join(row))
out.append("};\n\n#endif")
print("\n".join(out))
