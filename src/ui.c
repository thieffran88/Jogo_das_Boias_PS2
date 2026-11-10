/*
 * Jogo das Boias PS2 - telas, animacoes e fluxo de jogo.
 * Usa apenas render.h (desenho) e game.h (regras); nao depende do PS2.
 */
#include "ui.h"
#include "render.h"
#include "atlas.h"
#include "game.h"
#include "ai.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Constantes visuais                                                  */
/* ------------------------------------------------------------------ */

static const rgba_t PCOL[MAX_PLAYERS] = {
    RGB(0xE53935), RGB(0x2EAD4B), RGB(0x9C4DCC), RGB(0xFBC02D)
};
static const char *PNAME[MAX_PLAYERS] = { "Vermelho", "Verde", "Roxo", "Amarelo" };
static const char *PCOR_F[MAX_PLAYERS] = { "vermelha", "verde", "roxa", "amarela" };

#define C_WHITE   RGB(0xFFFFFF)
#define C_TEXT    RGB(0xFFFFFF)
#define C_DIM     RGB(0xB3E5FC)
#define C_GOLD    RGB(0xFFD54F)
#define C_DANGER  RGB(0xFF5252)
#define C_PANEL   RGBA(10, 40, 70, 200)

#define POOL_X 24.0f
#define POOL_Y 50.0f
#define POOL_W 592.0f
#define POOL_H 244.0f
#define LANE_W (POOL_W / NUM_LANES)
#define CHIP   44.0f

#define PANEL_Y 300.0f
#define PANEL_H 50.0f
#define PANEL_W 142.0f

#define BAR_Y 356.0f
#define CARD_X 206.0f
#define CARD_Y 362.0f
#define CARD_W 78.0f
#define CARD_H 54.0f

enum { SCR_TITLE, SCR_SETUP, SCR_RULES, SCR_GAME };
enum {
    PH_START_ROLL, PH_START_SHOW, PH_WAIT_ROLL, PH_ROLLING,
    PH_CHOOSE, PH_PLACING, PH_GAMEOVER
};

typedef struct {
    float x0, y0, x1, y1;
    int color;
    int t, dur, delay;
    float size;
} Fly;

#define MAX_FLY 24

static struct {
    int screen;
    unsigned int frame;

    int menu_sel;

    int setup_players;
    int setup_cpu[MAX_PLAYERS];
    int setup_sel;

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
    char banner[48];
    int lane_flash[NUM_LANES + 1];

    int paused, pause_sel;

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

static void set_msg(rgba_t c, const char *fmt, const char *a, const char *b, int n)
{
    /* formatos simples para nao depender de varargs no backend */
    snprintf(S.msg, sizeof(S.msg), fmt, a ? a : "", b ? b : "", n);
    S.msg_col = c;
}

static float lane_cx(int l) { return POOL_X + (l - 0.5f) * LANE_W; }
static float slot_cy(int s) { return POOL_Y + POOL_H - 34.0f - s * 62.0f; }
#define TOWER_Y (POOL_Y + 168.0f)

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
                    int dur, int delay, float size)
{
    Fly *f;
    if (S.n_fly >= MAX_FLY) return;
    f = &S.fly[S.n_fly++];
    f->x0 = x0; f->y0 = y0; f->x1 = x1; f->y1 = y1;
    f->color = color; f->t = 0; f->dur = dur; f->delay = delay; f->size = size;
}

static int flies_busy(void) { return S.n_fly > 0; }

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
/* Desenho de elementos                                                */
/* ------------------------------------------------------------------ */

static void draw_chip(float cx, float cy, float size, int color, int alpha)
{
    rgba_t c = WITH_ALPHA(PCOL[color], alpha);
    r_shape_c(SHAPE_DISC, cx + size * 0.06f, cy + size * 0.09f, size * 0.98f,
              RGBA(0, 20, 40, alpha * 70 / 255));
    r_shape_c(SHAPE_RING, cx, cy, size, c);
    r_shape_c(SHAPE_STRIPES, cx, cy, size, RGBA(255, 255, 255, alpha * 235 / 255));
    r_shape_c(SHAPE_RING_OUTLINE, cx, cy, size, RGBA(0, 0, 0, alpha * 60 / 255));
}

