/*
 * Jogo das Boias PS2 - telas, animacoes e fluxo de jogo.
 * Usa apenas render.h (desenho) e game.h (regras); nao depende do PS2.
 */
#include "ui.h"
#include "render.h"
#include "atlas.h"
#include "assets.h"
#include "game.h"
#include "ai.h"
#include "audio.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Jogadores                                                           */
/* ------------------------------------------------------------------ */

static const char *PNAME[MAX_PLAYERS] = { "Vermelho", "Amarelo", "Azul", "Verde" };
static const char *PCOR_F[MAX_PLAYERS] = { "vermelha", "amarela", "azul", "verde" };
static const int P_RING[MAX_PLAYERS] = { SPR_RING_RED, SPR_RING_YELLOW, SPR_RING_BLUE, SPR_RING_GREEN };
static const int P_CHAR[MAX_PLAYERS] = { SPR_CHAR_P1, SPR_CHAR_P2, SPR_CHAR_P3, SPR_CHAR_P4 };
static const int P_BADGE[MAX_PLAYERS] = { SPR_BADGE_P1, SPR_BADGE_P2, SPR_BADGE_P3, SPR_BADGE_P4 };
enum { REACT_IDLE, REACT_HAPPY, REACT_SAD };
static const int P_REACT[MAX_PLAYERS][3] = {
    { SPR_R_P1_IDLE, SPR_R_P1_HAPPY, SPR_R_P1_SAD },
    { SPR_R_P2_IDLE, SPR_R_P2_HAPPY, SPR_R_P2_SAD },
    { SPR_R_P3_IDLE, SPR_R_P3_HAPPY, SPR_R_P3_SAD },
    { SPR_R_P4_IDLE, SPR_R_P4_HAPPY, SPR_R_P4_SAD },
};

/* ------------------------------------------------------------------ */
/* Constantes visuais                                                  */
/* ------------------------------------------------------------------ */

#define C_WHITE   RGB(0xFFFFFF)
#define C_INK     RGB(0x0D3B66)
#define C_DIM     RGB(0xB3E5FC)
#define C_GOLD    RGB(0xFFD54F)
#define C_DANGER  RGB(0xFF5252)
#define C_CREAM   RGB(0xFFF6E0)
#define C_PANEL   RGBA(8, 52, 96, 215)

#define POOL_X 24.0f
#define POOL_Y 88.0f
#define POOL_W 592.0f
#define POOL_H 208.0f
#define LANE_W (POOL_W / NUM_LANES)
#define CHIP_S 0.90f          /* escala da boia nas raias */

#define PANEL_Y 304.0f
#define PANEL_H 48.0f
#define PANEL_W 142.0f

#define BAR_Y 356.0f
#define CARD_X 206.0f
#define CARD_Y 362.0f
#define CARD_W 78.0f
#define CARD_H 54.0f

enum { SCR_TITLE, SCR_SETUP, SCR_RULES, SCR_CREDITS, SCR_GAME };
enum {
    PH_START_ROLL, PH_START_SHOW, PH_WAIT_ROLL, PH_ROLLING,
    PH_CHOOSE, PH_PLACING, PH_GAMEOVER
};

typedef struct {
    float x0, y0, x1, y1;
    int color;
    int t, dur, delay;
    float scale;
} Fly;

#define MAX_FLY 24

static struct {
    int screen;
    unsigned int frame;

    int menu_sel;

    int setup_players;
    int setup_cpu[MAX_PLAYERS];
    int setup_row, setup_card;

    int rules_page;
    int rules_return;

    Game g;
    int phase, timer;
    Move moves[MAX_MOVES];
    int n_moves, sel, cpu_target;
    Move cur;
    int place_idx, placed;
    int start_dice[MAX_PLAYERS];
    int start_active[MAX_PLAYERS];
    int start_tie;
    int dice_show[3];

    char msg[128];
    rgba_t msg_col;
    int banner_timer;
    int lane_flash[NUM_LANES + 1];

    /* personagem que aparece reagindo atras do painel */
    int react_player, react_kind, react_timer;

    int paused, pause_sel;
    int toast_timer;

    Fly fly[MAX_FLY];
    int n_fly;
} S;

/* ------------------------------------------------------------------ */
/* Utilidades                                                          */
/* ------------------------------------------------------------------ */

static unsigned int ui_rand_state = 0x9E3779B9u;
static int urand(int n)
{
    unsigned int x = ui_rand_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    ui_rand_state = x;
    return (int)(x % (unsigned int)n);
}

static float pulse(float speed) { return 0.5f + 0.5f * sinf(S.frame * speed); }

static void set_msg(rgba_t c, const char *text)
{
    snprintf(S.msg, sizeof(S.msg), "%s", text);
    S.msg_col = c;
}

static void react(int player, int kind)
{
    S.react_player = player;
    S.react_kind = kind;
    S.react_timer = 80;
}

static float lane_cx(int l) { return POOL_X + (l - 0.5f) * LANE_W; }
static float slot_cy(int s) { return POOL_Y + POOL_H - 30.0f - s * 52.0f; }
#define TOWER_Y (POOL_Y + 150.0f)

static float panel_x(int p)
{
    int n = S.g.num_players;
    float total = n * (PANEL_W + 8.0f) - 8.0f;
    return 320.0f - total * 0.5f + p * (PANEL_W + 8.0f);
}

static void panel_center(int p, float *x, float *y)
{
    *x = panel_x(p) + 26.0f;
    *y = PANEL_Y + PANEL_H * 0.5f;
}

static void add_fly(float x0, float y0, float x1, float y1, int color,
                    int dur, int delay, float scale)
{
    Fly *f;
    if (S.n_fly >= MAX_FLY) return;
    f = &S.fly[S.n_fly++];
    f->x0 = x0; f->y0 = y0; f->x1 = x1; f->y1 = y1;
    f->color = color; f->t = 0; f->dur = dur; f->delay = delay; f->scale = scale;
}

static void update_flies(void)
{
    int i, j = 0;
    for (i = 0; i < S.n_fly; i++) {
        Fly *f = &S.fly[i];
        if (f->delay > 0) f->delay--;
        else f->t++;
        if (f->t < f->dur) S.fly[j++] = *f;
    }
    S.n_fly = j;
}

/* ------------------------------------------------------------------ */
/* Elementos de desenho                                                */
/* ------------------------------------------------------------------ */

static void draw_chip(float cx, float cy, float scale, int color, int alpha)
{
    float w = r_spr_w(P_RING[color]) * scale;
    r_shape(SHAPE_DISC, cx - w * 0.46f, cy + w * 0.08f, w * 0.96f, w * 0.42f,
            RGBA(0, 30, 60, alpha * 60 / 255));
    r_spr_c(P_RING[color], cx, cy, scale, RGBA(255, 255, 255, alpha));
}

