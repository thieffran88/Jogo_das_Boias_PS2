/*
 * Simulador de PC: roda a mesma UI do jogo com um rasterizador em
 * software e grava capturas de tela (PPM). Serve para conferir o visual
 * sem precisar de um PS2/emulador.
 *
 *   ./build/sim saida_dir "script"
 *
 * O script e uma lista separada por espacos de comandos:
 *   X O T Q S L R U D   botoes do controle 1 (cruz, bola, triangulo,
 *                       quadrado, start, esquerda, direita, cima, baixo)
 *   2X 2O ...           o mesmo botao no controle 2
 *   wN                  espera N quadros
 *   shot:nome           grava nome.ppm
 */
#include "../src/render.h"
#include "../src/atlas.h"
#include "../src/ui.h"
#include "../src/assets.h"
#include "../src/audio.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static float fb[SCREEN_H][SCREEN_W][3];
static unsigned int ticks = 12345;

unsigned int platform_ticks(void) { return ticks; }

static void blend(int x, int y, float r, float g, float b, float a)
{
    float *p;
    if (x < 0 || y < 0 || x >= SCREEN_W || y >= SCREEN_H || a <= 0) return;
    p = fb[y][x];
    p[0] += (r - p[0]) * a;
    p[1] += (g - p[1]) * a;
    p[2] += (b - p[2]) * a;
}

#define CR(c) (((c) >> 24) & 255) / 255.0f
#define CG(c) (((c) >> 16) & 255) / 255.0f
#define CB(c) (((c) >> 8) & 255) / 255.0f
#define CA(c) ((c) & 255) / 255.0f

void r_begin(rgba_t c)
{
    int x, y;
    for (y = 0; y < SCREEN_H; y++)
        for (x = 0; x < SCREEN_W; x++) {
            fb[y][x][0] = CR(c);
            fb[y][x][1] = CG(c);
            fb[y][x][2] = CB(c);
        }
}

void r_end(void) {}

void r_rect_grad(float x, float y, float w, float h, rgba_t top, rgba_t bot)
{
    int ix0 = (int)floorf(x + 0.5f), iy0 = (int)floorf(y + 0.5f);
    int ix1 = (int)floorf(x + w + 0.5f), iy1 = (int)floorf(y + h + 0.5f);
    int i, j;
    for (j = iy0; j < iy1; j++) {
        float t = h > 1 ? (j - y) / h : 0;
        float r = CR(top) + (CR(bot) - CR(top)) * t;
        float g = CG(top) + (CG(bot) - CG(top)) * t;
        float b = CB(top) + (CB(bot) - CB(top)) * t;
        float a = CA(top) + (CA(bot) - CA(top)) * t;
        for (i = ix0; i < ix1; i++) blend(i, j, r, g, b, a);
    }
}

void r_rect(float x, float y, float w, float h, rgba_t c)
{
    r_rect_grad(x, y, w, h, c, c);
}

void r_tri(float x1, float y1, float x2, float y2, float x3, float y3, rgba_t c)
{
    int x, y;
    int minx = (int)floorf(fminf(x1, fminf(x2, x3))), maxx = (int)ceilf(fmaxf(x1, fmaxf(x2, x3)));
    int miny = (int)floorf(fminf(y1, fminf(y2, y3))), maxy = (int)ceilf(fmaxf(y1, fmaxf(y2, y3)));
    float d = (y2 - y3) * (x1 - x3) + (x3 - x2) * (y1 - y3);
    if (fabsf(d) < 1e-6f) return;
    for (y = miny; y < maxy; y++)
        for (x = minx; x < maxx; x++) {
            float px = x + 0.5f, py = y + 0.5f;
            float a = ((y2 - y3) * (px - x3) + (x3 - x2) * (py - y3)) / d;
            float b = ((y3 - y1) * (px - x3) + (x1 - x3) * (py - y3)) / d;
            if (a < 0 || b < 0 || a + b > 1) continue;
            blend(x, y, CR(c), CG(c), CB(c), CA(c));
        }
}

