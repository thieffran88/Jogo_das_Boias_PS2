#include "lang.h"

int g_lang = LANG_PT;

const char *const LANG_NAME[LANG_COUNT] = { "Português", "English", "Español" };

const char *const PLAYER_NAME[LANG_COUNT][4] = {
    { "Vermelho", "Amarelo", "Azul", "Verde" },
    { "Red", "Yellow", "Blue", "Green" },
    { "Rojo", "Amarillo", "Azul", "Verde" },
};

/* cor da boia como adjetivo (PT: "boia vermelha", EN: "red float",
 * ES: "flotador rojo") */
const char *const COLOR_ADJ[LANG_COUNT][4] = {
    { "vermelha", "amarela", "azul", "verde" },
    { "red", "yellow", "blue", "green" },
    { "rojo", "amarillo", "azul", "verde" },
};

const char *const TXT[T_COUNT][LANG_COUNT] = {
    /* T_WHO_STARTS */ { "Quem começa?", "Who starts?", "¿Quién empieza?" },
    /* T_TURN_OF */ { "Vez do %s", "%s's turn", "Turno de %s" },
    /* T_TURN_OF_DOT */ { "Vez do %s.", "%s's turn.", "Turno de %s." },
    /* T_BONUS */ { "BÔNUS", "BONUS", "BONUS" },
    /* T_CPU_ROLLS */ { "%s (CPU) vai rolar...", "%s (CPU) is rolling...", "%s (CPU) va a lanzar..." },
    /* T_ROLL_DICE */ { "Rolar os dados!", "Roll the dice!", "¡Lanza los dados!" },
    /* T_ROLLING */ { "Rolando...", "Rolling...", "Lanzando..." },
    /* T_EACH_ROLLS */ { "Cada jogador rola um dado: o maior começa.",
                         "Each player rolls one die: the highest starts.",
                         "Cada jugador lanza un dado: el mayor empieza." },
    /* T_WON_MSG */ { "%s esvaziou a reserva e venceu!", "%s emptied the stock and won!",
                      "¡%s vació su reserva y ganó!" },
    /* T_SEVEN */ { "Somou 7! %s ganha mais um lance.", "Made 7! %s gets another roll.",
                    "¡Sumó 7! %s tira otra vez." },
    /* T_SAFE */ { "Boia segura com o salva-vidas da raia 7!",
                   "Float safe with the lifeguard in lane 7!",
                   "¡Flotador a salvo con el socorrista del carril 7!" },
    /* T_PENALTY */ { "PENALIDADE! %s expulsou a própria boia e recolhe %d da raia %d.",
                      "PENALTY! %s kicked out their own float and takes %d from lane %d.",
                      "¡PENALIZACIÓN! %s expulsó su propio flotador y recoge %d del carril %d." },
    /* T_KICKED */ { "Boia %s expulsa da raia %d! Volta para o %s.",
                     "The %s float was kicked out of lane %d! Back to %s.",
                     "¡Flotador %s expulsado del carril %d! Vuelve a %s." },
    /* T_PLACED */ { "Boia colocada na raia %d.", "Float placed in lane %d.",
                     "Flotador colocado en el carril %d." },
    /* T_HIGHEST */ { "%s tirou o maior número e começa!", "%s rolled the highest and starts!",
                      "¡%s sacó el número más alto y empieza!" },
    /* T_TIE */ { "Empate! Os empatados rolam de novo...", "Tie! The tied players roll again...",
                  "¡Empate! Los empatados lanzan otra vez..." },
    /* T_CHOOSE */ { "Escolha onde colocar as boias (estrela = soma 7, bônus).",
                     "Choose where to place your floats (star = sum of 7, bonus).",
                     "Elige dónde colocar los flotadores (estrella = suma 7, bonus)." },
    /* T_CONFIRM */ { "Confirmar", "Confirm", "Confirmar" },
    /* T_SELECT */ { "Escolher", "Select", "Elegir" },
    /* T_BACK */ { "Voltar", "Back", "Volver" },
    /* T_PLAY */ { "Jogar", "Play", "Jugar" },
    /* T_PAGE */ { "Página", "Page", "Página" },
    /* T_PLAYERS_TITLE */ { "JOGADORES", "PLAYERS", "JUGADORES" },
    /* T_N_PLAYERS */ { "%d JOGADORES", "%d PLAYERS", "%d JUGADORES" },
    /* T_HUMAN */ { "HUMANO", "HUMAN", "HUMANO" },
    /* T_OUT */ { "fora", "off", "fuera" },
    /* T_HOWTO_TITLE */ { "COMO JOGAR", "HOW TO PLAY", "COMO JUGAR" },
    /* T_CREDITS_TITLE */ { "CREDITOS", "CREDITS", "CREDITOS" },
    /* T_CRED1 */ { "Arte e ideia: equipe Jogo das Boias", "Art and idea: Jogo das Boias team",
                    "Arte e idea: equipo Jogo das Boias" },
    /* T_CRED2 */ { "Programação: C, PS2SDK e gsKit", "Programming: C, PS2SDK and gsKit",
                    "Programación: C, PS2SDK y gsKit" },
    /* T_CRED3 */ { "Música e sons: sintetizados no jogo", "Music and sounds: synthesized in-game",
                    "Música y sonidos: sintetizados en el juego" },
    /* T_CRED4 */ { "Texto: fonte DejaVu Sans", "Text: DejaVu Sans font", "Texto: fuente DejaVu Sans" },
    /* T_CRED5 */ { "Homebrew para PlayStation 2", "Homebrew for PlayStation 2",
                    "Homebrew para PlayStation 2" },
    /* T_BONUS_BIG */ { "BÔNUS!", "BONUS!", "¡BONUS!" },
    /* T_PLAY_AGAIN */ { "Jogue de novo!", "Roll again!", "¡Tira otra vez!" },
    /* T_WINS */ { "%s venceu!", "%s wins!", "¡%s gana!" },
    /* T_FIRST_EMPTY */ { "Foi o primeiro a esvaziar a reserva!", "First to empty the stock!",
                          "¡Fue el primero en vaciar la reserva!" },
    /* T_BACK_MENU */ { "Voltar ao menu", "Back to menu", "Volver al menú" },
    /* T_PAUSE */ { "PAUSA", "PAUSED", "PAUSA" },
    /* T_MUSIC_ON */ { "Música: ligada", "Music: on", "Música: activada" },
    /* T_MUSIC_OFF */ { "Música: desligada", "Music: off", "Música: desactivada" },
    /* T_LANGUAGE */ { "Idioma", "Language", "Idioma" },
    /* T_PAD_N */ { "Controle %d", "Controller %d", "Mando %d" },
    /* T_BTN_PLAY */ { "JOGAR", "PLAY", "JUGAR" },
    /* T_BTN_HOWTO */ { "COMO JOGAR", "HOW TO PLAY", "CÓMO JUGAR" },
    /* T_BTN_CREDITS */ { "CRÉDITOS", "CREDITS", "CRÉDITOS" },
    /* T_BTN_CONTINUE */ { "CONTINUAR", "CONTINUE", "CONTINUAR" },
    /* T_BTN_QUIT */ { "SAIR", "QUIT", "SALIR" },
    /* T_VICTORY */ { "VITORIA!", "VICTORY!", "VICTORIA!" },
    /* T_DEFEAT */ { "DERROTA!", "DEFEAT!", "DERROTA!" },
};