static void draw_die(float x, float y, float size, int v, rgba_t body)
{
    static const signed char pips[7][6][2] = {
        {{0}}, {{0, 0}},
        {{-1, -1}, {1, 1}},
        {{-1, -1}, {0, 0}, {1, 1}},
        {{-1, -1}, {1, -1}, {-1, 1}, {1, 1}},
        {{-1, -1}, {1, -1}, {0, 0}, {-1, 1}, {1, 1}},
        {{-1, -1}, {1, -1}, {-1, 0}, {1, 0}, {-1, 1}, {1, 1}}
    };
    static const int npips[7] = { 0, 1, 2, 3, 4, 5, 6 };
    float cx = x + size * 0.5f, cy = y + size * 0.5f, off = size * 0.26f;
    int i;
    r_shape(SHAPE_ROUNDRECT, x + 3, y + 4, size, size, RGBA(0, 0, 0, 80));
    r_shape(SHAPE_ROUNDRECT, x, y, size, size, body);
    if (v < 1 || v > 6) return;
    for (i = 0; i < npips[v]; i++)
        r_shape_c(SHAPE_DISC, cx + pips[v][i][0] * off, cy + pips[v][i][1] * off,
                  size * 0.2f, RGB(0x263238));
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
            r_text_shadow(x, y, scale, c, line);
            y += line_h;
            ll = 0; line[0] = 0;
            s++;
            continue;
        }
        while (*s && *s != ' ' && *s != '\n' && wl < 78) word[wl++] = *s++;
        word[wl] = 0;
        if (!wl) continue;
        {
            char test[240];
            snprintf(test, sizeof(test), "%s%s%s", line, ll ? " " : "", word);
            if (ll && r_text_w(test, scale) > maxw) {
                r_text_shadow(x, y, scale, c, line);
                y += line_h;
                snprintf(line, sizeof(line), "%s", word);
            } else {
                snprintf(line, sizeof(line), "%s", test);
            }
            ll = (int)strlen(line);
        }
    }
    if (ll) r_text_shadow(x, y, scale, c, line);
}

static void draw_water_bg(void)
{
    int i;
    float t = S.frame * 0.02f;
    r_rect_grad(0, 0, SCREEN_W, SCREEN_H, RGB(0x4FC3F7), RGB(0x01579B));
    for (i = 0; i < 14; i++) {
        float x = fmodf(i * 97.0f + t * 30.0f * (1 + (i % 3)), SCREEN_W + 120) - 120;
        float y = 30.0f + i * 31.0f + sinf(t + i) * 6.0f;
        r_shape(SHAPE_WAVE, x, y, 110, 24, RGBA(255, 255, 255, 30));
    }
}

static void draw_pool(void)
{
    int l, s, i;
    float t = S.frame * 0.03f;

    /* borda */
    r_panel(POOL_X - 10, POOL_Y - 8, POOL_W + 20, POOL_H + 16, RGB(0xECEFF1));
    r_rect_grad(POOL_X, POOL_Y, POOL_W, POOL_H, RGB(0x81D4FA), RGB(0x0277BD));

    /* reflexos na agua */
    for (i = 0; i < 10; i++) {
        float x = POOL_X + fmodf(i * 71.0f + t * 18.0f, POOL_W - 60);
        float y = POOL_Y + 30 + (i * 53) % 190 + sinf(t * 2 + i) * 4;
        r_shape(SHAPE_WAVE, x, y, 60, 14, RGBA(255, 255, 255, 40));
    }

    for (l = 1; l <= NUM_LANES; l++) {
        float x0 = POOL_X + (l - 1) * LANE_W;
        if (l == SAFE_LANE) continue;
        /* faixa escura no fundo da raia */
        r_rect(lane_cx(l) - 3, POOL_Y + 34, 6, POOL_H - 44, RGBA(1, 50, 90, 70));
        if (S.lane_flash[l] > 0 && (S.lane_flash[l] / 4) % 2 == 0)
            r_rect(x0, POOL_Y, LANE_W, POOL_H, RGBA(255, 60, 60, 110));
    }

    /* Torre do Salva-Vidas (raia 7) */
    {
        float x0 = POOL_X + (SAFE_LANE - 1) * LANE_W;
        r_rect_grad(x0, POOL_Y, LANE_W, POOL_H, RGB(0xFFF3D6), RGB(0xF2C98A));
        r_shape_c(SHAPE_GLOW, lane_cx(SAFE_LANE), POOL_Y + 80, 70, RGBA(255, 255, 255, 160));
        r_shape_c(SHAPE_TOWER, lane_cx(SAFE_LANE), POOL_Y + 78, 54, RGB(0xC62828));
    }

    /* cordas de raia (boias pequenas alternando vermelho e branco) */
    for (l = 1; l < NUM_LANES; l++) {
        float x = POOL_X + l * LANE_W;
        for (s = 0; s < 22; s++) {
            float y = POOL_Y + 6 + s * 11.0f;
            rgba_t c = ((s / 2) % 2) ? RGB(0xFFFFFF) : RGB(0xD32F2F);
            r_shape_c(SHAPE_DISC, x, y, 8, c);
        }
    }

    /* numeros das raias */
    for (l = 1; l <= NUM_LANES; l++) {
        char b[4];
        snprintf(b, sizeof(b), "%d", l);
        r_text_c(lane_cx(l) + 1, POOL_Y + 3, 0.8f, RGBA(0, 0, 0, 90), b);
        r_text_c(lane_cx(l), POOL_Y + 2, 0.8f,
                 l == SAFE_LANE ? RGB(0xC62828) : RGB(0x01335B), b);
    }

    /* boias nas raias */
    for (l = 1; l <= NUM_LANES; l++) {
        if (l == SAFE_LANE) continue;
        for (s = 0; s < S.g.lane_n[l]; s++) {
            float bob = sinf(t * 2.0f + l * 0.9f + s) * 2.0f;
            draw_chip(lane_cx(l), slot_cy(s) + bob, CHIP, S.g.lane[l][s], 255);
        }
    }

    /* contagem na torre */
    {
        float y = POOL_Y + 128;
        int c;
        for (c = 0; c < S.g.num_players; c++) {
            char b[8];
            if (!S.g.safe[c]) continue;
            draw_chip(lane_cx(SAFE_LANE) - 10, y, 24, c, 255);
            snprintf(b, sizeof(b), "%d", S.g.safe[c]);
            r_text(lane_cx(SAFE_LANE) + 4, y - 9, 0.65f, RGB(0x4E342E), b);
            y += 27;
        }
    }
}

