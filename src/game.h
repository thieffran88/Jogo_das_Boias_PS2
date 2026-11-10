/*
 * Jogo das Boias PS2 - motor de regras (C puro, sem dependencias do PS2).
 *
 * Resumo das regras implementadas:
 *  - 2 a 4 jogadores, 12 boias cada (vermelho, verde, roxo, amarelo).
 *  - A piscina tem 12 raias (1..12). A raia 7 e a "Torre do Salva-Vidas":
 *    capacidade ilimitada e as boias ali nunca voltam.
 *  - As demais raias tem 3 casas. Uma boia nova entra sempre por baixo e
 *    empurra as outras para cima; se a raia ja tem 3, a de cima e expulsa
 *    e volta para a reserva do dono (cor).
 *  - Penalidade: se a boia expulsa for da SUA cor, voce recolhe todas as
 *    boias daquela raia (qualquer cor) para sua reserva.
 *  - Boias de outras cores na sua reserva devem ser colocadas primeiro.
 *  - Em cada turno rolam-se 3 dados. Os dados podem ser usados
 *    separadamente ou somados (grupos de 2 ou 3), uma boia por grupo.
 *  - Bonus: somar 7 com 2 ou 3 dados coloca uma boia na Torre e da direito
 *    a jogar de novo (apenas uma vez; no lance bonus o 7 vale, mas nao ha
 *    terceiro lance).
 *  - Vence quem esvaziar a reserva primeiro.
 */
#ifndef GAME_H
#define GAME_H

#define NUM_LANES        12
#define SAFE_LANE        7
#define LANE_CAP         3
#define MAX_PLAYERS      4
#define CHIPS_PER_PLAYER 12
#define MAX_MOVES        5
#define NO_CHIP          (-1)

typedef struct {
    int num_players;
    int is_cpu[MAX_PLAYERS];
    int stock[MAX_PLAYERS][MAX_PLAYERS]; /* stock[jogador][cor] */
    int lane[NUM_LANES + 1][LANE_CAP];   /* [raia][casa], casa 0 = fundo */
    int lane_n[NUM_LANES + 1];
    int safe[MAX_PLAYERS];               /* boias por cor na raia 7 */
    int current;
    int dice[3];
    int bonus_roll;                      /* 1 durante o lance bonus */
    int winner;                          /* -1 enquanto ninguem venceu */
    unsigned int rng;
} Game;

typedef struct {
    int n;           /* quantas boias (1..3) */
    int lanes[3];    /* raia de cada boia, na ordem de colocacao */
    int groups[3];   /* mascara de bits dos dados usados por grupo */
    int uses_seven;  /* algum grupo de 2+ dados soma 7 */
} Move;

typedef struct {
    int lane;
    int color;        /* cor da boia colocada (NO_CHIP se nao havia) */
    int kicked;       /* cor da boia expulsa ou NO_CHIP */
    int penalty;      /* 1 se houve penalidade */
    int penalty_n;    /* boias recolhidas na penalidade */
    int won;          /* 1 se esta colocacao venceu o jogo */
} PlaceResult;

void game_init(Game *g, int num_players, const int *is_cpu, unsigned int seed);
unsigned int game_rand(Game *g);
int  game_roll_die(Game *g);
void game_roll(Game *g);
void game_set_dice(Game *g, int a, int b, int c);

int  game_stock_total(const Game *g, int player);
int  game_chips_on_board(const Game *g, int color);
int  game_next_color(const Game *g, int player);

int  game_moves(const Game *g, Move out[MAX_MOVES]);
PlaceResult game_place(Game *g, int lane);

/* Depois de aplicar todas as boias de um movimento:
 * retorna 1 se o jogador ganha um lance bonus (e ja prepara o estado),
 * 0 se o turno passou para o proximo jogador. */
int  game_end_move(Game *g, const Move *m);

#endif
