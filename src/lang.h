/* Jogo das Boias PS2 - textos em portugues, ingles e espanhol. */
#ifndef LANG_H
#define LANG_H

enum { LANG_PT, LANG_EN, LANG_ES, LANG_COUNT };

enum {
    T_WHO_STARTS, T_TURN_OF, T_TURN_OF_DOT, T_BONUS, T_CPU_ROLLS, T_ROLL_DICE,
    T_ROLLING, T_EACH_ROLLS, T_WON_MSG, T_SEVEN, T_SAFE, T_PENALTY, T_KICKED,
    T_PLACED, T_HIGHEST, T_TIE, T_CHOOSE, T_CONFIRM, T_SELECT, T_BACK, T_PLAY,
    T_PAGE, T_PLAYERS_TITLE, T_N_PLAYERS, T_HUMAN, T_OUT, T_HOWTO_TITLE,
    T_CREDITS_TITLE, T_CRED1, T_CRED2, T_CRED3, T_CRED4, T_CRED5, T_BONUS_BIG,
    T_PLAY_AGAIN, T_WINS, T_FIRST_EMPTY, T_BACK_MENU, T_PAUSE, T_MUSIC_ON,
    T_MUSIC_OFF, T_LANGUAGE, T_PAD_N,
    /* rotulos dos botoes e emblemas desenhados por cima da arte */
    T_BTN_PLAY, T_BTN_HOWTO, T_BTN_CREDITS, T_BTN_CONTINUE, T_BTN_QUIT,
    T_VICTORY, T_DEFEAT,
    T_COUNT
};

#define N_RULES 6

extern int g_lang;
extern const char *const LANG_NAME[LANG_COUNT];
extern const char *const TXT[T_COUNT][LANG_COUNT];
extern const char *const PLAYER_NAME[LANG_COUNT][4];
extern const char *const COLOR_ADJ[LANG_COUNT][4];
extern const char *const RULE_TITLE[LANG_COUNT][N_RULES];
extern const char *const RULE_BODY[LANG_COUNT][N_RULES];

#define T(id) (TXT[(id)][g_lang])

#endif