/* Marca a raia escolhida e as boias que seriam expulsas. */
static void draw_move_preview(const Move *m)
{
    Game t = S.g;
    int k[NUM_LANES + 1];
    int i;
    float pulse = 0.5f + 0.5f * sinf(S.frame * 0.15f);
    memset(k, 0, sizeof(k));
    for (i = 0; i < m->n; i++) {
        int l = m->lanes[i];
        int n0 = S.g.lane_n[l];
        PlaceResult r;
        float x0 = POOL_X + (l - 1) * LANE_W;
        r_rect(x0 + 2, POOL_Y, LANE_W - 4, POOL_H,
               RGBA(255, 255, 160, (int)(40 + 50 * pulse)));
        k[l]++;
        r = game_place(&t, l);
        if (l == SAFE_LANE) {
            r_shape_c(SHAPE_STAR, lane_cx(l), TOWER_Y + 40, 26 + 6 * pulse, C_GOLD);
            continue;
        }
        r_shape_c(SHAPE_DISC, lane_cx(l), POOL_Y + POOL_H - 6, 10, C_GOLD);
        if (r.kicked != NO_CHIP) {
            int s = LANE_CAP - k[l];
            if (s >= 0 && s < n0) {
                rgba_t c = r.penalty ? C_DANGER : RGB(0xFFEB3B);
                r_shape_c(SHAPE_RING_OUTLINE, lane_cx(l), slot_cy(s), CHIP + 8 + 4 * pulse, c);
            }
            if (r.penalty)
                r_text_c_shadow(lane_cx(l), POOL_Y + 26, 0.5f, C_DANGER, "!!");
        }
    }
}

