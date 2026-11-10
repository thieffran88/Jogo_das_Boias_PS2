/*
 * Camada de desenho abstrata. A tela virtual tem 640x448; cada backend
 * (PS2/gsKit ou o simulador de PC) escala para a resolucao real.
 * Cores sao 0xRRGGBBAA (alfa 255 = opaco).
 */
#ifndef RENDER_H
#define RENDER_H

#define SCREEN_W 640
#define SCREEN_H 448

typedef unsigned int rgba_t;

#define RGBA(r, g, b, a) ((rgba_t)(((r) << 24) | ((g) << 16) | ((b) << 8) | (a)))
#define RGB(hex) ((rgba_t)(((unsigned)(hex) << 8) | 0xFFu))
#define WITH_ALPHA(c, a) (((c) & 0xFFFFFF00u) | ((a) & 0xFFu))

/* ---- implementados por cada backend ---- */
void r_begin(rgba_t clear);
void r_end(void);
void r_rect(float x, float y, float w, float h, rgba_t c);
void r_rect_grad(float x, float y, float w, float h, rgba_t top, rgba_t bottom);
/* quadrado texturizado com o atlas (u/v em pixels do atlas), tingido por c */
void r_atlas(float x, float y, float w, float h,
             float u0, float v0, float u1, float v1, rgba_t c);

/* quadrado texturizado com uma pagina de sprites (assets.h), tingido por c */
void r_image(int page, float x, float y, float w, float h,
             float u0, float v0, float u1, float v1, rgba_t c);

/* ---- comuns (render.c) ---- */
/* sprites (ids SPR_* de assets.h) */
float r_spr_w(int id);
float r_spr_h(int id);
void  r_spr(int id, float x, float y, float w, float h, rgba_t c);
void  r_spr_inset(int id, float x, float y, float w, float h, rgba_t c);
void  r_spr_c(int id, float cx, float cy, float scale, rgba_t c);
void  r_spr_b(int id, float cx, float by, float scale, rgba_t c); /* ancorado embaixo */
/* fonte amarela de titulos (h = altura das letras) */
float r_ytext(float x, float y, float h, rgba_t c, const char *s);
float r_ytext_w(const char *s, float h);
void  r_ytext_c(float cx, float y, float h, rgba_t c, const char *s);

void  r_shape(int shape, float x, float y, float w, float h, rgba_t c);
void  r_shape_c(int shape, float cx, float cy, float size, rgba_t c);
float r_text(float x, float y, float scale, rgba_t c, const char *s);
float r_text_w(const char *s, float scale);
void  r_text_c(float cx, float y, float scale, rgba_t c, const char *s);
void  r_text_shadow(float x, float y, float scale, rgba_t c, const char *s);
void  r_text_c_shadow(float cx, float y, float scale, rgba_t c, const char *s);
void  r_text_outline(float x, float y, float scale, rgba_t c, rgba_t o, const char *s);
void  r_text_c_outline(float cx, float y, float scale, rgba_t c, rgba_t o, const char *s);
void  r_panel(float x, float y, float w, float h, rgba_t c);

#endif
