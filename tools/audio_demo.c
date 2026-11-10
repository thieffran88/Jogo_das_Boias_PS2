/* Gera previas em WAV das musicas e efeitos: ./build/audio_demo dir */
#include "../src/audio.h"
#include <stdio.h>
#include <stdlib.h>

static void wr32(FILE *f, unsigned v) { fputc(v & 255, f); fputc(v >> 8 & 255, f); fputc(v >> 16 & 255, f); fputc(v >> 24, f); }

static void render_to(const char *path, int seconds, void (*start)(void))
{
    FILE *f = fopen(path, "wb");
    static short buf[1024 * 2];
    long n = (long)seconds * AUDIO_RATE, done = 0;
    if (!f) { perror(path); exit(1); }
    fwrite("RIFF", 1, 4, f); wr32(f, 36 + n * 4); fwrite("WAVEfmt ", 1, 8, f);
    wr32(f, 16); wr32(f, 1 | (2 << 16)); wr32(f, AUDIO_RATE); wr32(f, AUDIO_RATE * 4); wr32(f, 4 | (16 << 16));
    fwrite("data", 1, 4, f); wr32(f, n * 4);
    start();
    while (done < n) {
        int k = n - done > 1024 ? 1024 : (int)(n - done);
        audio_render(buf, k);
        fwrite(buf, 4, k, f);
        done += k;
    }
    fclose(f);
}

static void s_title(void) { audio_music(SONG_TITLE); }
static void s_game(void) { audio_music(SONG_GAME); }
static void s_win(void) { audio_music(SONG_WIN); }
static void s_none(void) { audio_music(SONG_NONE); }

int main(int argc, char **argv)
{
    char p[512];
    int i;
    const char *dir = argc > 1 ? argv[1] : ".";
    audio_init();
    snprintf(p, sizeof p, "%s/musica_titulo.wav", dir); render_to(p, 18, s_title);
    snprintf(p, sizeof p, "%s/musica_partida.wav", dir); render_to(p, 21, s_game);
    snprintf(p, sizeof p, "%s/vitoria.wav", dir); render_to(p, 5, s_win);
    /* efeitos: um a cada 0,8 s */
    {
        FILE *f;
        static short buf[AUDIO_RATE * 4 / 5 * 2];
        int per = AUDIO_RATE * 4 / 5;
        s_none();
        snprintf(p, sizeof p, "%s/efeitos.wav", dir);
        f = fopen(p, "wb");
        fwrite("RIFF", 1, 4, f); wr32(f, 36 + SFX_COUNT * per * 4); fwrite("WAVEfmt ", 1, 8, f);
        wr32(f, 16); wr32(f, 1 | (2 << 16)); wr32(f, AUDIO_RATE); wr32(f, AUDIO_RATE * 4); wr32(f, 4 | (16 << 16));
        fwrite("data", 1, 4, f); wr32(f, SFX_COUNT * per * 4);
        for (i = 0; i < SFX_COUNT; i++) {
            audio_sfx(i);
            audio_render(buf, per);
            fwrite(buf, 4, per, f);
        }
        fclose(f);
    }
    return 0;
}
