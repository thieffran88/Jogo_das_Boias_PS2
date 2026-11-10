#include "game.h"
#include <string.h>

void game_init(Game *g, int num_players, const int *is_cpu, unsigned int seed)
{
    int p, l, s;
    memset(g, 0, sizeof(*g));
    if (num_players < 2) num_players = 2;
    if (num_players > MAX_PLAYERS) num_players = MAX_PLAYERS;
    g->num_players = num_players;
    for (p = 0; p < num_players; p++) {
        g->is_cpu[p] = is_cpu ? is_cpu[p] : 0;
        g->stock[p][p] = CHIPS_PER_PLAYER;
    }
    for (l = 0; l <= NUM_LANES; l++)
        for (s = 0; s < LANE_CAP; s++)
            g->lane[l][s] = NO_CHIP;
    g->winner = -1;
    g->rng = seed ? seed : 0x1234567u;
}

unsigned int game_rand(Game *g)
{
    /* xorshift32 */
    unsigned int x = g->rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    g->rng = x;
    return x;
}

int game_roll_die(Game *g)
{
    return (int)(game_rand(g) % 6u) + 1;
}

void game_roll(Game *g)
{
    int i;
    for (i = 0; i < 3; i++)
        g->dice[i] = game_roll_die(g);
}

void game_set_dice(Game *g, int a, int b, int c)
{
    g->dice[0] = a;
    g->dice[1] = b;
    g->dice[2] = c;
}

int game_stock_total(const Game *g, int player)
{
    int c, t = 0;
    for (c = 0; c < MAX_PLAYERS; c++)
        t += g->stock[player][c];
    return t;
}

int game_chips_on_board(const Game *g, int color)
{
    int l, s, t = g->safe[color];
    for (l = 1; l <= NUM_LANES; l++)
        for (s = 0; s < g->lane_n[l]; s++)
            if (g->lane[l][s] == color) t++;
    return t;
}

/* Boias de outras cores saem primeiro (a de maior quantidade). */
int game_next_color(const Game *g, int player)
{
    int c, best = NO_CHIP, best_n = 0;
    for (c = 0; c < MAX_PLAYERS; c++) {
        if (c == player) continue;
        if (g->stock[player][c] > best_n) {
            best_n = g->stock[player][c];
            best = c;
        }
    }
    if (best != NO_CHIP) return best;
    return g->stock[player][player] > 0 ? player : NO_CHIP;
}

static int sum_mask(const Game *g, int mask)
{
    int i, s = 0;
    for (i = 0; i < 3; i++)
        if (mask & (1 << i)) s += g->dice[i];
    return s;
}

static int popcount3(int m)
{
    return (m & 1) + ((m >> 1) & 1) + ((m >> 2) & 1);
}

int game_moves(const Game *g, Move out[MAX_MOVES])
{
    /* Todas as particoes de 3 dados. */
    static const int parts[5][3] = {
        {1, 2, 4}, {3, 4, 0}, {5, 2, 0}, {6, 1, 0}, {7, 0, 0}
    };
    int count = 0, p, k, j;
    for (p = 0; p < 5; p++) {
        Move m;
        int ok = 1;
        memset(&m, 0, sizeof(m));
        for (k = 0; k < 3 && parts[p][k]; k++) {
            int s = sum_mask(g, parts[p][k]);
            if (s < 1 || s > NUM_LANES) { ok = 0; break; }
            m.lanes[m.n] = s;
            m.groups[m.n] = parts[p][k];
            if (s == SAFE_LANE && popcount3(parts[p][k]) >= 2) m.uses_seven = 1;
            m.n++;
        }
        if (!ok) continue;
        /* ordena raias (insercao) mantendo os grupos juntos */
        for (k = 1; k < m.n; k++) {
            int lv = m.lanes[k], gv = m.groups[k];
            j = k - 1;
            while (j >= 0 && m.lanes[j] > lv) {
                m.lanes[j + 1] = m.lanes[j];
                m.groups[j + 1] = m.groups[j];
                j--;
            }
            m.lanes[j + 1] = lv;
            m.groups[j + 1] = gv;
        }
        /* remove duplicatas (mesmo conjunto de raias) */
        for (j = 0; j < count; j++) {
            if (out[j].n == m.n && out[j].uses_seven == m.uses_seven) {
                int same = 1;
                for (k = 0; k < m.n; k++)
                    if (out[j].lanes[k] != m.lanes[k]) same = 0;
                if (same) break;
            }
        }
        if (j < count) continue;
        out[count++] = m;
    }
    return count;
}

PlaceResult game_place(Game *g, int lane)
{
    PlaceResult r;
    int p = g->current;
    int color = game_next_color(g, p);
    int s;

    memset(&r, 0, sizeof(r));
    r.lane = lane;
    r.color = color;
    r.kicked = NO_CHIP;
    if (color == NO_CHIP || g->winner >= 0 || lane < 1 || lane > NUM_LANES)
        return r;

    g->stock[p][color]--;

    if (lane == SAFE_LANE) {
        g->safe[color]++;
    } else {
        int top = NO_CHIP;
        if (g->lane_n[lane] == LANE_CAP)
            top = g->lane[lane][LANE_CAP - 1];
        else
            g->lane_n[lane]++;
        for (s = g->lane_n[lane] - 1; s > 0; s--)
            g->lane[lane][s] = g->lane[lane][s - 1];
        g->lane[lane][0] = color;

        if (top != NO_CHIP) {
            r.kicked = top;
            if (top == p) {
                /* Penalidade: recolhe a expulsa + toda a raia. */
                r.penalty = 1;
                g->stock[p][top]++;
                r.penalty_n = 1;
                for (s = 0; s < g->lane_n[lane]; s++) {
                    g->stock[p][g->lane[lane][s]]++;
                    g->lane[lane][s] = NO_CHIP;
                    r.penalty_n++;
                }
                g->lane_n[lane] = 0;
            } else if (top < g->num_players) {
                g->stock[top][top]++;
            }
        }
    }

    if (game_stock_total(g, p) == 0) {
        g->winner = p;
        r.won = 1;
    }
    return r;
}

int game_end_move(Game *g, const Move *m)
{
    if (g->winner >= 0) return 0;
    if (m && m->uses_seven && !g->bonus_roll) {
        g->bonus_roll = 1;
        return 1;
    }
    g->bonus_roll = 0;
    g->current = (g->current + 1) % g->num_players;
    return 0;
}