static void draw_panels(void)
{
    int p, c;
    for (p = 0; p < S.g.num_players; p++) {
        float x = panel_x(p);
        int active = (p == S.g.current && S.phase != PH_START_ROLL && S.phase != PH_START_SHOW);
        int total = game_stock_total(&S.g, p);
        char b[32];
        float fx;
        if (active) {
            float pulse = 0.5f + 0.5f * sinf(S.frame * 0.12f);
            r_panel(x - 3, PANEL_Y - 3, PANEL_W + 6, PANEL_H + 6,
                    WITH_ALPHA(C_GOLD, (int)(150 + 100 * pulse)));
        }
        r_panel(x, PANEL_Y, PANEL_W, PANEL_H, RGBA(10, 40, 70, 215));
        draw_chip(x + 26, PANEL_Y + 25, 38, p, 255);
        r_text(x + 50, PANEL_Y + 3, 0.62f, C_TEXT, PNAME[p]);
        if (S.g.is_cpu[p])
            r_text(x + 50 + r_text_w(PNAME[p], 0.62f) + 4, PANEL_Y + 5, 0.5f, C_DIM, "CPU");
        snprintf(b, sizeof(b), "%d", total);
        r_text(x + 50, PANEL_Y + 21, 0.95f, total <= 3 ? C_GOLD : C_TEXT, b);
        /* boias de outras cores na reserva */
        fx = x + 50 + r_text_w(b, 0.95f) + 8;
        for (c = 0; c < MAX_PLAYERS; c++) {
            if (c == p || !S.g.stock[p][c]) continue;
            draw_chip(fx + 8, PANEL_Y + 35, 16, c, 255);
            snprintf(b, sizeof(b), "%d", S.g.stock[p][c]);
            r_text(fx + 17, PANEL_Y + 26, 0.5f, C_TEXT, b);
            fx += 30;
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
    r_panel(16, BAR_Y, 608, 84, C_PANEL);

    for (i = 0; i < 3; i++) {
        int v = 0;
        float jx = 0, jy = 0;
        if (S.phase == PH_ROLLING) {
            v = S.dice_show[i];
            jx = (float)(urand(7) - 3);
            jy = (float)(urand(7) - 3);
        } else if (show_dice) {
            v = S.g.dice[i];
        }
        if (v) draw_die(28 + i * 58 + jx, BAR_Y + 10 + jy, 48, v, C_WHITE);
        else draw_die(28 + i * 58, BAR_Y + 10, 48, 0, RGBA(255, 255, 255, 50));
    }

    if (S.phase == PH_CHOOSE) {
        for (i = 0; i < S.n_moves; i++) {
            char top[24], bot[32];
            float x = CARD_X + i * (CARD_W + 4);
            int selc = (i == S.sel);
            rgba_t bg = selc ? RGB(0xFFF59D) : RGBA(255, 255, 255, 40);
            rgba_t fg = selc ? RGB(0x01335B) : C_TEXT;
            if (selc) {
                float pulse = 0.5f + 0.5f * sinf(S.frame * 0.2f);
                r_panel(x - 3, CARD_Y - 3 - 3 * pulse, CARD_W + 6, CARD_H + 6, C_GOLD);
            }
            r_panel(x, CARD_Y - (selc ? 3 : 0), CARD_W, CARD_H, bg);
            move_label(&S.moves[i], top, bot, sizeof(top));
            r_text_c(x + CARD_W / 2, CARD_Y + 4 - (selc ? 3 : 0),
                     r_text_w(top, 0.9f) > CARD_W - 8 ? 0.7f : 0.9f, fg, top);
            r_text_c(x + CARD_W / 2, CARD_Y + 32 - (selc ? 3 : 0),
                     r_text_w(bot, 0.5f) > CARD_W - 6 ? 0.42f : 0.5f, fg, bot);
            if (S.moves[i].uses_seven)
                r_shape_c(SHAPE_STAR, x + CARD_W - 8, CARD_Y + 4 - (selc ? 3 : 0), 20, RGB(0xFF8F00));
        }
    } else if (S.phase == PH_WAIT_ROLL) {
        const char *who = PNAME[S.g.current];
        char b[64];
        if (S.g.is_cpu[S.g.current]) snprintf(b, sizeof(b), "%s (CPU) vai rolar...", who);
        else snprintf(b, sizeof(b), "Aperte X para rolar os dados");
        r_text_shadow(CARD_X + 6, BAR_Y + 18, 0.8f, C_TEXT, b);
    } else if (S.phase == PH_ROLLING) {
        r_text_shadow(CARD_X + 6, BAR_Y + 18, 0.8f, C_DIM, "Rolando...");
    } else if (S.phase == PH_START_ROLL || S.phase == PH_START_SHOW) {
        int p;
        for (p = 0; p < S.g.num_players; p++) {
            float x = CARD_X + p * 102.0f;
            int v = S.start_dice[p];
            if (S.phase == PH_START_ROLL && S.start_active[p])
                v = 1 + (int)((S.frame / 4 + p * 7) % 6);
            draw_chip(x + 18, BAR_Y + 34, 32, p, S.start_active[p] ? 255 : 110);
            if (v)
                draw_die(x + 40, BAR_Y + 10, 48, v,
                         S.start_active[p] ? C_WHITE : RGBA(255, 255, 255, 110));
        }
    }

    r_text_shadow(28, BAR_Y + 64, 0.6f, S.msg_col, S.msg);
}

static void draw_header(void)
{
    char b[64];
    float x;
    r_text_shadow(20, 12, 0.85f, C_TEXT, "Jogo das Boias");
    if (S.phase == PH_START_ROLL || S.phase == PH_START_SHOW) {
        r_text_shadow(380, 12, 0.7f, C_TEXT, "Quem começa?");
        return;
    }
    snprintf(b, sizeof(b), "Vez do %s", PNAME[S.g.current]);
    x = 620 - r_text_w(b, 0.75f);
    draw_chip(x - 18, 26, 26, S.g.current, 255);
    r_text_shadow(x, 13, 0.75f, C_TEXT, b);
    if (S.g.bonus_roll && S.phase != PH_GAMEOVER) {
        float pulse = 0.5f + 0.5f * sinf(S.frame * 0.25f);
        r_shape_c(SHAPE_STAR, x - 60, 26, 26 + 4 * pulse, C_GOLD);
        r_text_shadow(x - 46 - r_text_w("BÔNUS", 0.6f) - 20, 16, 0.6f, C_GOLD, "BÔNUS");
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
        draw_chip(x, y, f->size * (1.0f + 0.25f * sinf(u * 3.14159f)), f->color, 255);
    }
}

/* ------------------------------------------------------------------ */
/* Fluxo do jogo                                                       */
/* ------------------------------------------------------------------ */

static void begin_turn(void)
{
    S.phase = PH_WAIT_ROLL;
    S.timer = 0;
    if (!S.g.bonus_roll)
        set_msg(C_DIM, "Vez do %s.", PNAME[S.g.current], NULL, 0);
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
    memset(S.lane_flash, 0, sizeof(S.lane_flash));
    for (p = 0; p < MAX_PLAYERS; p++) {
        S.start_active[p] = p < S.g.num_players;
        S.start_dice[p] = 0;
    }
    S.phase = PH_START_ROLL;
    S.timer = 0;
    set_msg(C_DIM, "Cada jogador rola um dado: o maior começa.", NULL, NULL, 0);
}

static void finish_move(void)
{
    if (S.g.winner >= 0) {
        S.phase = PH_GAMEOVER;
        S.timer = 0;
        set_msg(C_GOLD, "%s esvaziou a reserva e venceu!", PNAME[S.g.winner], NULL, 0);
        return;
    }
    if (game_end_move(&S.g, &S.cur)) {
        snprintf(S.banner, sizeof(S.banner), "BÔNUS! Jogue de novo!");
        S.banner_timer = 90;
        set_msg(C_GOLD, "Somou 7! %s ganha mais um lance.", PNAME[S.g.current], NULL, 0);
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
    add_fly(px, py, tx, ty, color, 24, 0, CHIP);
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
        set_msg(C_GOLD, "Boia segura na Torre do Salva-Vidas!", NULL, NULL, 0);
    } else if (r.penalty) {
        float px, py;
        char tmp[24];
        panel_center(me, &px, &py);
        /* a boia expulsa (do topo) e todas as outras voltam para mim */
        add_fly(lane_cx(lane), slot_cy(LANE_CAP - 1) - 20, px, py, r.kicked, 30, 0, CHIP);
        add_fly(lane_cx(lane), slot_cy(0), px, py, r.color, 30, 6, CHIP);
        for (s = 0; s < n0 - 1; s++)
            add_fly(lane_cx(lane), slot_cy(s + 1), px, py, before[s], 30, 10 + s * 5, CHIP);
        S.lane_flash[lane] = 48;
        snprintf(tmp, sizeof(tmp), "%d", lane);
        snprintf(S.msg, sizeof(S.msg), "PENALIDADE! %s expulsou a própria boia e recolhe %d da raia %s.",
                 PNAME[me], r.penalty_n, tmp);
        S.msg_col = C_DANGER;
    } else if (r.kicked != NO_CHIP) {
        float px, py;
        panel_center(r.kicked, &px, &py);
        add_fly(lane_cx(lane), slot_cy(LANE_CAP - 1) - 20, px, py, r.kicked, 30, 0, CHIP);
        snprintf(S.msg, sizeof(S.msg), "Boia %s expulsa da raia %d! Volta para o %s.",
                 PCOR_F[r.kicked], lane, PNAME[r.kicked]);
        S.msg_col = RGB(0xFFEB3B);
    } else {
        snprintf(S.msg, sizeof(S.msg), "Boia colocada na raia %d.", lane);
        S.msg_col = C_TEXT;
    }
}

static void update_game(unsigned int pressed)
{
    int p, l;

    if (S.banner_timer > 0) S.banner_timer--;
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
        if (S.timer >= 45) {
            int best = 0, nbest = 0;
            for (p = 0; p < S.g.num_players; p++)
                if (S.start_active[p]) {
                    S.start_dice[p] = game_roll_die(&S.g);
                    if (S.start_dice[p] > best) best = S.start_dice[p];
                }
            for (p = 0; p < S.g.num_players; p++)
                if (S.start_active[p] && S.start_dice[p] == best) nbest++;
            if (nbest == 1) {
                for (p = 0; p < S.g.num_players; p++)
                    if (S.start_active[p] && S.start_dice[p] == best) S.g.current = p;
                S.start_tie = 0;
                set_msg(C_GOLD, "%s tirou o maior número e começa!", PNAME[S.g.current], NULL, 0);
            } else {
                for (p = 0; p < S.g.num_players; p++)
                    if (S.start_active[p] && S.start_dice[p] != best) S.start_active[p] = 0;
                S.start_tie = 1;
                set_msg(C_DIM, "Empate! Os empatados rolam de novo...", NULL, NULL, 0);
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
        }
        break;
    case PH_ROLLING:
        if (S.timer % 4 == 1)
            for (p = 0; p < 3; p++) S.dice_show[p] = 1 + urand(6);
        if (S.timer >= 36) {
            game_roll(&S.g);
            S.n_moves = game_moves(&S.g, S.moves);
            S.sel = 0;
            S.cpu_target = S.g.is_cpu[S.g.current] ? ai_choose(&S.g, S.moves, S.n_moves) : 0;
            S.phase = PH_CHOOSE;
            S.timer = 0;
            set_msg(C_DIM, "Escolha onde colocar as boias (estrela = soma 7, bônus).", NULL, NULL, 0);
        }
        break;
    case PH_CHOOSE:
        if (S.g.is_cpu[S.g.current]) {
            if (S.timer % 14 == 0 && S.sel != S.cpu_target)
                S.sel += S.sel < S.cpu_target ? 1 : -1;
            if (S.timer >= 30 && S.sel == S.cpu_target && S.timer % 14 == 13)
                pressed |= BTN_CROSS;
            else
                pressed = 0;
        }
        if (pressed & BTN_LEFT) S.sel = (S.sel + S.n_moves - 1) % S.n_moves;
        if (pressed & BTN_RIGHT) S.sel = (S.sel + 1) % S.n_moves;
        if (pressed & BTN_CROSS) {
            S.cur = S.moves[S.sel];
            S.place_idx = 0;
            start_place();
        }
        break;
    case PH_PLACING:
        if (!S.placed && S.timer >= 24) {
            apply_place();
            S.timer = 0;
        } else if (S.placed && !flies_busy() && S.timer >= 22) {
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
    "Objetivo", "Os dados", "Colocando boias", "Penalidade", "Bônus do 7", "Controles"
};
static const char *RULE_TEXT[] = {
    "Cada jogador começa com 12 boias da sua cor na reserva.\n"
    "Seja o primeiro a esvaziar a reserva colocando as boias nas raias da piscina.\n"
    "As raias vão de 1 a 12. A raia 7 é a Torre do Salva-Vidas.\n"
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

    "Somou 7 com 2 ou 3 dados? A boia vai para a Torre do Salva-Vidas (sem limite, "
    "e de lá nunca volta) e você joga de novo!\n"
    "No lance bônus o 7 ainda leva à Torre, mas não há um terceiro lance.\n"
    "Usar o 7 é opcional. Vence quem ficar sem boias primeiro!",

    "Direcional: escolher opção\n"
    "X: confirmar / rolar os dados\n"
    "O: voltar\n"
    "START: pausar o jogo\n"
    "Jogue com amigos passando o controle, ou contra a CPU."
};
#define N_RULES 6

static void draw_title(void)
{
    static const char *items[] = { "Novo Jogo", "Como Jogar" };
    float t = S.frame * 0.05f;
    int i;
    draw_water_bg();

    for (i = 0; i < 8; i++) {
        float x = 50 + i * 78.0f;
        float y = 330 + sinf(t + i * 0.8f) * 10.0f;
        draw_chip(x, y, 56, i % 4, 255);
    }

    r_shape_c(SHAPE_GLOW, 320, 120, 420, RGBA(255, 255, 255, 70));
    r_text_c(320 + 4, 74 + 5, 2.4f, RGBA(0, 30, 60, 120), "Jogo das Boias");
    r_text_c(320, 74, 2.4f, C_WHITE, "Jogo das Boias");
    r_panel(270, 150, 100, 40, RGB(0xD32F2F));
    r_text_c(320, 155, 1.0f, C_WHITE, "PS2");

    for (i = 0; i < 2; i++) {
        int sel = (i == S.menu_sel);
        float y = 214 + i * 46.0f;
        if (sel) {
            r_panel(200, y - 4, 240, 40, RGBA(255, 245, 157, 235));
            r_shape_c(SHAPE_RING, 218, y + 16, 24, PCOL[(S.frame / 30) % 4]);
        } else {
            r_panel(200, y - 4, 240, 40, RGBA(0, 40, 80, 120));
        }
        r_text_c(320, y + 2, 0.95f, sel ? RGB(0x01335B) : C_WHITE, items[i]);
    }
    if ((S.frame / 30) % 2)
        r_text_c_shadow(320, 402, 0.6f, C_WHITE, "Pressione X");
}

static void draw_setup(void)
{
    int i;
    char b[48];
    draw_water_bg();
    r_text_c_shadow(320, 26, 1.3f, C_WHITE, "Novo Jogo");
    r_panel(110, 90, 420, 290, C_PANEL);

    {
        int sel = (S.setup_sel == 0);
        if (sel) r_panel(124, 104, 392, 42, RGBA(255, 245, 157, 230));
        snprintf(b, sizeof(b), "Jogadores:  <  %d  >", S.setup_players);
        r_text_c(320, 111, 0.85f, sel ? RGB(0x01335B) : C_WHITE, b);
    }
    for (i = 0; i < MAX_PLAYERS; i++) {
        float y = 160 + i * 48.0f;
        int sel = (S.setup_sel == i + 1);
        int on = i < S.setup_players;
        rgba_t fg = sel ? RGB(0x01335B) : (on ? C_WHITE : RGBA(255, 255, 255, 70));
        if (sel) r_panel(124, y - 6, 392, 42, RGBA(255, 245, 157, 230));
        draw_chip(156, y + 15, 34, i, on ? 255 : 80);
        r_text(182, y + 1, 0.8f, fg, PNAME[i]);
        if (on) {
            snprintf(b, sizeof(b), "<  %s  >", S.setup_cpu[i] ? "CPU" : "Humano");
            r_text(500 - r_text_w(b, 0.8f), y + 1, 0.8f, fg, b);
        } else {
            r_text(500 - r_text_w("fora", 0.7f), y + 3, 0.7f, fg, "fora");
        }
    }
    r_text_c_shadow(320, 396, 0.6f, C_WHITE, "X / START: começar      O: voltar");
}

static void draw_rules(void)
{
    char b[32];
    int i;
    draw_water_bg();
    r_text_c_shadow(320, 18, 1.2f, C_WHITE, "Como Jogar");
    r_panel(40, 70, 560, 316, C_PANEL);
    r_text_shadow(64, 84, 1.0f, C_GOLD, RULE_TITLES[S.rules_page]);
    wrap_text(64, 130, 512, 0.68f, C_WHITE, RULE_TEXT[S.rules_page], 23);

    for (i = 0; i < N_RULES; i++)
        r_shape_c(SHAPE_DISC, 320 - (N_RULES - 1) * 9 + i * 18, 400, 10,
                  i == S.rules_page ? C_GOLD : RGBA(255, 255, 255, 90));
    snprintf(b, sizeof(b), "%d/%d", S.rules_page + 1, N_RULES);
    r_text_shadow(560, 392, 0.55f, C_WHITE, b);
    r_text_c_shadow(320, 414, 0.55f, C_WHITE, "<  >  páginas      O: voltar");
}

static void draw_game(void)
{
    r_rect_grad(0, 0, SCREEN_W, SCREEN_H, RGB(0x0D47A1), RGB(0x002B4F));
    draw_header();
    draw_pool();
    if (S.phase == PH_CHOOSE && S.n_moves > 0)
        draw_move_preview(&S.moves[S.sel]);
    draw_panels();
    draw_bottom_bar();
    draw_flies();

    if (S.banner_timer > 0) {
        float a = S.banner_timer > 20 ? 1.0f : S.banner_timer / 20.0f;
        r_panel(150, 150, 340, 70, RGBA(255, 193, 7, (int)(235 * a)));
        r_shape_c(SHAPE_STAR, 180, 185, 40, RGBA(255, 255, 255, (int)(255 * a)));
        r_shape_c(SHAPE_STAR, 460, 185, 40, RGBA(255, 255, 255, (int)(255 * a)));
        r_text_c(320, 168, 0.95f, RGBA(1, 51, 91, (int)(255 * a)), S.banner);
    }

    if (S.phase == PH_GAMEOVER) {
        int w = S.g.winner, i;
        float t = S.frame * 0.08f;
        char b[48];
        r_rect(0, 0, SCREEN_W, SCREEN_H, RGBA(0, 0, 0, 120));
        r_panel(120, 110, 400, 200, RGBA(255, 255, 255, 240));
        for (i = 0; i < 6; i++)
            r_shape_c(SHAPE_STAR, 150 + i * 68, 128 + sinf(t + i) * 6, 26, PCOL[w]);
        draw_chip(320, 200, 70 + 6 * sinf(t), w, 255);
        snprintf(b, sizeof(b), "%s venceu!", PNAME[w]);
        r_text_c(320, 246, 1.2f, RGB(0x01335B), b);
        r_text_c(320, 284, 0.55f, RGB(0x37474F), "X: voltar ao menu");
    }

    if (S.paused) {
        static const char *items[] = { "Continuar", "Como Jogar", "Sair para o menu" };
        int i;
        r_rect(0, 0, SCREEN_W, SCREEN_H, RGBA(0, 0, 0, 140));
        r_panel(190, 120, 260, 210, C_PANEL);
        r_text_c_shadow(320, 132, 1.0f, C_WHITE, "Pausa");
        for (i = 0; i < 3; i++) {
            int sel = i == S.pause_sel;
            float y = 184 + i * 44.0f;
            if (sel) r_panel(206, y - 4, 228, 38, RGBA(255, 245, 157, 235));
            r_text_c(320, y + 1, 0.8f, sel ? RGB(0x01335B) : C_WHITE, items[i]);
        }
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
    S.msg_col = C_TEXT;
}

void ui_update(unsigned int pressed)
{
    S.frame++;
    ui_rand_state ^= S.frame * 0x9E3779B9u;
    if (!ui_rand_state) ui_rand_state = 1;

    switch (S.screen) {
    case SCR_TITLE:
        if (pressed & (BTN_UP | BTN_DOWN)) S.menu_sel ^= 1;
        if (pressed & (BTN_CROSS | BTN_START)) {
            if (S.menu_sel == 0) {
                S.screen = SCR_SETUP;
                S.setup_sel = 0;
            } else {
                S.screen = SCR_RULES;
                S.rules_page = 0;
                S.rules_return = SCR_TITLE;
            }
        }
        break;
    case SCR_SETUP:
        if (pressed & BTN_UP)
            S.setup_sel = (S.setup_sel + S.setup_players) % (S.setup_players + 1);
        if (pressed & BTN_DOWN)
            S.setup_sel = (S.setup_sel + 1) % (S.setup_players + 1);
        if (pressed & (BTN_LEFT | BTN_RIGHT)) {
            if (S.setup_sel == 0) {
                S.setup_players += (pressed & BTN_RIGHT) ? 1 : -1;
                if (S.setup_players < 2) S.setup_players = 4;
                if (S.setup_players > 4) S.setup_players = 2;
            } else {
                S.setup_cpu[S.setup_sel - 1] ^= 1;
            }
        }
        if ((pressed & BTN_CROSS) && S.setup_sel > 0 && !(pressed & BTN_START))
            S.setup_cpu[S.setup_sel - 1] ^= 1;
        if ((pressed & BTN_START) || ((pressed & BTN_CROSS) && S.setup_sel == 0))
            start_game();
        else if (pressed & BTN_CIRCLE)
            S.screen = SCR_TITLE;
        break;
    case SCR_RULES:
        if (pressed & (BTN_RIGHT | BTN_CROSS)) {
            if (S.rules_page < N_RULES - 1) S.rules_page++;
            else if (pressed & BTN_CROSS) S.screen = S.rules_return;
        }
        if ((pressed & BTN_LEFT) && S.rules_page > 0) S.rules_page--;
        if (pressed & (BTN_CIRCLE | BTN_START)) S.screen = S.rules_return;
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
    case SCR_GAME: draw_game(); break;
    }
}