static void draw_die(float cx, float cy, float size, int v, int alpha)
{
    if (v < 1 || v > 6) {
        r_shape_c(SHAPE_ROUNDRECT, cx, cy, size * 0.9f, RGBA(255, 255, 255, 40));
        return;
    }
    r_spr_c(SPR_DIE_1 + v - 1, cx, cy, size / r_spr_h(SPR_DIE_1 + v - 1),
            RGBA(255, 255, 255, alpha));
}

/* Agua animada: mistura dois quadros consecutivos. */
static void draw_water(float x, float y, float w, float h, int alpha)
{
    float t = S.frame / 7.0f;
    int f = (int)t % WATER_FRAMES;
    int n = (f + 1) % WATER_FRAMES;
    float k = t - (int)t;
    r_spr_inset(SPR_WATER_0 + f, x, y, w, h, RGBA(255, 255, 255, alpha));
    r_spr_inset(SPR_WATER_0 + n, x, y, w, h, RGBA(255, 255, 255, (int)(alpha * k)));
}

static void draw_hint(float x, float y, int icon, const char *text)
{
    r_spr_c(icon, x + 12, y + 10, 0.8f, C_WHITE);
    r_text_outline(x + 28, y, 0.6f, C_WHITE, RGBA(0, 40, 80, 200), text);
}

static float hint_w(const char *text) { return 34 + r_text_w(text, 0.6f) + 16; }

/* Varias dicas centralizadas: icones e textos alternados. */
static void draw_hints(float cy, int n, const int *icons, const char **texts)
{
    float w = 0, x;
    int i;
    for (i = 0; i < n; i++) w += hint_w(texts[i]);
    x = 320 - w * 0.5f;
    for (i = 0; i < n; i++) {
        draw_hint(x, cy, icons[i], texts[i]);
        x += hint_w(texts[i]);
    }
}

static void wrap_text(float x, float y, float maxw, float scale, rgba_t c,
                      const char *s, float line_h)
{
    char line[240], word[80];
    int ll = 0;
    line[0] = 0;
    while (*s) {
        int wl = 0;
        while (*s == ' ') s++;
        if (*s == '\n') {
            r_text(x, y, scale, c, line);
            y += line_h;
            ll = 0; line[0] = 0;
            s++;
            continue;
        }
        while (*s && *s != ' ' && *s != '\n' && wl < 78) word[wl++] = *s++;
        word[wl] = 0;
        if (!wl) continue;
        {
            char test[320];
            snprintf(test, sizeof(test), "%s%s%s", line, ll ? " " : "", word);
            if (ll && r_text_w(test, scale) > maxw) {
                r_text(x, y, scale, c, line);
                y += line_h;
                snprintf(line, sizeof(line), "%s", word);
            } else {
                snprintf(line, sizeof(line), "%s", test);
            }
            ll = (int)strlen(line);
        }
    }
    if (ll) r_text(x, y, scale, c, line);
}

/* Cenario do clube: ceu, nuvens, arbustos, palmeira, guarda-sol e agua. */
static void draw_scene(float water_y)
{
    float t = S.frame * 0.4f;
    int i;
    r_rect_grad(0, 0, SCREEN_W, water_y, RGB(0x4FB3FF), RGB(0xCDEFFF));
    for (i = 0; i < 4; i++) {
        float x = fmodf(i * 190.0f + t * (0.25f + 0.1f * i), SCREEN_W + 200) - 100;
        r_spr_c(i % 2 ? SPR_CLOUD_1 : SPR_CLOUD_2, x, 40 + (i % 3) * 38, 1.0f, C_WHITE);
    }
    for (i = 0; i < 6; i++)
        r_spr_b(i % 2 ? SPR_BUSH_1 : SPR_BUSH_2, 50 + i * 110.0f, water_y - 4, 1.0f, C_WHITE);
    r_spr_b(SPR_PALM, 40, water_y - 2, 1.0f, C_WHITE);
    r_spr_b(SPR_UMBRELLA, 600, water_y + 2, 0.95f, C_WHITE);
    /* borda da piscina */
    r_rect(0, water_y - 6, SCREEN_W, 10, RGB(0xF3E2C3));
    r_rect(0, water_y + 4, SCREEN_W, 2, RGB(0xC9B48F));
    draw_water(0, water_y + 6, SCREEN_W, SCREEN_H - water_y - 6, 255);
}

static void draw_button(int spr, float cx, float cy, int selected)
{
    float s = selected ? 1.08f + 0.04f * pulse(0.15f) : 0.95f;
    rgba_t c = selected ? C_WHITE : RGBA(190, 200, 215, 255);
    if (selected)
        r_shape_c(SHAPE_GLOW, cx, cy, r_spr_w(spr) * 1.3f, RGBA(255, 255, 200, 140));
    r_spr_c(spr, cx, cy, s, c);
}

/* ------------------------------------------------------------------ */
/* Tabuleiro                                                           */
/* ------------------------------------------------------------------ */