const char *const RULE_TITLE[LANG_COUNT][N_RULES] = {
    { "OBJETIVO", "OS DADOS", "COLOCANDO BOIAS", "PENALIDADE", "BONUS DO 7", "CONTROLES" },
    { "GOAL", "THE DICE", "PLACING FLOATS", "PENALTY", "BONUS OF 7", "CONTROLS" },
    { "OBJETIVO", "LOS DADOS", "COLOCAR FLOTADORES", "PENALIZACION", "BONUS DEL 7", "CONTROLES" },
};

const char *const RULE_BODY[LANG_COUNT][N_RULES] = {
    {
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
        "START: pausar o jogo  |  SELECT: música\n"
        "Controle 1: Jogador 1 (Vermelho).\n"
        "Controle 2: Jogadores 2, 3 e 4 (passem o controle 2 entre si).\n"
        "Menus aceitam os dois controles."
    },
    {
        "Each player starts with 12 floats of their color in stock.\n"
        "Be the first to empty your stock by placing your floats in the pool lanes.\n"
        "Lanes go from 1 to 12. The lifeguard sits in lane 7.\n"
        "To start, everyone rolls one die: the highest roll goes first.",

        "On your turn, roll the 3 dice. Each die places 1 float in the lane with its number, "
        "or you can add 2 or 3 dice together to reach higher lanes (placing fewer floats).\n"
        "E.g. 6, 4 and 2 allow: lanes 2, 4 and 6 / lanes 10 and 2 / lanes 8 and 4 / "
        "two floats in lane 6 / one float in lane 12.",

        "A float always enters the bottom space and pushes the others up.\n"
        "Each lane holds at most 3 floats. If the lane is full, the top float is "
        "kicked out and goes back to its owner's stock (same color).",

        "If you kick out a float of YOUR color, you get a penalty: you take ALL the "
        "floats in that lane, of any color.\n"
        "Floats of other colors in your stock must be placed first. They still belong "
        "to your opponents if they are kicked out later.",

        "Made 7 with 2 or 3 dice? The float goes to lane 7, with the lifeguard (no limit, "
        "and it never comes back), and you roll again!\n"
        "On the bonus roll a 7 still counts, but there is no third roll.\n"
        "Using the 7 is optional. The first player out of floats wins!",

        "D-pad: choose option\n"
        "X: confirm / roll the dice\n"
        "O: back\n"
        "START: pause  |  SELECT: music on/off\n"
        "Controller 1: Player 1 (Red).\n"
        "Controller 2: Players 2, 3 and 4 (pass controller 2 around).\n"
        "Menus accept both controllers."
    },
    {
        "Cada jugador empieza con 12 flotadores de su color en la reserva.\n"
        "Sé el primero en vaciar tu reserva colocando los flotadores en los carriles de la piscina.\n"
        "Los carriles van del 1 al 12. En el carril 7 está el socorrista.\n"
        "Para empezar, cada uno lanza un dado: quien saque más juega primero.",

        "En tu turno, lanza los 3 dados. Cada dado coloca 1 flotador en el carril de su número, "
        "o puedes sumar 2 o 3 dados para llegar a carriles más altos (colocando menos flotadores).\n"
        "Ej.: 6, 4 y 2 permiten: carriles 2, 4 y 6 / carriles 10 y 2 / carriles 8 y 4 / "
        "dos flotadores en el 6 / un flotador en el 12.",

        "El flotador siempre entra por la casilla de abajo y empuja a los demás hacia arriba.\n"
        "Cada carril tiene como máximo 3 flotadores. Si el carril está lleno, el de arriba "
        "es expulsado y vuelve a la reserva de su dueño (del mismo color).",

        "Si expulsas un flotador de TU color, recibes una penalización: recoges TODOS los "
        "flotadores de ese carril, de cualquier color.\n"
        "Los flotadores de otros colores en tu reserva deben colocarse primero. Siguen siendo "
        "de los rivales si luego son expulsados.",

        "¿Sumaste 7 con 2 o 3 dados? El flotador va al carril 7, con el socorrista (sin límite, "
        "y de allí nunca vuelve), ¡y juegas otra vez!\n"
        "En la tirada extra el 7 sigue valiendo, pero no hay una tercera tirada.\n"
        "Usar el 7 es opcional. ¡Gana quien se quede sin flotadores primero!",

        "Cruceta: elegir opción\n"
        "X: confirmar / lanzar los dados\n"
        "O: volver\n"
        "START: pausa  |  SELECT: música\n"
        "Mando 1: Jugador 1 (Rojo).\n"
        "Mando 2: Jugadores 2, 3 y 4 (pásense el mando 2).\n"
        "Los menús aceptan los dos mandos."
    },
};
