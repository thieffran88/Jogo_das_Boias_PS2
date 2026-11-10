/*
 * Simulador de PC: roda a mesma UI do jogo com um rasterizador em
 * software e grava capturas de tela (PPM). Serve para conferir o visual
 * sem precisar de um PS2/emulador.
 *
 *   ./build/sim saida_dir "script"
 *
 * O script e uma lista separada por espacos de comandos:
 *   X O T Q S L R U D   botoes (cruz, bola, triangulo, quadrado, start,
 *                       esquerda, direita, cima, baixo)
 *   wN                  espera N quadros
 *   shot:nome           grava nome.ppm
 */
#include "../src/render.h"
#include "../src/atlas.h"
#include "../src/ui.h"

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

static void frame(unsigned int pressed)
{
    ticks += 7919;
    ui_update(pressed);
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
        switch (tok[0]) {
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
        frame(b);
        frame(0);
    }
    return 0;
}
