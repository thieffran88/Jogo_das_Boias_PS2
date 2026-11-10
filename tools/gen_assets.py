#!/usr/bin/env python3
"""Gera assets/assets.bin + src/assets.h a partir das folhas em assets/src/.

Cada sprite e recortado de uma folha (fundo branco -> transparente),
redimensionado e empacotado em paginas de 512x512 com paleta de 256 cores
(PSMT8 no PS2 = 256 KB por pagina de VRAM).

Uso: python3 tools/gen_assets.py   (demora ~30 s)
"""
import os
import sys

import numpy as np
from PIL import Image

sys.path.insert(0, os.path.dirname(__file__))
from sheetlib import bg_mask, label_image, to_rgba  # noqa: E402

ROOT = os.path.join(os.path.dirname(__file__), "..")
SRC = os.path.join(ROOT, "assets", "src")
PAGE = 512
PAD = 2

# ---------------------------------------------------------------------------
# Definicao dos sprites: nome, folha, retangulo (x0, y0, x1, y1), tamanho
# tamanho: ("h", px) altura alvo, ("w", px) largura alvo, ("s", escala)
# opcoes: xsplit=(n, i) divide o retangulo em n colunas; rot=90
# ---------------------------------------------------------------------------
S = []


def spr(name, sheet, rect, size, **opt):
    S.append(dict(name=name, sheet=sheet, rect=rect, size=size, **opt))


# Logo e personagens
spr("LOGO", 2, (15, 9, 361, 171), ("w", 330))
spr("CHAR_P1", 2, (374, 12, 566, 181), ("h", 118))
spr("CHAR_P2", 2, (577, 18, 718, 180), ("h", 118))
spr("CHAR_P3", 2, (718, 26, 882, 179), ("h", 118))
spr("CHAR_P4", 2, (882, 21, 1043, 176), ("h", 118))
for i, (a, b) in enumerate([(1056, 1150), (1150, 1244), (1244, 1338), (1338, 1432)]):
    spr("BADGE_P%d" % (i + 1), 2, (a, 33, b, 167), ("h", 64), keepall=True)

# Reacoes de cada jogador: normal, feliz, triste
spr("R_P1_IDLE", 2, (26, 200, 108, 289), ("h", 70))
spr("R_P1_HAPPY", 2, (212, 428, 326, 529), ("h", 80))
spr("R_P1_SAD", 2, (421, 436, 529, 526), ("h", 70))
spr("R_P2_IDLE", 2, (550, 204, 648, 295), ("h", 70))
spr("R_P2_HAPPY", 2, (759, 202, 887, 295), ("h", 74))
spr("R_P2_SAD", 2, (663, 205, 759, 295), ("h", 70))
spr("R_P3_IDLE", 2, (545, 319, 653, 407), ("h", 70))
spr("R_P3_HAPPY", 2, (898, 311, 1017, 407), ("h", 74))
spr("R_P3_SAD", 2, (759, 312, 896, 407), ("h", 74))
spr("R_P4_IDLE", 2, (545, 431, 667, 535), ("h", 74))
spr("R_P4_HAPPY", 2, (795, 433, 899, 535), ("h", 72))
spr("R_P4_SAD", 2, (675, 434, 789, 534), ("h", 70))

# Emblemas de fim de jogo
spr("VICTORY", 1, (33, 8, 278, 222), ("h", 160))
spr("DEFEAT", 1, (306, 16, 522, 232), ("h", 160))
# versoes sem o texto em portugues (para ingles/espanhol)
spr("VICTORY_BASE", 1, (33, 86, 278, 222), ("h", 102), keepall=True)
spr("DEFEAT_BASE", 1, (306, 78, 522, 232), ("h", 115), keepall=True)

# Dados 1..6
for i, r in enumerate([(1046, 331, 1104, 389), (1112, 330, 1169, 387),
                       (1174, 330, 1234, 392), (1241, 328, 1302, 389),
                       (1306, 331, 1364, 393), (1374, 331, 1437, 396)]):
    spr("DIE_%d" % (i + 1), 2, r, ("h", 54))

# Boias (vermelha, amarela, azul, verde) da folha 1
RINGS = (973, 266, 1436, 326)
for name, idx in (("RING_RED", 0), ("RING_YELLOW", 1), ("RING_BLUE", 2), ("RING_GREEN", 3)):
    spr(name, 1, RINGS, ("w", 52), xsplit=(6, idx), keepall=True)

spr("ROPE", 1, (868, 480, 966, 512), ("w", 14), rot=90)
spr("LIFEGUARD", 2, (926, 732, 1023, 900), ("h", 118))

