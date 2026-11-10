"""Utilidades para recortar sprites das folhas (fundo branco -> transparente)."""
from collections import deque
import numpy as np
from PIL import Image

WHITE_T = 228


def bg_mask(rgb):
    """Fundo = pixels quase brancos conectados a borda da imagem."""
    h, w, _ = rgb.shape
    white = rgb.min(axis=2) >= WHITE_T
    bg = np.zeros((h, w), bool)
    q = deque()
    for x in range(w):
        for y in (0, h - 1):
            if white[y, x] and not bg[y, x]:
                bg[y, x] = True
                q.append((y, x))
    for y in range(h):
        for x in (0, w - 1):
            if white[y, x] and not bg[y, x]:
                bg[y, x] = True
                q.append((y, x))
    while q:
        y, x = q.popleft()
        for dy, dx in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            ny, nx = y + dy, x + dx
            if 0 <= ny < h and 0 <= nx < w and white[ny, nx] and not bg[ny, nx]:
                bg[ny, nx] = True
                q.append((ny, nx))
    return bg


def components(fg, min_area=30):
    """Componentes 8-conectados: lista de (x0, y0, x1, y1, area)."""
    h, w = fg.shape
    seen = np.zeros_like(fg)
    out = []
    ys, xs = np.nonzero(fg)
    for y0, x0 in zip(ys, xs):
        if seen[y0, x0]:
            continue
        q = [(y0, x0)]
        seen[y0, x0] = True
        a = 0
        bx0 = bx1 = x0
        by0 = by1 = y0
        while q:
            y, x = q.pop()
            a += 1
            if x < bx0: bx0 = x
            if x > bx1: bx1 = x
            if y < by0: by0 = y
            if y > by1: by1 = y
            for dy in (-1, 0, 1):
                for dx in (-1, 0, 1):
                    ny, nx = y + dy, x + dx
                    if 0 <= ny < h and 0 <= nx < w and fg[ny, nx] and not seen[ny, nx]:
                        seen[ny, nx] = True
                        q.append((ny, nx))
        if a >= min_area:
            out.append((int(bx0), int(by0), int(bx1) + 1, int(by1) + 1, a))
    return out


def to_rgba(rgb, bg):
    """Converte para RGBA: fundo transparente e borda suavizada."""
    h, w, _ = rgb.shape
    alpha = np.where(bg, 0.0, 1.0)
    # borda: pixels de frente vizinhos do fundo ficam com alfa pela "brancura"
    near = np.zeros_like(bg)
    for dy in (-1, 0, 1):
        for dx in (-1, 0, 1):
            near |= np.roll(np.roll(bg, dy, 0), dx, 1)
    edge = near & ~bg
    lum = rgb.min(axis=2).astype(float)
    a_edge = np.clip((255.0 - lum) / 170.0, 0.0, 1.0)
    alpha = np.where(edge, a_edge, alpha)
    out = rgb.astype(float)
    a3 = alpha[..., None]
    # remove a mistura com branco nas bordas
    with np.errstate(divide="ignore", invalid="ignore"):
        un = np.where(a3 > 0.05, (out - 255.0 * (1 - a3)) / np.maximum(a3, 1e-3), 0)
    out = np.where(edge[..., None], np.clip(un, 0, 255), out)
    rgba = np.dstack([out, alpha * 255.0]).astype(np.uint8)
    return Image.fromarray(rgba, "RGBA")


def label_image(fg):
    """Rotula componentes 8-conectados. Retorna (labels, lista de bbox)."""
    h, w = fg.shape
    lab = np.zeros((h, w), np.int32)
    boxes = [None]
    ys, xs = np.nonzero(fg)
    cur = 0
    for y0, x0 in zip(ys, xs):
        if lab[y0, x0]:
            continue
        cur += 1
        lab[y0, x0] = cur
        q = [(y0, x0)]
        bx0 = bx1 = x0
        by0 = by1 = y0
        while q:
            y, x = q.pop()
            if x < bx0: bx0 = x
            if x > bx1: bx1 = x
            if y < by0: by0 = y
            if y > by1: by1 = y
            for dy in (-1, 0, 1):
                for dx in (-1, 0, 1):
                    ny, nx = y + dy, x + dx
                    if 0 <= ny < h and 0 <= nx < w and fg[ny, nx] and not lab[ny, nx]:
                        lab[ny, nx] = cur
                        q.append((ny, nx))
        boxes.append((int(bx0), int(by0), int(bx1) + 1, int(by1) + 1))
    return lab, boxes