static void draw_pool(void)
{
    int l, s, i;
    float t = S.frame * 0.05f;

    /* borda (pedras claras) */
    r_panel(POOL_X - 12, POOL_Y - 10, POOL_W + 24, POOL_H + 20, RGB(0x3E5871));
    r_panel(POOL_X - 10, POOL_Y - 8, POOL_W + 20, POOL_H + 16, RGB(0xF3E2C3));
    r_rect(POOL_X - 2, POOL_Y - 2, POOL_W + 4, POOL_H + 4, RGB(0x1C4E80));
    draw_water(POOL_X, POOL_Y, POOL_W, POOL_H, 255);

    for (l = 1; l <= NUM_LANES; l++) {
        float x0 = POOL_X + (l - 1) * LANE_W;
        if (l == SAFE_LANE) continue;
        r_rect(lane_cx(l) - 3, POOL_Y + 12, 6, POOL_H - 24, RGBA(10, 40, 90, 60));
        if (S.lane_flash[l] > 0 && (S.lane_flash[l] / 4) % 2 == 0)
            r_rect(x0, POOL_Y, LANE_W, POOL_H, RGBA(255, 60, 60, 120));
    }

    /* raia 7: deck com a cadeira do salva-vidas */
    {
        float x0 = POOL_X + (SAFE_LANE - 1) * LANE_W;
        r_rect_grad(x0, POOL_Y, LANE_W, POOL_H, RGB(0xFFF1D0), RGB(0xEFCF95));
        for (i = 1; i < 6; i++)
            r_rect(x0, POOL_Y + i * 35.0f, LANE_W, 1, RGBA(160, 120, 70, 60));
        r_spr_b(SPR_LIFEGUARD, lane_cx(SAFE_LANE), POOL_Y + 104, 0.78f, C_WHITE);
    }

    /* cordas entre as raias */
    for (l = 1; l < NUM_LANES; l++) {
        float x = POOL_X + l * LANE_W;
        float pw = r_spr_w(SPR_ROPE) * 0.85f;
        float ph = (POOL_H + 4.0f * 3) / 5.0f;
        for (i = 0; i < 5; i++)
            r_spr(SPR_ROPE, x - pw * 0.5f, POOL_Y + i * (ph - 3), pw, ph, C_WHITE);
    }

    /* placas com os numeros */
    for (l = 1; l <= NUM_LANES; l++) {
        char b[4];
        float x = lane_cx(l) - 18, y = POOL_Y - 46;
        int seven = l == SAFE_LANE;
        snprintf(b, sizeof(b), "%d", l);
        r_panel(x + 1, y + 3, 36, 36, RGBA(0, 20, 50, 90));
        r_panel(x - 2, y - 2, 40, 40, RGB(0x1B1B1B));
        r_rect_grad(x + 2, y + 2, 32, 32, seven ? RGB(0xFFE14D) : RGB(0xFFF8EA),
                    seven ? RGB(0xF2B705) : RGB(0xE9D3B0));
        r_text_c(x + 18, y + 2, l >= 10 ? 0.92f : 1.05f, RGB(0x111111), b);
    }

    /* boias nas raias */
    for (l = 1; l <= NUM_LANES; l++) {
        if (l == SAFE_LANE) continue;
        for (s = 0; s < S.g.lane_n[l]; s++) {
            float bob = sinf(t * 1.5f + l * 0.9f + s) * 2.0f;
            draw_chip(lane_cx(l), slot_cy(s) + bob, CHIP_S, S.g.lane[l][s], 255);
        }
    }

    /* boias seguras na raia 7 */
    {
        float y = POOL_Y + 120;
        int c;
        for (c = 0; c < S.g.num_players; c++) {
            char b[8];
            if (!S.g.safe[c]) continue;
            draw_chip(lane_cx(SAFE_LANE) - 9, y, 0.5f, c, 255);
            snprintf(b, sizeof(b), "%d", S.g.safe[c]);
            r_ytext(lane_cx(SAFE_LANE) + 6, y - 10, 20, C_WHITE, b);
            y += 22;
        }
    }
}

/* Destaca as raias da opcao escolhida e as boias que seriam expulsas. */
static void draw_move_preview(const Move *m)
{
    Game t = S.g;
    int k[NUM_LANES + 1];
    int i;
    float p = pulse(0.15f);
    memset(k, 0, sizeof(k));
    for (i = 0; i < m->n; i++) {
        int l = m->lanes[i];
        int n0 = S.g.lane_n[l];
        PlaceResult r;
        float x0 = POOL_X + (l - 1) * LANE_W;
        k[l]++;
        r_rect(x0 + 2, POOL_Y, LANE_W - 4, POOL_H, RGBA(255, 250, 170, (int)(50 + 50 * p)));
        if (k[l] == 1)
            r_spr_c(SPR_ARROW_D, lane_cx(l), POOL_Y + 14 + 6 * p, 0.85f, C_WHITE);
        r = game_place(&t, l);
        if (l == SAFE_LANE) {
            r_spr_c(SPR_STAR, lane_cx(l), POOL_Y + 116, 0.9f + 0.2f * p, C_WHITE);
            continue;
        }
        if (r.kicked != NO_CHIP) {
            int s = LANE_CAP - k[l];
            if (s >= 0 && s < n0) {
                rgba_t c = r.penalty ? C_DANGER : RGB(0xFFEB3B);
                r_shape_c(SHAPE_RING_OUTLINE, lane_cx(l), slot_cy(s), 50 + 4 * p, c);
                if (r.penalty)
                    r_spr_c(SPR_FX_EXCLAIM, lane_cx(l) + 14, slot_cy(s) - 22, 0.9f, C_WHITE);
            }
        }
    }
}

static void draw_reaction(void)
{
    float x, y, k, a;
    int spr;
    if (S.react_timer <= 0) return;
    panel_center(S.react_player, &x, &y);
    x = panel_x(S.react_player) + PANEL_W - 36;
    spr = P_REACT[S.react_player][S.react_kind];
    /* sobe de tras do painel, espera e desce */
    if (S.react_timer > 68) k = (80 - S.react_timer) / 12.0f;
    else if (S.react_timer < 12) k = S.react_timer / 12.0f;
    else k = 1.0f;
    a = k;
    r_spr_b(spr, x, PANEL_Y + 14 - 58 * k, 0.9f, RGBA(255, 255, 255, (int)(255 * a)));
}

static void draw_panels(void)
{
    int p, c;
    for (p = 0; p < S.g.num_players; p++) {
        float x = panel_x(p);
        int active = (p == S.g.current && S.phase != PH_START_ROLL && S.phase != PH_START_SHOW);
        int total = game_stock_total(&S.g, p);
        char b[16];
        float fx;
        if (active)
            r_panel(x - 3, PANEL_Y - 3, PANEL_W + 6, PANEL_H + 6,
                    WITH_ALPHA(C_GOLD, (int)(150 + 100 * pulse(0.12f))));
        r_panel(x, PANEL_Y, PANEL_W, PANEL_H, C_PANEL);
        r_spr_c(P_BADGE[p], x + 24, PANEL_Y + 22, 0.72f, C_WHITE);
        r_text(x + 48, PANEL_Y + 2, 0.58f, C_WHITE, PNAME[p]);
        if (S.g.is_cpu[p])
            r_text(x + 50 + r_text_w(PNAME[p], 0.58f), PANEL_Y + 4, 0.45f, C_DIM, "CPU");
        snprintf(b, sizeof(b), "%d", total);
        fx = r_ytext(x + 48, PANEL_Y + 20, 24, C_WHITE, b) + 6;
        /* boias de outras cores na reserva */
        for (c = 0; c < MAX_PLAYERS; c++) {
            if (c == p || !S.g.stock[p][c]) continue;
            draw_chip(fx + 10, PANEL_Y + 34, 0.38f, c, 255);
            snprintf(b, sizeof(b), "%d", S.g.stock[p][c]);
            r_text_outline(fx + 21, PANEL_Y + 24, 0.5f, C_WHITE, RGBA(0, 0, 0, 160), b);
            fx += 32;
        }
    }
}

static void move_label(const Move *m, char *top, char *bot, int n)
{
    int i, j, tl = 0, bl = 0;
    top[0] = bot[0] = 0;
    for (i = 0; i < m->n; i++) {
        tl += snprintf(top + tl, n - tl, "%s%d", i ? " " : "", m->lanes[i]);
        bl += snprintf(bot + bl, n - bl, "%s", i ? " | " : "");
        for (j = 0; j < 3; j++)
            if (m->groups[i] & (1 << j))
                bl += snprintf(bot + bl, n - bl, "%s%d",
                               (m->groups[i] & ((1 << j) - 1)) ? "+" : "", S.g.dice[j]);
    }
}

