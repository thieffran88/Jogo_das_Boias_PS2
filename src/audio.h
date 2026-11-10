/*
 * Jogo das Boias PS2 - musica e efeitos sonoros.
 *
 * Tudo e sintetizado no proprio jogo (nenhum arquivo de audio): os
 * instrumentos (steel drum, marimba, baixo, percussao) e os efeitos
 * (splash, apito do salva-vidas, bolhas, dados...) sao gerados em
 * audio_init() e tocados por um mixer em software de 48 kHz estereo.
 *
 * audio_sfx()/audio_music() podem ser chamados pelo jogo enquanto
 * audio_render() roda em outra thread: os pedidos passam por uma fila.
 */
#ifndef AUDIO_H
#define AUDIO_H

#define AUDIO_RATE 48000

enum {
    SFX_MOVE,       /* cursor (bolha) */
    SFX_CONFIRM,
    SFX_BACK,
    SFX_PAUSE,
    SFX_DICE,       /* dados chacoalhando */
    SFX_DICE_LAND,
    SFX_SPLASH,     /* boia caindo na agua */
    SFX_BOING,      /* boia expulsa */
    SFX_WHISTLE,    /* apito do salva-vidas (penalidade) */
    SFX_CHIME,      /* boia segura na raia 7 */
    SFX_SPARKLE,    /* bonus */
    SFX_POP,        /* troca de vez */
    SFX_COUNT
};

enum {
    SONG_NONE,
    SONG_TITLE,     /* calipso animado */
    SONG_GAME,      /* tema tranquilo da partida */
    SONG_WIN,       /* fanfarra de vitoria (toca uma vez) */
    SONG_COUNT
};

void audio_init(void);
void audio_sfx(int id);
void audio_music(int song);        /* nao reinicia se ja estiver tocando */
void audio_set_music_on(int on);
int  audio_music_on(void);
/* Gera 'frames' quadros estereo intercalados (L, R) em 16 bits. */
void audio_render(short *out, int frames);

#endif
