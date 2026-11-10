/* Testes do motor de regras (rodam no PC: make test). */
#include "../src/game.h"
#include "../src/ai.h"
#include <stdio.h>
#include <string.h>

static int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("FALHOU %s:%d: %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static int has_move(const Move *m, int n, int k, int a, int b, int c)
{
    int i;
    for (i = 0; i < n; i++) {
        if (m[i].n != k) continue;
        if (m[i].lanes[0] == a && (k < 2 || m[i].lanes[1] == b) && (k < 3 || m[i].lanes[2] == c))
            return 1;
    }
    return 0;
}

static void test_moves_example(void)
{
    /* Exemplo do manual: 6 + 4 + 2 */
    Game g;
    Move m[MAX_MOVES];
    int n;
    game_init(&g, 2, NULL, 1);
    game_set_dice(&g, 6, 4, 2);
    n = game_moves(&g, m);
    CHECK(n == 5);
    CHECK(has_move(m, n, 3, 2, 4, 6));
    CHECK(has_move(m, n, 2, 2, 10, 0));
    CHECK(has_move(m, n, 2, 4, 8, 0));
    CHECK(has_move(m, n, 2, 6, 6, 0));
    CHECK(has_move(m, n, 1, 12, 0, 0));
}

static void test_moves_limits_and_seven(void)
{
    Game g;
    Move m[MAX_MOVES];
    int n, i, sevens = 0;
    game_init(&g, 2, NULL, 1);
    game_set_dice(&g, 6, 6, 6);  /* soma 18 nao existe; 6,6,6 e 12,6 */
    n = game_moves(&g, m);
    CHECK(n == 2);
    CHECK(has_move(m, n, 3, 6, 6, 6));
    CHECK(has_move(m, n, 2, 6, 12, 0));

    game_set_dice(&g, 3, 4, 5);
    n = game_moves(&g, m);
    for (i = 0; i < n; i++) if (m[i].uses_seven) sevens++;
    CHECK(sevens == 1);           /* 3+4 = 7 (e 5 separado) */
    CHECK(has_move(m, n, 2, 5, 7, 0));

    game_set_dice(&g, 1, 1, 5);   /* 1+1+5 = 7 com 3 dados */
    n = game_moves(&g, m);
    CHECK(has_move(m, n, 1, 7, 0, 0));
    sevens = 0;
    for (i = 0; i < n; i++) if (m[i].uses_seven) sevens++;
    CHECK(sevens == 1);
}

static void test_push_and_kick(void)
{
    Game g;
    PlaceResult r;
    game_init(&g, 2, NULL, 1);
    g.current = 0;
    game_place(&g, 3);            /* V */
    g.current = 1;
    game_place(&g, 3);            /* G empurra V */
    game_place(&g, 3);            /* G empurra */
    CHECK(g.lane_n[3] == 3);
    CHECK(g.lane[3][2] == 0);     /* vermelho no topo */
    /* Verde coloca de novo: expulsa o vermelho, que volta ao dono */
    r = game_place(&g, 3);
    CHECK(r.kicked == 0 && !r.penalty);
    CHECK(g.stock[0][0] == 12);
    CHECK(g.stock[1][1] == 12 - 3);
    CHECK(g.lane_n[3] == 3);
}

static void test_penalty(void)
{
    Game g;
    PlaceResult r;
    game_init(&g, 2, NULL, 1);
    g.current = 0;
    game_place(&g, 5);            /* V no fundo */
    g.current = 1;
    game_place(&g, 5);
    game_place(&g, 5);            /* raia: V(topo), G, G */
    g.current = 0;
    r = game_place(&g, 5);        /* vermelho expulsa o proprio vermelho */
    CHECK(r.penalty);
    CHECK(r.penalty_n == 4);      /* expulsa + 3 da raia */
    CHECK(g.lane_n[5] == 0);
    CHECK(g.stock[0][0] == 12);   /* 10 + 2 vermelhas de volta */
    CHECK(g.stock[0][1] == 2);    /* 2 verdes agora na reserva do vermelho */
    CHECK(game_stock_total(&g, 0) == 14);
    /* Verdes devem sair primeiro */
    CHECK(game_next_color(&g, 0) == 1);
    r = game_place(&g, 9);
    CHECK(r.color == 1);
    CHECK(g.lane[9][0] == 1);
}

static void test_foreign_kicked_goes_to_owner(void)
{
    Game g;
    game_init(&g, 3, NULL, 1);
    g.stock[0][2] = 1;            /* vermelho tem 1 roxa */
    g.current = 0;
    game_place(&g, 4);            /* coloca a roxa */
    CHECK(g.lane[4][0] == 2);
    g.current = 1;
    game_place(&g, 4);
    game_place(&g, 4);
    game_place(&g, 4);            /* expulsa a roxa: volta para o Roxo */
    CHECK(g.stock[2][2] == 13);
    CHECK(g.stock[0][2] == 0);
}

static void test_safe_lane_and_bonus(void)
{
    Game g;
    Move m;
    int i;
    game_init(&g, 2, NULL, 1);
    g.current = 0;
    for (i = 0; i < 5; i++) game_place(&g, 7);
    CHECK(g.safe[0] == 5);
    CHECK(g.lane_n[7] == 0);
    memset(&m, 0, sizeof(m));
    m.uses_seven = 1;
    CHECK(game_end_move(&g, &m) == 1);   /* bonus */
    CHECK(g.current == 0 && g.bonus_roll == 1);
    CHECK(game_end_move(&g, &m) == 0);   /* sem terceiro lance */
    CHECK(g.current == 1 && g.bonus_roll == 0);
}

static void test_win(void)
{
    Game g;
    PlaceResult r;
    game_init(&g, 2, NULL, 1);
    g.current = 1;
    g.stock[1][1] = 1;
    r = game_place(&g, 7);
    CHECK(r.won && g.winner == 1);
}

static void test_full_games(void)
{
    /* Partidas CPU x CPU completas: o jogo deve sempre terminar
     * e conservar o numero total de boias. */
    int seed, games = 0, total_turns = 0;
    for (seed = 1; seed <= 300; seed++) {
        Game g;
        int cpu[4] = {1, 1, 1, 1};
        int np = 2 + seed % 3, turns = 0;
        game_init(&g, np, cpu, (unsigned)seed * 7919u);
        while (g.winner < 0 && turns < 5000) {
            Move m[MAX_MOVES];
            int n, k, i, c, sum;
            game_roll(&g);
            n = game_moves(&g, m);
            CHECK(n >= 1);
            k = (seed % 2) ? ai_choose(&g, m, n) : (int)(game_rand(&g) % (unsigned)n);
            for (i = 0; i < m[k].n; i++) game_place(&g, m[k].lanes[i]);
            game_end_move(&g, &m[k]);
            for (c = 0; c < np; c++) {
                int p, l, s;
                sum = g.safe[c];
                for (p = 0; p < np; p++) sum += g.stock[p][c];
                for (l = 1; l <= NUM_LANES; l++)
                    for (s = 0; s < g.lane_n[l]; s++)
                        if (g.lane[l][s] == c) sum++;
                if (sum != CHIPS_PER_PLAYER) { CHECK(sum == CHIPS_PER_PLAYER); break; }
            }
            turns++;
        }
        CHECK(g.winner >= 0);
        games++;
        total_turns += turns;
    }
    printf("%d partidas simuladas, media de %d jogadas\n", games, total_turns / games);
}

int main(void)
{
    test_moves_example();
    test_moves_limits_and_seven();
    test_push_and_kick();
    test_penalty();
    test_foreign_kicked_goes_to_owner();
    test_safe_lane_and_bonus();
    test_win();
    test_full_games();
    if (fails) {
        printf("%d falha(s)\n", fails);
        return 1;
    }
    printf("Todos os testes passaram.\n");
    return 0;
}