static void draw_bottom_bar(void)
{
    int i;
    int show_dice = (S.phase == PH_ROLLING || S.phase == PH_CHOOSE || S.phase == PH_PLACING);
    r_panel(16, BAR_Y, 608, 86, C_PANEL);

    if (S.phase != PH_START_ROLL && S.phase != PH_START_SHOW) {
        for (i = 0; i < 3; i++) {
            float cx = 50 + i * 54.0f, cy = BAR_Y + 34;
            if (S.phase == PH_ROLLING)
                draw_die(cx + urand(7) - 3, cy + urand(7) - 3, 50, S.dice_show[i], 255);
            else if (show_dice)
                draw_die(cx, cy, 50, S.g.dice[i], 255);
            else
                draw_die(cx, cy, 50, 0, 255);
        }
    }

    if (S.phase == PH_CHOOSE) {
        for (i = 0; i < S.n_moves; i++) {
            char top[24], bot[32];
            float x = CARD_X + i * (CARD_W + 4);
            int selc = (i == S.sel);
            float lift = selc ? 4 : 0;
            rgba_t bg = selc ? RGB(0xFFE082) : RGBA(255, 246, 224, 215);
            if (selc)
                r_panel(x - 3, CARD_Y - 3 - lift, CARD_W + 6, CARD_H + 6,
                        WITH_ALPHA(RGB(0xFF8F00), (int)(160 + 90 * pulse(0.2f))));
            r_panel(x, CARD_Y - lift, CARD_W, CARD_H, bg);
            move_label(&S.moves[i], top, bot, sizeof(top));
            r_text_c(x + CARD_W / 2, CARD_Y + 3 - lift,
                     r_text_w(top, 0.9f) > CARD_W - 8 ? 0.7f : 0.9f, C_INK, top);
            r_text_c(x + CARD_W / 2, CARD_Y + 32 - lift,
                     r_text_w(bot, 0.5f) > CARD_W - 6 ? 0.42f : 0.5f, RGB(0x37474F), bot);
            if (S.moves[i].uses_seven)
                r_spr_c(SPR_STAR, x + CARD_W - 6, CARD_Y + 2 - lift, 0.6f, C_WHITE);
        }
    } else if (S.phase == PH_WAIT_ROLL) {
        if (S.g.is_cpu[S.g.current]) {
            char b[64];
            snprintf(b, sizeof(b), "%s (CPU) vai rolar...", PNAME[S.g.current]);
            r_text_shadow(CARD_X + 6, BAR_Y + 16, 0.8f, C_WHITE, b);
        } else {
            r_spr_c(SPR_ICON_X, CARD_X + 22, BAR_Y + 30, 1.0f + 0.15f * pulse(0.2f), C_WHITE);
            r_text_shadow(CARD_X + 44, BAR_Y + 16, 0.8f, C_WHITE, "Rolar os dados!");
        }
    } else if (S.phase == PH_ROLLING) {
        r_text_shadow(CARD_X + 6, BAR_Y + 16, 0.8f, C_DIM, "Rolando...");
    } else if (S.phase == PH_START_ROLL || S.phase == PH_START_SHOW) {
        int p;
        for (p = 0; p < S.g.num_players; p++) {
            float x = 40 + p * 146.0f;
            int v = S.start_dice[p];
            int a = S.start_active[p] ? 255 : 110;
            if (S.phase == PH_START_ROLL && S.start_active[p])
                v = 1 + (int)((S.frame / 4 + p * 7) % 6);
            r_spr_c(P_BADGE[p], x + 22, BAR_Y + 32, 0.7f, RGBA(255, 255, 255, a));
            if (v) draw_die(x + 80, BAR_Y + 32, 48, v, a);
        }
    }

    r_text_shadow(28, BAR_Y + 64, 0.6f, S.msg_col, S.msg);
}

static void draw_header(void)
{
    char b[64];
    float x;
    r_ytext(18, 8, 26, C_WHITE, "JOGO DAS BOIAS");
    if (S.phase == PH_START_ROLL || S.phase == PH_START_SHOW) {
        r_text_outline(400, 12, 0.75f, C_WHITE, RGBA(0, 40, 80, 220), "Quem começa?");
        return;
    }
    snprintf(b, sizeof(b), "Vez do %s", PNAME[S.g.current]);
    x = 620 - r_text_w(b, 0.75f);
    r_spr_c(P_BADGE[S.g.current], x - 20, 26, 0.55f, C_WHITE);
    r_text_outline(x, 12, 0.75f, C_WHITE, RGBA(0, 40, 80, 220), b);
    if (S.g.bonus_roll && S.phase != PH_GAMEOVER) {
        r_spr_c(SPR_STAR, x - 58, 26, 0.75f + 0.1f * pulse(0.25f), C_WHITE);
        r_text_outline(x - 76 - r_text_w("BÔNUS", 0.6f), 16, 0.6f, C_GOLD, RGBA(0, 40, 80, 220), "BÔNUS");
    }
}

static void draw_flies(void)
{
    int i;
    for (i = 0; i < S.n_fly; i++) {
        Fly *f = &S.fly[i];
        float u, x, y;
        if (f->delay > 0) continue;
        u = (float)f->t / (float)f->dur;
        u = u * u * (3 - 2 * u);
        x = f->x0 + (f->x1 - f->x0) * u;
        y = f->y0 + (f->y1 - f->y0) * u - sinf(u * 3.14159f) * 60.0f;
        draw_chip(x, y, f->scale * (1.0f + 0.3f * sinf(u * 3.14159f)), f->color, 255);
    }
}

/* ------------------------------------------------------------------ */
/* Fluxo do jogo                                                       */
/* ------------------------------------------------------------------ */

static void begin_turn(void)
{
    S.phase = PH_WAIT_ROLL;
    S.timer = 0;
    audio_sfx(SFX_POP);
    if (!S.g.bonus_roll) {
        char b[64];
        snprintf(b, sizeof(b), "Vez do %s.", PNAME[S.g.current]);
        set_msg(C_DIM, b);
    }
}

static void start_game(void)
{
    int p;
    game_init(&S.g, S.setup_players, S.setup_cpu, platform_ticks() ^ (S.frame * 2654435761u));
    ui_rand_state ^= platform_ticks() | 1u;
    S.screen = SCR_GAME;
    S.paused = 0;
    S.n_fly = 0;
    S.banner_timer = 0;
    S.react_timer = 0;
    memset(S.lane_flash, 0, sizeof(S.lane_flash));
    for (p = 0; p < MAX_PLAYERS; p++) {
        S.start_active[p] = p < S.g.num_players;
        S.start_dice[p] = 0;
    }
    S.phase = PH_START_ROLL;
    S.timer = 0;
    set_msg(C_DIM, "Cada jogador rola um dado: o maior começa.");
}

