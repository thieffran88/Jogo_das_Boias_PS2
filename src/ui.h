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
/* pressed = botoes que acabaram de ser apertados neste quadro */
void ui_update(unsigned int pressed);
void ui_draw(void);

#endif
