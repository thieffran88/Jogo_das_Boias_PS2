#include "render.h"
#include "atlas.h"

void r_shape(int shape, float x, float y, float w, float h, rgba_t c)
{
    float u = (float)(SHAPE_X0 + (shape % 3) * SHAPE_SIZE);
    float v = (float)((shape / 3) * SHAPE_SIZE);
    r_atlas(x, y, w, h, u, v, u + SHAPE_SIZE, v + SHAPE_SIZE, c);
}

void r_shape_c(int shape, float cx, float cy, float size, rgba_t c)
{
    r_shape(shape, cx - size * 0.5f, cy - size * 0.5f, size, size, c);
}

/* Decodifica o proximo caractere UTF-8 e devolve o indice do glifo. */
static int next_glyph(const unsigned char **ps)
{
    const unsigned char *s = *ps;
    unsigned int cp;
    int i;
    if (s[0] < 0x80) {
        cp = s[0];
        *ps = s + 1;
    } else if ((s[0] & 0xE0) == 0xC0 && s[1]) {
        cp = ((s[0] & 0x1Fu) << 6) | (s[1] & 0x3Fu);
        *ps = s + 2;
    } else {
        *ps = s + 1;
        return '?' - 32;
    }
    if (cp >= 32 && cp < 127) return (int)cp - 32;
    for (i = 0; i < (int)(sizeof(font_extra) / sizeof(font_extra[0])); i++)
        if (font_extra[i] == cp) return 96 + i;
    return '?' - 32;
}

float r_text_w(const char *str, float scale)
{
    const unsigned char *s = (const unsigned char *)str;
    float w = 0;
    while (*s) {
        int gi = next_glyph(&s);
        w += (font_advance[gi] + 1) * scale;
    }
    return w;
}

float r_text(float x, float y, float scale, rgba_t c, const char *str)
{
    const unsigned char *s = (const unsigned char *)str;
    while (*s) {
        int gi = next_glyph(&s);
        if (gi != 0) {
            float u = (float)((gi % FONT_COLS) * FONT_CELL_W);
            float v = (float)((gi / FONT_COLS) * FONT_CELL_H);
            r_atlas(x, y, FONT_CELL_W * scale, FONT_CELL_H * scale,
                    u, v, u + FONT_CELL_W, v + FONT_CELL_H, c);
        }
        x += (font_advance[gi] + 1) * scale;
    }
    return x;
}

void r_text_c(float cx, float y, float scale, rgba_t c, const char *s)
{
    r_text(cx - r_text_w(s, scale) * 0.5f, y, scale, c, s);
}

void r_text_shadow(float x, float y, float scale, rgba_t c, const char *s)
{
    float o = scale < 1.0f ? 1.0f : 2.0f;
    r_text(x + o, y + o, scale, RGBA(0, 0, 0, 150), s);
    r_text(x, y, scale, c, s);
}

void r_text_c_shadow(float cx, float y, float scale, rgba_t c, const char *s)
{
    r_text_shadow(cx - r_text_w(s, scale) * 0.5f, y, scale, c, s);
}

/* Painel com cantos arredondados: 4 cantos do roundrect + retangulos. */
void r_panel(float x, float y, float w, float h, rgba_t c)
{
    /* No atlas o roundrect tem margem de 2px e raio ~13px. Usamos
     * 30px de cada canto (sem a margem). */
    float u = (float)(SHAPE_X0 + (SHAPE_ROUNDRECT % 3) * SHAPE_SIZE);
    float v = (float)((SHAPE_ROUNDRECT / 3) * SHAPE_SIZE);
    float a0 = 2.0f, a1 = 32.0f, b0 = 32.0f, b1 = SHAPE_SIZE - 2.0f;
    float cs = 26.0f;               /* tamanho do canto na tela */
    if (cs * 2 > w) cs = w / 2;
    if (cs * 2 > h) cs = h / 2;
    r_atlas(x, y, cs, cs, u + a0, v + a0, u + a1, v + a1, c);
    r_atlas(x + w - cs, y, cs, cs, u + b0, v + a0, u + b1, v + a1, c);
    r_atlas(x, y + h - cs, cs, cs, u + a0, v + b0, u + a1, v + b1, c);
    r_atlas(x + w - cs, y + h - cs, cs, cs, u + b0, v + b0, u + b1, v + b1, c);
    r_rect(x + cs, y, w - 2 * cs, h, c);
    r_rect(x, y + cs, cs, h - 2 * cs, c);
    r_rect(x + w - cs, y + cs, cs, h - 2 * cs, c);
}