static void finish_move(void)
{
    char b[96];
    if (S.g.winner >= 0) {
        S.phase = PH_GAMEOVER;
        S.timer = 0;
        snprintf(b, sizeof(b), "%s esvaziou a reserva e venceu!", PNAME[S.g.winner]);
        set_msg(C_GOLD, b);
        return;
    }
    if (game_end_move(&S.g, &S.cur)) {
        S.banner_timer = 90;
        audio_sfx(SFX_SPARKLE);
        react(S.g.current, REACT_HAPPY);
        snprintf(b, sizeof(b), "Somou 7! %s ganha mais um lance.", PNAME[S.g.current]);
        set_msg(C_GOLD, b);
        S.phase = PH_WAIT_ROLL;
        S.timer = 0;
        return;
    }
    begin_turn();
}

static void start_place(void)
{
    int lane, color;
    float px, py, tx, ty;
    if (S.place_idx >= S.cur.n || S.g.winner >= 0) {
        finish_move();
        return;
    }
    lane = S.cur.lanes[S.place_idx];
    color = game_next_color(&S.g, S.g.current);
    if (color == NO_CHIP) {
        finish_move();
        return;
    }
    panel_center(S.g.current, &px, &py);
    tx = lane_cx(lane);
    ty = lane == SAFE_LANE ? TOWER_Y : slot_cy(0);
    add_fly(px, py, tx, ty, color, 24, 0, CHIP_S);
    S.phase = PH_PLACING;
    S.timer = 0;
    S.placed = 0;
}

static void apply_place(void)
{
    int lane = S.cur.lanes[S.place_idx];
    int before[LANE_CAP], n0 = S.g.lane_n[lane], s;
    int me = S.g.current;
    PlaceResult r;
    for (s = 0; s < LANE_CAP; s++) before[s] = S.g.lane[lane][s];

    r = game_place(&S.g, lane);
    S.placed = 1;

    if (lane == SAFE_LANE) {
        set_msg(C_GOLD, "Boia segura com o salva-vidas da raia 7!");
        audio_sfx(SFX_CHIME);
    } else if (r.penalty) {
        float px, py;
        panel_center(me, &px, &py);
        /* a boia expulsa (do topo) e todas as outras voltam para mim */
        add_fly(lane_cx(lane), slot_cy(LANE_CAP - 1) - 20, px, py, r.kicked, 30, 0, CHIP_S);
        add_fly(lane_cx(lane), slot_cy(0), px, py, r.color, 30, 6, CHIP_S);
        for (s = 0; s < n0 - 1; s++)
            add_fly(lane_cx(lane), slot_cy(s + 1), px, py, before[s], 30, 10 + s * 5, CHIP_S);
        S.lane_flash[lane] = 48;
        snprintf(S.msg, sizeof(S.msg), "PENALIDADE! %s expulsou a própria boia e recolhe %d da raia %d.",
                 PNAME[me], r.penalty_n, lane);
        S.msg_col = C_DANGER;
        react(me, REACT_SAD);
        audio_sfx(SFX_WHISTLE);
    } else if (r.kicked != NO_CHIP) {
        float px, py;
        panel_center(r.kicked, &px, &py);
        add_fly(lane_cx(lane), slot_cy(LANE_CAP - 1) - 20, px, py, r.kicked, 30, 0, CHIP_S);
        snprintf(S.msg, sizeof(S.msg), "Boia %s expulsa da raia %d! Volta para o %s.",
                 PCOR_F[r.kicked], lane, PNAME[r.kicked]);
        S.msg_col = RGB(0xFFEB3B);
        react(r.kicked, REACT_SAD);
        audio_sfx(SFX_BOING);
    } else {
        snprintf(S.msg, sizeof(S.msg), "Boia colocada na raia %d.", lane);
        audio_sfx(SFX_SPLASH);
        S.msg_col = C_WHITE;
    }
}

static void update_game(unsigned int pressed)
{
    int p, l;

    if (S.banner_timer > 0) S.banner_timer--;
    if (S.react_timer > 0) S.react_timer--;
    for (l = 0; l <= NUM_LANES; l++)
        if (S.lane_flash[l] > 0) S.lane_flash[l]--;

    if (S.paused) {
        if (pressed & BTN_UP) S.pause_sel = (S.pause_sel + 2) % 3;
        if (pressed & BTN_DOWN) S.pause_sel = (S.pause_sel + 1) % 3;
        if (pressed & (BTN_START | BTN_CIRCLE)) S.paused = 0;
        else if (pressed & BTN_CROSS) {
            if (S.pause_sel == 0) S.paused = 0;
            else if (S.pause_sel == 1) {
                S.rules_page = 0;
                S.rules_return = SCR_GAME;
                S.screen = SCR_RULES;
            } else {
                S.paused = 0;
                S.screen = SCR_TITLE;
            }
        }
        return;
    }
    if ((pressed & BTN_START) && S.phase != PH_GAMEOVER) {
        S.paused = 1;
        S.pause_sel = 0;
        return;
    }

    update_flies();
    S.timer++;

    switch (S.phase) {
    case PH_START_ROLL:
        if (S.timer == 1) audio_sfx(SFX_DICE);
        if (S.timer >= 45) {
            audio_sfx(SFX_DICE_LAND);
            int best = 0, nbest = 0;
            for (p = 0; p < S.g.num_players; p++)
                if (S.start_active[p]) {
                    S.start_dice[p] = game_roll_die(&S.g);
                    if (S.start_dice[p] > best) best = S.start_dice[p];
                }
            for (p = 0; p < S.g.num_players; p++)
                if (S.start_active[p] && S.start_dice[p] == best) nbest++;
            if (nbest == 1) {
                char b[64];
                for (p = 0; p < S.g.num_players; p++)
                    if (S.start_active[p] && S.start_dice[p] == best) S.g.current = p;
                S.start_tie = 0;
                snprintf(b, sizeof(b), "%s tirou o maior número e começa!", PNAME[S.g.current]);
                set_msg(C_GOLD, b);
                react(S.g.current, REACT_HAPPY);
            } else {
                for (p = 0; p < S.g.num_players; p++)
                    if (S.start_active[p] && S.start_dice[p] != best) S.start_active[p] = 0;
                S.start_tie = 1;
                set_msg(C_DIM, "Empate! Os empatados rolam de novo...");
            }
            S.phase = PH_START_SHOW;
            S.timer = 0;
        }
        break;
    case PH_START_SHOW:
        if (S.timer >= 100 || (S.timer > 20 && (pressed & BTN_CROSS))) {
            if (S.start_tie) {
                S.phase = PH_START_ROLL;
                S.timer = 0;
            } else {
                begin_turn();
            }
        }
        break;
    case PH_WAIT_ROLL:
        if (S.g.is_cpu[S.g.current] ? S.timer >= 40 : (pressed & BTN_CROSS) != 0) {
            S.phase = PH_ROLLING;
            S.timer = 0;
            audio_sfx(SFX_DICE);
        }
        break;
    case PH_ROLLING:
        if (S.timer % 4 == 1)
            for (p = 0; p < 3; p++) S.dice_show[p] = 1 + urand(6);
        if (S.timer >= 36) {
            game_roll(&S.g);
            audio_sfx(SFX_DICE_LAND);
            S.n_moves = game_moves(&S.g, S.moves);
            S.sel = 0;
            S.cpu_target = S.g.is_cpu[S.g.current] ? ai_choose(&S.g, S.moves, S.n_moves) : 0;
            S.phase = PH_CHOOSE;
            S.timer = 0;
            set_msg(C_DIM, "Escolha onde colocar as boias (estrela = soma 7, bônus).");
        }
        break;
    case PH_CHOOSE:
        if (S.g.is_cpu[S.g.current]) {
            if (S.timer % 14 == 0 && S.sel != S.cpu_target) {
                S.sel += S.sel < S.cpu_target ? 1 : -1;
                audio_sfx(SFX_MOVE);
            }
            if (S.timer >= 30 && S.sel == S.cpu_target && S.timer % 14 == 13)
                pressed |= BTN_CROSS;
            else
                pressed = 0;
        }
        if (pressed & (BTN_LEFT | BTN_RIGHT)) audio_sfx(SFX_MOVE);
        if (pressed & BTN_LEFT) S.sel = (S.sel + S.n_moves - 1) % S.n_moves;
        if (pressed & BTN_RIGHT) S.sel = (S.sel + 1) % S.n_moves;
        if (pressed & BTN_CROSS) {
            audio_sfx(SFX_CONFIRM);
            S.cur = S.moves[S.sel];
            S.place_idx = 0;
            start_place();
        }
        break;
    case PH_PLACING:
        if (!S.placed && S.timer >= 24) {
            apply_place();
            S.timer = 0;
        } else if (S.placed && S.n_fly == 0 && S.timer >= 22) {
            S.place_idx++;
            start_place();
        }
        break;
    case PH_GAMEOVER:
        if (S.timer > 30 && (pressed & (BTN_CROSS | BTN_START)))
            S.screen = SCR_TITLE;
        break;
    }
}

