#ifndef UI_H
#define UI_H

/* Botoes (mascara de bits), independentes do hardware. */
enum {
    BTN_UP = 1 << 0,
    BTN_DOWN = 1 << 1,
    BTN_LEFT = 1 << 2,
    BTN_RIGHT = 1 << 3,
    BTN_CROSS = 1 << 4,
    BTN_CIRCLE = 1 << 5,
    BTN_TRIANGLE = 1 << 6,
    BTN_SQUARE = 1 << 7,
    BTN_START = 1 << 8,
    BTN_SELECT = 1 << 9
};

/* Fornecido pelo backend: contador qualquer para semente aleatoria. */
unsigned int platform_ticks(void);

void ui_init(void);
/* Botoes que acabaram de ser apertados neste quadro em cada controle.
 * Menus aceitam os dois controles; na partida, o controle 1 joga so pelo
 * Jogador 1 e o controle 2 joga pelos Jogadores 2, 3 e 4. */
void ui_update(unsigned int pad1, unsigned int pad2);
void ui_draw(void);

#endif
