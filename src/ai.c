#include "ai.h"

/* Avalia um movimento simulando-o numa copia do jogo. */
static int evaluate(const Game *g, const Move *m)
{
    Game t = *g;
    int me = g->current;
    int before = game_stock_total(g, me);
    int opp_before = 0, opp_after = 0;
    int score, p, i, l;

    for (p = 0; p < g->num_players; p++)
        if (p != me) opp_before += game_stock_total(g, p);

    for (i = 0; i < m->n; i++) {
        PlaceResult r = game_place(&t, m->lanes[i]);
        if (r.won) return 100000;
    }

    for (p = 0; p < t.num_players; p++)
        if (p != me) opp_after += game_stock_total(&t, p);

    score = (before - game_stock_total(&t, me)) * 100;
    score += (opp_after - opp_before) * 45;

    if (m->uses_seven && !g->bonus_roll) score += 120; /* joga de novo */
    if (m->uses_seven) score += 20;                    /* boia segura */

    /* Exposicao: boias minhas no topo de raias cheias podem ser expulsas
     * por adversarios e voltar para mim; boias minhas logo abaixo do topo
     * podem me gerar penalidade se eu mesmo jogar ali. */
    for (l = 1; l <= NUM_LANES; l++) {
        int n = t.lane_n[l];
        if (l == SAFE_LANE || n == 0) continue;
        if (t.lane[l][n - 1] == me) {
            int risk = (n == LANE_CAP) ? 30 : 12;
            /* raias centrais (6, 8) saem mais nos dados */
            if (l >= 5 && l <= 9) risk += 8;
            score -= risk;
        }
    }
    return score;
}

int ai_choose(const Game *g, const Move *moves, int n_moves)
{
    int i, best = 0, best_score = -1000000;
    for (i = 0; i < n_moves; i++) {
        int s = evaluate(g, &moves[i]);
        if (s > best_score) {
            best_score = s;
            best = i;
        }
    }
    return best;
}