/* ------------------------------------------------------------------ */
/* Telas                                                               */
/* ------------------------------------------------------------------ */

static const char *RULE_TITLES[] = {
    "OBJETIVO", "OS DADOS", "COLOCANDO BOIAS", "PENALIDADE", "BONUS DO 7", "CONTROLES"
};
static const char *RULE_TEXT[] = {
    "Cada jogador começa com 12 boias da sua cor na reserva.\n"
    "Seja o primeiro a esvaziar a reserva colocando as boias nas raias da piscina.\n"
    "As raias vão de 1 a 12. Na raia 7 fica o salva-vidas.\n"
    "Para começar, cada um rola um dado: quem tirar mais joga primeiro.",

    "Na sua vez, role os 3 dados. Cada dado coloca 1 boia na raia do seu número, "
    "ou você pode somar 2 ou 3 dados para alcançar raias maiores (colocando menos boias).\n"
    "Ex.: 6, 4 e 2 permitem: raias 2, 4 e 6 / raias 10 e 2 / raias 8 e 4 / "
    "duas boias na 6 / uma boia na 12.",

    "A boia entra sempre na casa de baixo e empurra as outras para cima.\n"
    "Cada raia tem no máximo 3 boias. Se a raia estiver cheia, a boia de cima é "
    "expulsa e volta para a reserva do dono (da mesma cor).",

    "Se você expulsar uma boia da SUA cor, recebe uma penalidade: recolhe TODAS as "
    "boias daquela raia, de qualquer cor.\n"
    "Boias de outras cores na sua reserva devem ser colocadas primeiro. Elas continuam "
    "sendo dos adversários se forem expulsas depois.",

    "Somou 7 com 2 ou 3 dados? A boia vai para a raia 7, com o salva-vidas (sem limite, "
    "e de lá nunca volta) e você joga de novo!\n"
    "No lance bônus o 7 ainda vale, mas não há um terceiro lance.\n"
    "Usar o 7 é opcional. Vence quem ficar sem boias primeiro!",

    "Direcional: escolher opção\n"
    "X: confirmar / rolar os dados\n"
    "O: voltar\n"
    "START: pausar o jogo\n"
    "SELECT: liga/desliga a música\n"
    "Jogue com amigos passando o controle, ou contra a CPU."
};
#define N_RULES 6

static void draw_title(void)
{
    static const int items[] = { SPR_BTN_JOGAR, SPR_BTN_COMOJOGAR, SPR_BTN_CREDITOS };
    static const int icons[] = { SPR_ICON_X };
    static const char *texts[] = { "Confirmar" };
    float t = S.frame * 0.05f;
    int i;
    draw_scene(300);

    /* personagens boiando */
    for (i = 0; i < 4; i++) {
        static const float xs[4] = { 92, 238, 402, 548 };
        float y = 412 + sinf(t + i * 1.3f) * 5.0f;
        r_spr_b(P_CHAR[i], xs[i], y, 0.82f, C_WHITE);
    }

    r_spr_c(SPR_LOGO, 320, 88 + sinf(t * 0.8f) * 4.0f, 1.0f, C_WHITE);

    for (i = 0; i < 3; i++)
        draw_button(items[i], 320, 190 + i * 44.0f, i == S.menu_sel);
    draw_hints(422, 1, icons, texts);
}