/* ------------------------------------------------------------------ */
/* Sprites                                                             */
/* ------------------------------------------------------------------ */
#include "assets.h"

float r_spr_w(int id) { return asset_sprites[id].w; }
float r_spr_h(int id) { return asset_sprites[id].h; }

void r_spr(int id, float x, float y, float w, float h, rgba_t c)
{
    const AssetSprite *s = &asset_sprites[id];
    r_image(s->page, x, y, w, h, s->x, s->y, s->x + s->w, s->y + s->h, c);
}

void r_spr_inset(int id, float x, float y, float w, float h, rgba_t c)
{
    const AssetSprite *s = &asset_sprites[id];
    r_image(s->page, x, y, w, h, s->x + 1, s->y + 1, s->x + s->w - 1, s->y + s->h - 1, c);
}

void r_spr_c(int id, float cx, float cy, float scale, rgba_t c)
{
    float w = asset_sprites[id].w * scale, h = asset_sprites[id].h * scale;
    r_spr(id, cx - w * 0.5f, cy - h * 0.5f, w, h, c);
}

void r_spr_b(int id, float cx, float by, float scale, rgba_t c)
{
    float w = asset_sprites[id].w * scale, h = asset_sprites[id].h * scale;
    r_spr(id, cx - w * 0.5f, by - h, w, h, c);
}

void r_text_outline(float x, float y, float scale, rgba_t c, rgba_t o, const char *s)
{
    static const signed char d[8][2] = {
        {-2, 0}, {2, 0}, {0, -2}, {0, 2}, {-1, -1}, {1, -1}, {-1, 1}, {1, 1}
    };
    int i;
    for (i = 0; i < 8; i++)
        r_text(x + d[i][0], y + d[i][1], scale, o, s);
    r_text(x, y, scale, c, s);
}

void r_text_c_outline(float cx, float y, float scale, rgba_t c, rgba_t o, const char *s)
{
    r_text_outline(cx - r_text_w(s, scale) * 0.5f, y, scale, c, o, s);
}

/* Fonte amarela: so maiusculas, numeros e alguma pontuacao. */
static int ychar(const unsigned char **ps)
{
    const unsigned char *s = *ps;
    unsigned int cp;
    if (s[0] < 0x80) {
        cp = s[0];
        *ps = s + 1;
    } else if ((s[0] & 0xE0) == 0xC0 && s[1]) {
        cp = ((s[0] & 0x1Fu) << 6) | (s[1] & 0x3Fu);
        *ps = s + 2;
    } else {
        *ps = s + 1;
        return '?';
    }
    if (cp >= 'a' && cp <= 'z') return (int)(cp - 32);
    if ((cp >= 0xC0 && cp <= 0xC5) || (cp >= 0xE0 && cp <= 0xE5)) return 'A';
    if (cp == 0xC7 || cp == 0xE7) return 'C';
    if ((cp >= 0xC8 && cp <= 0xCB) || (cp >= 0xE8 && cp <= 0xEB)) return 'E';
    if ((cp >= 0xCC && cp <= 0xCF) || (cp >= 0xEC && cp <= 0xEF)) return 'I';
    if ((cp >= 0xD2 && cp <= 0xD6) || (cp >= 0xF2 && cp <= 0xF6)) return 'O';
    if ((cp >= 0xD9 && cp <= 0xDC) || (cp >= 0xF9 && cp <= 0xFC)) return 'U';
    if (cp < 128) return (int)cp;
    return '?';
}

float r_ytext_w(const char *str, float h)
{
    const unsigned char *s = (const unsigned char *)str;
    float k = h / YFONT_H, w = 0;
    while (*s) {
        int ch = ychar(&s);
        if (ch == ' ' || yfont_map[ch] < 0) w += h * 0.35f;
        else w += asset_sprites[yfont_map[ch]].w * k - h * 0.06f;
    }
    return w;
}

float r_ytext(float x, float y, float h, rgba_t c, const char *str)
{
    const unsigned char *s = (const unsigned char *)str;
    float k = h / YFONT_H;
    while (*s) {
        int ch = ychar(&s);
        if (ch == ' ' || yfont_map[ch] < 0) {
            x += h * 0.35f;
            continue;
        }
        r_spr(yfont_map[ch], x, y, asset_sprites[yfont_map[ch]].w * k, h, c);
        x += asset_sprites[yfont_map[ch]].w * k - h * 0.06f;
    }
    return x;
}

void r_ytext_c(float cx, float y, float h, rgba_t c, const char *s)
{
    r_ytext(cx - r_ytext_w(s, h) * 0.5f, y, h, c, s);
}