# Botoes
spr("BTN_JOGAR", 2, (1094, 549, 1264, 593), ("h", 40))
spr("BTN_CONTINUAR", 2, (1273, 549, 1438, 592), ("h", 40))
spr("BTN_OPCOES", 2, (1094, 595, 1265, 638), ("h", 40))
spr("BTN_VOLTAR", 2, (1273, 595, 1438, 638), ("h", 40))
spr("BTN_CREDITOS", 2, (1094, 641, 1265, 685), ("h", 40))
spr("BTN_SAIR", 2, (1273, 641, 1438, 685), ("h", 40))
spr("BTN_COMOJOGAR", 1, (19, 835, 230, 879), ("h", 40), keepall=True)
spr("BTN_BLANK_YELLOW", 2, (1094, 549, 1264, 593), ("h", 40), blank=True)
spr("BTN_BLANK_BLUE", 2, (1273, 595, 1438, 638), ("h", 40), blank=True)
spr("BTN_BLANK_RED", 2, (1273, 641, 1438, 685), ("h", 40), blank=True)

# Icones
spr("ICON_X", 2, (855, 630, 905, 681), ("h", 28))
spr("ICON_O", 2, (909, 630, 959, 681), ("h", 28))
spr("ICON_TRI", 2, (964, 630, 1013, 680), ("h", 28))
spr("ICON_SQ", 2, (1018, 629, 1069, 680), ("h", 28))
spr("ARROW_L", 2, (855, 562, 906, 614), ("h", 28))
spr("ARROW_R", 2, (909, 563, 960, 613), ("h", 28))
spr("ARROW_U", 2, (965, 562, 1013, 616), ("h", 28))
spr("ARROW_D", 2, (1020, 563, 1069, 616), ("h", 28))
spr("STAR", 2, (1099, 451, 1148, 501), ("h", 34))
spr("TROPHY", 1, (1242, 725, 1292, 778), ("h", 52))
spr("BURST", 2, (521, 940, 666, 1048), ("s", 1.0))
spr("FX_EXCLAIM", 1, (1068, 812, 1117, 859), ("h", 30))
spr("FX_HEART", 1, (1332, 816, 1380, 859), ("h", 30))

# Cenario
spr("CLOUD_1", 2, (600, 712, 684, 756), ("h", 40))
spr("CLOUD_2", 2, (855, 709, 945, 755), ("h", 40))
spr("BUSH_1", 2, (481, 709, 588, 765), ("h", 46))
spr("BUSH_2", 2, (563, 844, 738, 903), ("h", 46))
spr("PALM", 2, (691, 714, 813, 841), ("h", 118))
spr("UMBRELLA", 2, (796, 757, 923, 902), ("h", 118))
spr("BUNTING", 2, (1188, 711, 1422, 795), ("h", 60))
spr("BUOY_POST", 2, (1155, 787, 1227, 902), ("h", 80))

# Fonte amarela (titulos e numeros)
YFONT = [
    (1, (55, 935, 810, 996), "ABCDEFGHIJKLM"),
    (1, (50, 1000, 825, 1066), "NOPQRSTUVWXYZ"),
    (1, (870, 935, 1380, 996), "0123456789"),
    (1, (870, 1003, 1380, 1062), "!?:;.,+-x/="),
]
YFONT_H = 36

# Animacao da agua (10 quadros da folha 3)
WATER = [(22, 623, 297, 827), (305, 623, 579, 827), (587, 623, 860, 827),
         (869, 623, 1143, 828), (1151, 623, 1430, 828),
         (22, 843, 298, 1051), (305, 842, 578, 1051), (587, 842, 860, 1051),
         (869, 843, 1143, 1052), (1151, 843, 1429, 1052)]
WATER_W, WATER_H = 256, 102


# ---------------------------------------------------------------------------

def load_sheets():
    sheets = {}
    for n in (1, 2, 3):
        im = Image.open(os.path.join(SRC, "folha%d.png" % n)).convert("RGB")
        rgb = np.array(im)
        bg = bg_mask(rgb)
        lab, boxes = label_image(~bg)
        sheets[n] = dict(rgb=rgb, rgba=np.array(to_rgba(rgb, bg)), lab=lab, boxes=boxes)
        print("folha%d: %d componentes" % (n, len(boxes) - 1), file=sys.stderr)
    return sheets