static void draw_setup(void)
{
    static const int icons[] = { SPR_ICON_X, SPR_ICON_O, SPR_ICON_TRI };
    static const char *texts[] = { "Escolher", "Voltar", "Jogar" };
    char b[48];
    int i;
    draw_scene(330);
    r_ytext_c(320, 12, 40, C_WHITE, "JOGADORES");

    /* numero de jogadores */
    {
        int sel = S.setup_row == 0;
        float pz = sel ? 0.1f * pulse(0.2f) : 0;
        r_panel(200, 60, 240, 40, sel ? RGBA(255, 236, 160, 240) : RGBA(255, 246, 224, 200));
        r_spr_c(SPR_ARROW_L, 222, 80, 0.9f + pz, sel ? C_WHITE : RGBA(255, 255, 255, 120));
        r_spr_c(SPR_ARROW_R, 418, 80, 0.9f + pz, sel ? C_WHITE : RGBA(255, 255, 255, 120));
        snprintf(b, sizeof(b), "%d JOGADORES", S.setup_players);
        r_ytext_c(320, 66, 28, C_WHITE, b);
    }

    for (i = 0; i < MAX_PLAYERS; i++) {
        float x = 26 + i * 150.0f, y = 112;
        int on = i < S.setup_players;
        int sel = S.setup_row == 1 && S.setup_card == i;
        rgba_t tint = on ? C_WHITE : RGBA(255, 255, 255, 90);
        float bob = sel ? sinf(S.frame * 0.15f) * 3 : 0;
        if (sel) r_panel(x - 4, y - 4, 146, 206, WITH_ALPHA(RGB(0xFF8F00), (int)(170 + 80 * pulse(0.2f))));
        r_panel(x, y, 138, 198, on ? RGBA(255, 246, 224, 235) : RGBA(255, 246, 224, 110));
        r_spr_b(P_CHAR[i], x + 69, y + 120 + bob, 0.86f, tint);
        r_text_c(x + 69, y + 124, 0.75f, on ? C_INK : RGBA(13, 59, 102, 110), PNAME[i]);
        if (on) {
            const char *mode = S.setup_cpu[i] ? "CPU" : "HUMANO";
            r_panel(x + 12, y + 156, 114, 32, S.setup_cpu[i] ? RGB(0x5C6BC0) : RGB(0x43A047));
            r_text_c(x + 69, y + 158, 0.72f, C_WHITE, mode);
        } else {
            r_text_c(x + 69, y + 160, 0.65f, RGBA(13, 59, 102, 120), "fora");
        }
    }

    draw_button(SPR_BTN_JOGAR, 320, 344, S.setup_row == 2);
    draw_hints(408, 3, icons, texts);
}

static void draw_rules(void)
{
    static const int icons[] = { SPR_ARROW_L, SPR_ARROW_R, SPR_ICON_O };
    static const char *texts[] = { "", "Página", "Voltar" };
    char b[32];
    int i;
    draw_scene(360);
    r_ytext_c(320, 10, 40, C_WHITE, "COMO JOGAR");
    r_panel(30, 62, 580, 300, RGBA(255, 246, 224, 245));
    r_ytext(52, 74, 28, C_WHITE, RULE_TITLES[S.rules_page]);
    wrap_text(52, 116, 440, 0.66f, C_INK, RULE_TEXT[S.rules_page], 22);
    r_spr_b(P_REACT[S.rules_page % 4][S.rules_page == 3 ? REACT_SAD : REACT_HAPPY],
            552, 350 + sinf(S.frame * 0.1f) * 3, 1.15f, C_WHITE);

    for (i = 0; i < N_RULES; i++)
        r_shape_c(SHAPE_DISC, 320 - (N_RULES - 1) * 9 + i * 18, 376, 10,
                  i == S.rules_page ? C_GOLD : RGBA(255, 255, 255, 120));
    snprintf(b, sizeof(b), "%d/%d", S.rules_page + 1, N_RULES);
    r_text_outline(566, 368, 0.55f, C_WHITE, RGBA(0, 40, 80, 200), b);
    draw_hints(414, 3, icons, texts);
}

static void draw_credits(void)
{
    static const int icons[] = { SPR_ICON_O };
    static const char *texts[] = { "Voltar" };
    draw_scene(330);
    r_ytext_c(320, 12, 40, C_WHITE, "CREDITOS");
    r_panel(90, 66, 460, 250, RGBA(255, 246, 224, 240));
    r_spr_c(SPR_LOGO, 320, 128, 0.6f, C_WHITE);
    r_text_c(320, 196, 0.7f, C_INK, "Arte e ideia: equipe Jogo das Boias");
    r_text_c(320, 224, 0.7f, C_INK, "Programação: C, PS2SDK e gsKit");
    r_text_c(320, 252, 0.7f, C_INK, "Texto: fonte DejaVu Sans");
    r_text_c(320, 280, 0.6f, RGB(0x37474F), "Homebrew para PlayStation 2");
    r_spr_c(SPR_TROPHY, 130, 290, 1.0f, C_WHITE);
    r_spr_c(SPR_STAR, 512, 290, 1.0f, C_WHITE);
    draw_hints(414, 1, icons, texts);
}

static void draw_game(void)
{
    r_rect_grad(0, 0, SCREEN_W, SCREEN_H, RGB(0x2F9BEA), RGB(0x0A3D6B));
    draw_header();
    draw_pool();
    if (S.phase == PH_CHOOSE && S.n_moves > 0)
        draw_move_preview(&S.moves[S.sel]);
    draw_reaction();
    draw_panels();
    draw_bottom_bar();
    draw_flies();

    if (S.banner_timer > 0) {
        float a = S.banner_timer > 20 ? 1.0f : S.banner_timer / 20.0f;
        float s = S.banner_timer > 80 ? 1.0f + (S.banner_timer - 80) * 0.08f : 1.0f;
        r_spr_c(SPR_BURST, 320, 190, 2.1f * s, RGBA(255, 255, 255, (int)(255 * a)));
        r_text_c(320, 160, 1.3f, RGBA(13, 59, 102, (int)(255 * a)), "BÔNUS!");
        r_text_c(320, 200, 0.75f, RGBA(13, 59, 102, (int)(255 * a)), "Jogue de novo!");
    }

    if (S.phase == PH_GAMEOVER) {
        int w = S.g.winner, i, humans = 0;
        float t = S.frame * 0.08f;
        char b[48];
        for (i = 0; i < S.g.num_players; i++) humans += !S.g.is_cpu[i];
        r_rect(0, 0, SCREEN_W, SCREEN_H, RGBA(0, 0, 0, 150));
        r_shape_c(SHAPE_GLOW, 320, 160, 560, RGBA(255, 240, 180, 110));
        r_panel(150, 236, 340, 132, RGBA(8, 40, 80, 225));
        /* derrota quando um humano perde para a CPU */
        r_spr_c(humans > 0 && S.g.is_cpu[w] ? SPR_DEFEAT : SPR_VICTORY, 320,
                138 + sinf(t) * 4, 1.15f, C_WHITE);
        r_spr_b(P_CHAR[w], 118, 352 + sinf(t * 1.3f) * 4, 1.05f, C_WHITE);
        r_spr_c(SPR_TROPHY, 528, 300, 1.4f, C_WHITE);
        snprintf(b, sizeof(b), "%s venceu!", PNAME[w]);
        r_text_c_outline(320, 246, 1.25f, C_GOLD, RGBA(0, 30, 60, 255), b);
        r_text_c_outline(320, 290, 0.65f, C_WHITE, RGBA(0, 30, 60, 255),
                         "Foi o primeiro a esvaziar a reserva!");
        r_spr_c(SPR_ICON_X, 262, 346, 0.9f, C_WHITE);
        r_text_outline(282, 336, 0.65f, C_WHITE, RGBA(0, 30, 60, 220), "Voltar ao menu");
    }

    if (S.paused) {
        static const int items[] = { SPR_BTN_CONTINUAR, SPR_BTN_COMOJOGAR, SPR_BTN_SAIR };
        int i;
        r_rect(0, 0, SCREEN_W, SCREEN_H, RGBA(0, 0, 0, 150));
        r_panel(180, 110, 280, 230, RGBA(255, 246, 224, 240));
        r_ytext_c(320, 122, 36, C_WHITE, "PAUSA");
        for (i = 0; i < 3; i++)
            draw_button(items[i], 320, 196 + i * 48.0f, i == S.pause_sel);
    }
}