static float sample(float u, float v)
{
    int x0, y0;
    float fx, fy, a, b, c, d;
    u -= 0.5f; v -= 0.5f;
    x0 = (int)floorf(u); y0 = (int)floorf(v);
    fx = u - x0; fy = v - y0;
#define A(xx, yy) (((xx) < 0 || (yy) < 0 || (xx) >= ATLAS_W || (yy) >= ATLAS_H_PX) ? 0 : atlas_alpha[(yy) * ATLAS_W + (xx)] / 255.0f)
    a = A(x0, y0); b = A(x0 + 1, y0); c = A(x0, y0 + 1); d = A(x0 + 1, y0 + 1);
#undef A
    return (a * (1 - fx) + b * fx) * (1 - fy) + (c * (1 - fx) + d * fx) * fy;
}

void r_atlas(float x, float y, float w, float h,
             float u0, float v0, float u1, float v1, rgba_t c)
{
    int ix0 = (int)floorf(x), iy0 = (int)floorf(y);
    int ix1 = (int)ceilf(x + w), iy1 = (int)ceilf(y + h);
    int i, j;
    if (w <= 0 || h <= 0) return;
    for (j = iy0; j < iy1; j++)
        for (i = ix0; i < ix1; i++) {
            float px = i + 0.5f, py = j + 0.5f;
            float u, v, al;
            if (px < x || py < y || px > x + w || py > y + h) continue;
            u = u0 + (px - x) / w * (u1 - u0);
            v = v0 + (py - y) / h * (v1 - v0);
            /* evita amostrar fora da regiao (vizinhos no atlas) */
            if (u < u0 + 0.5f) u = u0 + 0.5f;
            if (v < v0 + 0.5f) v = v0 + 0.5f;
            if (u > u1 - 0.5f) u = u1 - 0.5f;
            if (v > v1 - 0.5f) v = v1 - 0.5f;
            al = sample(u, v) * CA(c);
            blend(i, j, CR(c), CG(c), CB(c), al);
        }
}

/* amostra bilinear (pre-multiplicada) de uma pagina T8 + paleta */
static void sample_page(int page, float u, float v, float out[4])
{
    const unsigned char *idx = assets_bin + asset_pages[page].idx_off;
    const unsigned char *pal = assets_bin + asset_pages[page].pal_off;
    int W = ASSET_PAGE_SIZE, H = (int)asset_pages[page].h;
    int x0, y0, k, dx, dy;
    float fx, fy;
    u -= 0.5f; v -= 0.5f;
    x0 = (int)floorf(u); y0 = (int)floorf(v);
    fx = u - x0; fy = v - y0;
    out[0] = out[1] = out[2] = out[3] = 0;
    for (dy = 0; dy < 2; dy++)
        for (dx = 0; dx < 2; dx++) {
            int xx = x0 + dx, yy = y0 + dy;
            float wgt = (dx ? fx : 1 - fx) * (dy ? fy : 1 - fy);
            const unsigned char *p;
            float a;
            if (xx < 0) xx = 0;
            if (yy < 0) yy = 0;
            if (xx >= W) xx = W - 1;
            if (yy >= H) yy = H - 1;
            p = pal + 4 * idx[yy * W + xx];
            a = p[3] / 255.0f;
            for (k = 0; k < 3; k++) out[k] += p[k] / 255.0f * a * wgt;
            out[3] += a * wgt;
        }
}

void r_image(int page, float x, float y, float w, float h,
             float u0, float v0, float u1, float v1, rgba_t c)
{
    int ix0 = (int)floorf(x), iy0 = (int)floorf(y);
    int ix1 = (int)ceilf(x + w), iy1 = (int)ceilf(y + h);
    int i, j;
    if (w <= 0 || h <= 0) return;
    for (j = iy0; j < iy1; j++) {
        if (j < 0 || j >= SCREEN_H) continue;
        for (i = ix0; i < ix1; i++) {
            float px = i + 0.5f, py = j + 0.5f, s[4], a;
            if (i < 0 || i >= SCREEN_W) continue;
            if (px < x || py < y || px > x + w || py > y + h) continue;
            sample_page(page, u0 + (px - x) / w * (u1 - u0), v0 + (py - y) / h * (v1 - v0), s);
            a = s[3] * CA(c);
            if (s[3] <= 0.001f) continue;
            blend(i, j, s[0] / s[3] * CR(c), s[1] / s[3] * CG(c), s[2] / s[3] * CB(c), a);
        }
    }
}

