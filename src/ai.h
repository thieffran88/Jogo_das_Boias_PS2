#ifndef AI_H
#define AI_H

#include "game.h"

/* Escolhe o melhor movimento para o jogador atual (indice em moves). */
int ai_choose(const Game *g, const Move *moves, int n_moves);

#endif