/* ------------------------------------------------------------------ */
/* API                                                                 */
/* ------------------------------------------------------------------ */

void ui_init(void)
{
    memset(&S, 0, sizeof(S));
    S.screen = SCR_TITLE;
    S.setup_players = 2;
    S.setup_cpu[1] = 1;
    S.setup_cpu[2] = 1;
    S.setup_cpu[3] = 1;
    S.g.num_players = 2;
    S.msg_col = C_WHITE;
}

static void update_setup(unsigned int pressed)
{
    if (pressed & BTN_UP) S.setup_row = (S.setup_row + 2) % 3;
    if (pressed & BTN_DOWN) S.setup_row = (S.setup_row + 1) % 3;
    if (S.setup_row == 0 && (pressed & (BTN_LEFT | BTN_RIGHT))) {
        S.setup_players += (pressed & BTN_RIGHT) ? 1 : -1;
        if (S.setup_players < 2) S.setup_players = 4;
        if (S.setup_players > 4) S.setup_players = 2;
    }
    if (S.setup_card >= S.setup_players) S.setup_card = S.setup_players - 1;
    if (S.setup_row == 1) {
        if (pressed & BTN_LEFT) S.setup_card = (S.setup_card + S.setup_players - 1) % S.setup_players;
        if (pressed & BTN_RIGHT) S.setup_card = (S.setup_card + 1) % S.setup_players;
        if (pressed & BTN_CROSS) S.setup_cpu[S.setup_card] ^= 1;
    }
    if ((pressed & (BTN_START | BTN_TRIANGLE)) || (S.setup_row == 2 && (pressed & BTN_CROSS)))
        start_game();
    else if (pressed & BTN_CIRCLE)
        S.screen = SCR_TITLE;
}

static void ui_update_screens(unsigned int pressed);

/* Sons de menu, musica de cada tela e liga/desliga da musica (SELECT). */
void ui_update(unsigned int pressed)
{
    int prev_screen = S.screen, prev_paused = S.paused;
    int prev_menu = S.menu_sel, prev_row = S.setup_row, prev_card = S.setup_card;
    int prev_players = S.setup_players, prev_page = S.rules_page, prev_psel = S.pause_sel;
    int prev_cpu = S.setup_cpu[0] | S.setup_cpu[1] << 1 | S.setup_cpu[2] << 2 | S.setup_cpu[3] << 3;
    int cpu;

    if (pressed & BTN_SELECT) {
        audio_set_music_on(!audio_music_on());
        S.toast_timer = 100;
    }
    if (S.toast_timer > 0) S.toast_timer--;

    ui_update_screens(pressed);

    cpu = S.setup_cpu[0] | S.setup_cpu[1] << 1 | S.setup_cpu[2] << 2 | S.setup_cpu[3] << 3;
    if (S.screen != prev_screen || S.paused != prev_paused) {
        if (S.paused != prev_paused && S.screen == SCR_GAME) audio_sfx(SFX_PAUSE);
        else if (pressed & BTN_CIRCLE) audio_sfx(SFX_BACK);
        else audio_sfx(SFX_CONFIRM);
    } else if (S.menu_sel != prev_menu || S.setup_row != prev_row || S.setup_card != prev_card ||
               S.setup_players != prev_players || S.rules_page != prev_page ||
               S.pause_sel != prev_psel) {
        audio_sfx(SFX_MOVE);
    } else if (cpu != prev_cpu) {
        audio_sfx(SFX_POP);
    }

    if (S.screen == SCR_GAME)
        audio_music(S.phase == PH_GAMEOVER ? SONG_WIN : SONG_GAME);
    else
        audio_music(SONG_TITLE);
}

static void ui_update_screens(unsigned int pressed)
{
    S.frame++;
    ui_rand_state ^= S.frame * 0x9E3779B9u;
    if (!ui_rand_state) ui_rand_state = 1;

    switch (S.screen) {
    case SCR_TITLE:
        if (pressed & BTN_UP) S.menu_sel = (S.menu_sel + 2) % 3;
        if (pressed & BTN_DOWN) S.menu_sel = (S.menu_sel + 1) % 3;
        if (pressed & (BTN_CROSS | BTN_START)) {
            if (S.menu_sel == 0) {
                S.screen = SCR_SETUP;
                S.setup_row = 1;
                S.setup_card = 0;
            } else if (S.menu_sel == 1) {
                S.screen = SCR_RULES;
                S.rules_page = 0;
                S.rules_return = SCR_TITLE;
            } else {
                S.screen = SCR_CREDITS;
            }
        }
        break;
    case SCR_SETUP:
        update_setup(pressed);
        break;
    case SCR_RULES:
        if (pressed & (BTN_RIGHT | BTN_CROSS)) {
            if (S.rules_page < N_RULES - 1) S.rules_page++;
            else if (pressed & BTN_CROSS) S.screen = S.rules_return;
        }
        if ((pressed & BTN_LEFT) && S.rules_page > 0) S.rules_page--;
        if (pressed & (BTN_CIRCLE | BTN_START)) S.screen = S.rules_return;
        break;
    case SCR_CREDITS:
        if (pressed & (BTN_CIRCLE | BTN_CROSS | BTN_START)) S.screen = SCR_TITLE;
        break;
    case SCR_GAME:
        update_game(pressed);
        break;
    }
}

void ui_draw(void)
{
    switch (S.screen) {
    case SCR_TITLE: draw_title(); break;
    case SCR_SETUP: draw_setup(); break;
    case SCR_RULES: draw_rules(); break;
    case SCR_CREDITS: draw_credits(); break;
    case SCR_GAME: draw_game(); break;
    }
    if (S.toast_timer > 0) {
        const char *t = audio_music_on() ? "Música: ligada" : "Música: desligada";
        int a = S.toast_timer > 20 ? 230 : S.toast_timer * 230 / 20;
        float w = r_text_w(t, 0.7f) + 40;
        r_panel(320 - w / 2, 54, w, 34, RGBA(8, 52, 96, a));
        r_text_c(320, 58, 0.7f, RGBA(255, 255, 255, a), t);
    }
}