def xsplit_bounds(alpha, n):
    """Divide colunas em n partes nos vales do perfil de alfa."""
    prof = alpha.sum(axis=0).astype(float)
    w = len(prof)
    cuts = [0]
    for k in range(1, n):
        c = int(round(w * k / n))
        r = max(2, w // (n * 4))
        lo, hi = max(1, c - r), min(w - 1, c + r)
        cuts.append(lo + int(np.argmin(prof[lo:hi])))
    cuts.append(w)
    return cuts


def cut_sprite(sheets, d):
    sh = sheets[d["sheet"]]
    x0, y0, x1, y1 = d["rect"]
    rgba = sh["rgba"][y0:y1, x0:x1].copy()
    if not d.get("keepall"):
        # mantem so componentes cujo centro esta dentro do retangulo
        lab = sh["lab"][y0:y1, x0:x1]
        keep = np.zeros(lab.shape, bool)
        for l in np.unique(lab):
            if l == 0:
                continue
            bx0, by0, bx1, by1 = sh["boxes"][l]
            cx, cy = (bx0 + bx1) / 2, (by0 + by1) / 2
            if x0 <= cx < x1 and y0 <= cy < y1:
                keep |= lab == l
        rgba[..., 3] = np.where(keep, rgba[..., 3], 0)
    if "xsplit" in d:
        n, i = d["xsplit"]
        cuts = xsplit_bounds(rgba[..., 3] > 128, n)
        rgba = rgba[:, cuts[i]:cuts[i + 1]]
    im = Image.fromarray(rgba, "RGBA")
    bb = im.getbbox()
    if bb:
        im = im.crop(bb)
    if d.get("blank"):
        # apaga o texto do botao repetindo, linha a linha, a cor de uma coluna lateral
        a = np.array(im)
        w = a.shape[1]
        x0, x1 = int(w * 0.2), int(w * 0.8)
        a[:, x0:x1] = a[:, x0:x0 + 1]
        im = Image.fromarray(a, "RGBA")
    if d.get("rot"):
        im = im.rotate(d["rot"], expand=True)
    kind, v = d["size"]
    w, h = im.size
    s = v / h if kind == "h" else v / w if kind == "w" else v
    return resize(im, max(1, round(w * s)), max(1, round(h * s)))


def resize(im, w, h):
    """Redimensiona com alfa pre-multiplicado (sem halos)."""
    a = np.array(im).astype(float)
    a[..., :3] *= a[..., 3:4] / 255.0
    pm = Image.fromarray(a.astype(np.uint8), "RGBA").resize((w, h), Image.LANCZOS)
    b = np.array(pm).astype(float)
    al = b[..., 3:4]
    b[..., :3] = np.where(al > 0, np.clip(b[..., :3] * 255.0 / np.maximum(al, 1), 0, 255), 0)
    return Image.fromarray(b.astype(np.uint8), "RGBA")


def yfont_sprites(sheets):
    out = []
    for sheet, (x0, y0, x1, y1), chars in YFONT:
        sh = sheets[sheet]
        lab = sh["lab"][y0:y1, x0:x1]
        comps = []
        for l in np.unique(lab):
            if l == 0:
                continue
            bx0, by0, bx1, by1 = sh["boxes"][l]
            if bx0 >= x0 and bx1 <= x1 and by0 >= y0 and by1 <= y1 and (bx1 - bx0) * (by1 - by0) > 120:
                comps.append((bx0, by0, bx1, by1, l))
        comps.sort()
        if len(comps) != len(chars):
            raise SystemExit("fonte: %d componentes para %r" % (len(comps), chars))
        for (bx0, by0, bx1, by1, l), ch in zip(comps, chars):
            rgba = sh["rgba"][y0:y1, bx0:bx1].copy()
            rgba[..., 3] = np.where(sh["lab"][y0:y1, bx0:bx1] == l, rgba[..., 3], 0)
            im = Image.fromarray(rgba, "RGBA")
            # mantem a linha de base: recorta so na horizontal, altura da linha
            im = im.crop((0, 0, im.size[0], im.size[1]))
            row_h = y1 - y0
            s = YFONT_H / row_h
            img = resize(im, max(1, round(im.size[0] * s)), YFONT_H)
            out.append(("YF_%d" % ord(ch), img, ch))
    return out


def pack(items):
    """Empacotamento em prateleiras. items: [(nome, img)] -> paginas."""
    items = sorted(items, key=lambda t: (-t[1].size[1], -t[1].size[0]))
    pages = []
    for name, img in items:
        w, h = img.size
        for pg in pages:
            if pg["x"] + w + PAD <= PAGE and pg["y"] + h + PAD <= PAGE:
                break
            ny = pg["y"] + pg["sh"] + PAD
            if ny + h + PAD <= PAGE:
                pg["y"], pg["x"], pg["sh"] = ny, 0, h
                break
        else:
            pg = dict(x=0, y=0, sh=h, items=[])
            pages.append(pg)
        pg["items"].append((name, img, pg["x"] + PAD, pg["y"] + PAD))
        pg["x"] += w + PAD
        pg["sh"] = max(pg["sh"], h)
    return [p["items"] for p in pages]


def page_height(items):
    used = max(y + img.size[1] for _, img, _, y in items)
    h = 64
    while h < used:
        h *= 2
    return h


def quantize_page(items):
    canvas = Image.new("RGBA", (PAGE, page_height(items)), (0, 0, 0, 0))
    for _, img, x, y in items:
        canvas.alpha_composite(img, (x, y))
    q = canvas.quantize(colors=256, method=Image.Quantize.FASTOCTREE, dither=Image.Dither.NONE)
    pal = q.getpalette(rawmode="RGBA")[:1024]
    pal += [0] * (1024 - len(pal))
    idx = np.array(q, dtype=np.uint8)
    # garante transparencia exata onde o alfa original e 0
    alpha = np.array(canvas)[..., 3]
    pal_a = np.array(pal, dtype=np.uint8).reshape(256, 4)
    zero = [i for i in range(256) if pal_a[i, 3] == 0]
    if zero:
        idx[alpha == 0] = zero[0]
    return canvas, idx, bytes(pal)


def main():
    sheets = load_sheets()
    items = []
    for d in S:
        items.append((d["name"], cut_sprite(sheets, d)))
    yf = yfont_sprites(sheets)
    for name, img, _ in yf:
        items.append((name, img))

    pages = pack(items)

    # pagina da agua
    sh = sheets[3]
    water = Image.new("RGBA", (PAGE, PAGE), (0, 0, 0, 0))
    witems = []
    for i, (x0, y0, x1, y1) in enumerate(WATER):
        fr = Image.fromarray(sh["rgb"][y0 + 58:y1 - 14, x0 + 14:x1 - 14], "RGB").convert("RGBA")
        fr = fr.resize((WATER_W, WATER_H), Image.LANCZOS)
        x, y = (i % 2) * WATER_W, (i // 2) * WATER_H
        witems.append(("WATER_%d" % i, fr, x, y))
    pages.append(witems)

    blob = bytearray()
    page_desc = []
    sprites = {}
    previews = []
    for pi, items_p in enumerate(pages):
        canvas, idx, pal = quantize_page(items_p)
        previews.append(canvas)
        while len(blob) % 128:
            blob.append(0)
        io = len(blob)
        blob += idx.tobytes()
        po = len(blob)
        blob += pal
        page_desc.append((io, po, canvas.size[1]))
        for name, img, x, y in items_p:
            sprites[name] = (pi, x, y, img.size[0], img.size[1])

    os.makedirs(os.path.join(ROOT, "assets"), exist_ok=True)
    with open(os.path.join(ROOT, "assets", "assets.bin"), "wb") as f:
        f.write(blob)
    prev_dir = os.environ.get("ASSET_PREVIEW")
    if prev_dir:
        for i, c in enumerate(previews):
            c.save(os.path.join(prev_dir, "page%d.png" % i))

    order = [d["name"] for d in S] + ["WATER_%d" % i for i in range(len(WATER))] + [n for n, _, _ in yf]
    out = ["/* Gerado por tools/gen_assets.py - nao edite a mao. */",
           "#ifndef ASSETS_H", "#define ASSETS_H", "",
           "#define ASSET_PAGE_SIZE %d" % PAGE,
           "#define ASSET_NUM_PAGES %d" % len(pages),
           "#define ASSET_BIN_SIZE %d" % len(blob),
           "#define WATER_FRAMES %d" % len(WATER),
           "#define YFONT_H %d" % YFONT_H, "",
           "typedef struct { unsigned int idx_off, pal_off, h; } AssetPage;",
           "typedef struct { unsigned char page; unsigned short x, y, w, h; } AssetSprite;", "",
           "enum {"]
    for n in order:
        out.append("    SPR_%s," % n)
    out.append("    SPR_COUNT\n};\n")
    out.append("static const AssetPage asset_pages[ASSET_NUM_PAGES] = {")
    for io, po, ph in page_desc:
        out.append("    {%d, %d, %d}," % (io, po, ph))
    out.append("};\n")
    out.append("static const AssetSprite asset_sprites[SPR_COUNT] = {")
    for n in order:
        p, x, y, w, h = sprites[n]
        out.append("    {%d, %d, %d, %d, %d}, /* %s */" % (p, x, y, w, h, n))
    out.append("};\n")
    fmap = ["-1"] * 128
    for n, _, ch in yf:
        fmap[ord(ch)] = "SPR_" + n
    out.append("static const short yfont_map[128] = {")
    for i in range(0, 128, 8):
        out.append("    " + ", ".join(fmap[i:i + 8]) + ",")
    out.append("};\n")
    out.append("extern const unsigned char assets_bin[];\n")
    out.append("#endif")
    with open(os.path.join(ROOT, "src", "assets.h"), "w") as f:
        f.write("\n".join(out) + "\n")
    print("%d paginas, %d bytes" % (len(pages), len(blob)), file=sys.stderr)


if __name__ == "__main__":
    main()