static void save(const char *dir, const char *name)
{
    char path[512];
    FILE *f;
    int x, y;
    snprintf(path, sizeof(path), "%s/%s.ppm", dir, name);
    f = fopen(path, "wb");
    if (!f) { perror(path); exit(1); }
    fprintf(f, "P6\n%d %d\n255\n", SCREEN_W, SCREEN_H);
    for (y = 0; y < SCREEN_H; y++)
        for (x = 0; x < SCREEN_W; x++) {
            int k;
            for (k = 0; k < 3; k++) {
                float v = fb[y][x][k];
                fputc(v <= 0 ? 0 : v >= 1 ? 255 : (int)(v * 255 + 0.5f), f);
            }
        }
    fclose(f);
}

/* Audio: se SIM_WAV estiver definido, grava o som da sessao em WAV. */
static FILE *wav;
static long wav_frames;

static void wav_u32(unsigned int v) { fputc(v & 255, wav); fputc(v >> 8 & 255, wav); fputc(v >> 16 & 255, wav); fputc(v >> 24, wav); }

static void wav_open(const char *path)
{
    wav = fopen(path, "wb");
    if (!wav) { perror(path); exit(1); }
    fwrite("RIFF\0\0\0\0WAVEfmt ", 1, 16, wav);
    wav_u32(16); wav_u32(1 | (2 << 16)); wav_u32(AUDIO_RATE); wav_u32(AUDIO_RATE * 4); wav_u32(4 | (16 << 16));
    fwrite("data\0\0\0\0", 1, 8, wav);
}

static void wav_close(void)
{
    if (!wav) return;
    fseek(wav, 4, SEEK_SET); wav_u32(36 + wav_frames * 4);
    fseek(wav, 40, SEEK_SET); wav_u32(wav_frames * 4);
    fclose(wav);
}

static void frame2(unsigned int p1, unsigned int p2);
static void frame(unsigned int pressed) { frame2(pressed, 0); }

static void frame2(unsigned int pressed, unsigned int pad2)
{
    static short buf[AUDIO_RATE / 60 * 2];
    ticks += 7919;
    ui_update(pressed, pad2);
    audio_render(buf, AUDIO_RATE / 60);
    if (wav) {
        fwrite(buf, 4, AUDIO_RATE / 60, wav);
        wav_frames += AUDIO_RATE / 60;
    }
    r_begin(RGBA(0, 0, 0, 255));
    ui_draw();
    r_end();
}

int main(int argc, char **argv)
{
    char *script, *tok;
    if (argc < 3) {
        fprintf(stderr, "uso: %s dir \"script\"\n", argv[0]);
        return 1;
    }
    script = strdup(argv[2]);
    audio_init();
    if (getenv("SIM_WAV")) wav_open(getenv("SIM_WAV"));
    ui_init();
    frame(0);
    for (tok = strtok(script, " "); tok; tok = strtok(NULL, " ")) {
        unsigned int b = 0;
        if (tok[0] == 'w') {
            int n = atoi(tok + 1);
            while (n-- > 0) frame(0);
            continue;
        }
        if (!strncmp(tok, "shot:", 5)) {
            save(argv[1], tok + 5);
            continue;
        }
        {
            int pad2 = tok[0] == '2' && tok[1];
            const char *k = pad2 ? tok + 1 : tok;
            switch (k[0]) {
            case 'X': b = BTN_CROSS; break;
            case 'O': b = BTN_CIRCLE; break;
            case 'T': b = BTN_TRIANGLE; break;
            case 'Q': b = BTN_SQUARE; break;
            case 'S': b = BTN_START; break;
            case 'L': b = BTN_LEFT; break;
            case 'R': b = BTN_RIGHT; break;
            case 'U': b = BTN_UP; break;
            case 'D': b = BTN_DOWN; break;
            }
            if (pad2) frame2(0, b);
            else frame(b);
            frame(0);
        }
    }
    wav_close();
    return 0;
}
