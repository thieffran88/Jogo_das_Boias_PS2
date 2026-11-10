/*
 * Jogo das Boias PS2 - sintetizador, sequenciador e mixer.
 * C puro (sem dependencias do PS2): roda igual no PS2 e no simulador.
 */
#include "audio.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define SR ((float)AUDIO_RATE)
#define TWO_PI 6.28318530718f

/* ------------------------------------------------------------------ */
/* Amostras sintetizadas                                               */
/* ------------------------------------------------------------------ */

typedef struct {
    short *data;
    int len;
    int base;        /* nota MIDI da gravacao (instrumentos afinados) */
} Sample;

enum {
    SMP_STEEL, SMP_MARIMBA, SMP_BASS, SMP_SHAKER, SMP_KICK, SMP_BONGO,
    SMP_SPLASH, SMP_BOING, SMP_WHISTLE, SMP_DICE, SMP_DICE_LAND,
    SMP_BLIP, SMP_POP, SMP_CHIME, SMP_SPARKLE,
    SMP_COUNT
};

static Sample smp[SMP_COUNT];
static unsigned int noise_state = 0x13579BDFu;

static float noise(void)
{
    noise_state = noise_state * 1664525u + 1013904223u;
    return ((int)(noise_state >> 8) - 0x800000) / (float)0x800000;
}

static float frand(float a, float b)
{
    noise_state = noise_state * 1664525u + 1013904223u;
    return a + (b - a) * ((noise_state >> 8) / (float)0x1000000);
}

static float midi_hz(float n) { return 440.0f * powf(2.0f, (n - 69.0f) / 12.0f); }

static float *tmp_buf(int len)
{
    float *b = (float *)calloc((size_t)len, sizeof(float));
    return b;
}

/* Normaliza para 'peak' e converte para 16 bits. */
static void store(int id, float *b, int len, int base, float peak)
{
    int i;
    float m = 1e-6f;
    short *d = (short *)malloc((size_t)len * sizeof(short));
    for (i = 0; i < len; i++)
        if (fabsf(b[i]) > m) m = fabsf(b[i]);
    for (i = 0; i < len; i++) {
        float v = b[i] / m * peak;
        if (v > 1) v = 1;
        if (v < -1) v = -1;
        d[i] = (short)(v * 32000.0f);
    }
    free(b);
    smp[id].data = d;
    smp[id].len = len;
    smp[id].base = base;
}

static float att(float t, float a) { return t < a ? t / a : 1.0f; }

static void gen_steel(void)
{
    int n = (int)(SR * 1.3f), i;
    float *b = tmp_buf(n), f = midi_hz(72), ph[4] = { 0, 0, 0, 0 };
    static const float ratio[4] = { 1.0f, 2.0f, 3.0f, 4.16f };
    static const float amp[4] = { 1.0f, 0.55f, 0.28f, 0.16f };
    static const float dec[4] = { 2.6f, 5.0f, 9.0f, 14.0f };
    for (i = 0; i < n; i++) {
        float t = i / SR, v = 0;
        float bend = 1.0f + 0.02f * expf(-t * 60.0f);
        int k;
        for (k = 0; k < 4; k++) {
            ph[k] += TWO_PI * f * ratio[k] * bend / SR;
            v += amp[k] * sinf(ph[k]) * expf(-t * dec[k]);
        }
        b[i] = v * att(t, 0.004f);
    }
    store(SMP_STEEL, b, n, 72, 0.9f);
}

static void gen_marimba(void)
{
    int n = (int)(SR * 0.9f), i;
    float *b = tmp_buf(n), f = midi_hz(72);
    for (i = 0; i < n; i++) {
        float t = i / SR;
        float v = sinf(TWO_PI * f * t) * expf(-t * 4.5f)
                + 0.35f * sinf(TWO_PI * f * 3.99f * t) * expf(-t * 22.0f)
                + 0.08f * sinf(TWO_PI * f * 9.9f * t) * expf(-t * 60.0f);
        b[i] = v * att(t, 0.002f);
    }
    store(SMP_MARIMBA, b, n, 72, 0.9f);
}

static void gen_bass(void)
{
    int n = (int)(SR * 0.9f), i;
    float *b = tmp_buf(n), f = midi_hz(48);
    for (i = 0; i < n; i++) {
        float t = i / SR;
        float v = sinf(TWO_PI * f * t)
                + 0.35f * sinf(TWO_PI * 2 * f * t) * expf(-t * 6.0f)
                + 0.12f * sinf(TWO_PI * 3 * f * t) * expf(-t * 10.0f);
        b[i] = v * att(t, 0.004f) * expf(-t * 3.5f);
    }
    store(SMP_BASS, b, n, 48, 0.95f);
}

static void gen_shaker(void)
{
    int n = (int)(SR * 0.07f), i;
    float *b = tmp_buf(n), prev = 0;
    for (i = 0; i < n; i++) {
        float t = i / SR, x = noise();
        b[i] = (x - prev) * att(t, 0.006f) * expf(-t * 70.0f);
        prev = x;
    }
    store(SMP_SHAKER, b, n, 60, 0.8f);
}

static void gen_kick(void)
{
    int n = (int)(SR * 0.3f), i;
    float *b = tmp_buf(n), ph = 0;
    for (i = 0; i < n; i++) {
        float t = i / SR;
        ph += TWO_PI * (50.0f + 90.0f * expf(-t * 25.0f)) / SR;
        b[i] = sinf(ph) * expf(-t * 9.0f) * att(t, 0.002f);
    }
    store(SMP_KICK, b, n, 60, 0.95f);
}

static void gen_bongo(void)
{
    int n = (int)(SR * 0.25f), i;
    float *b = tmp_buf(n), ph = 0;
    for (i = 0; i < n; i++) {
        float t = i / SR;
        ph += TWO_PI * 330.0f * (1.0f + 0.3f * expf(-t * 40.0f)) / SR;
        b[i] = sinf(ph) * expf(-t * 22.0f) + noise() * 0.4f * expf(-t * 900.0f);
    }
    store(SMP_BONGO, b, n, 60, 0.9f);
}

/* Bolhinha: seno subindo de frequencia. */
static void add_bubble(float *b, int n, float t0, float f0, float f1, float dur, float a)
{
    int i0 = (int)(t0 * SR), len = (int)(dur * SR), i;
    float ph = 0;
    for (i = 0; i < len && i0 + i < n; i++) {
        float u = (float)i / len;
        ph += TWO_PI * (f0 + (f1 - f0) * u * u) / SR;
        b[i0 + i] += a * sinf(ph) * sinf(3.14159f * u);
    }
}

static void gen_splash(void)
{
    int n = (int)(SR * 0.6f), i, k;
    float *b = tmp_buf(n), lp = 0, ph = 0;
    for (i = 0; i < n; i++) {
        float t = i / SR;
        float a = 0.5f * expf(-t * 4.0f) + 0.03f;
        lp += a * (noise() - lp);
        b[i] = lp * att(t, 0.005f) * expf(-t * 6.0f) * 1.6f;
        /* "bloop" da boia afundando e voltando */
        ph += TWO_PI * (140.0f + 380.0f * expf(-t * 7.0f)) / SR;
        b[i] += 0.5f * sinf(ph) * expf(-t * 14.0f);
    }
    for (k = 0; k < 5; k++)
        add_bubble(b, n, frand(0.08f, 0.42f), frand(500, 800), frand(1100, 1700),
                   frand(0.02f, 0.04f), 0.18f);
    store(SMP_SPLASH, b, n, 60, 0.95f);
}

static void gen_boing(void)
{
    int n = (int)(SR * 0.6f), i;
    float *b = tmp_buf(n), ph = 0;
    for (i = 0; i < n; i++) {
        float t = i / SR;
        float f = 180.0f + 260.0f * (1 - expf(-t * 6.0f))
                + 30.0f * sinf(TWO_PI * 18.0f * t) * expf(-t * 3.0f);
        ph += TWO_PI * f / SR;
        b[i] = (sinf(ph) + 0.2f * sinf(2 * ph)) * expf(-t * 4.5f) * att(t, 0.003f);
    }
    store(SMP_BOING, b, n, 60, 0.9f);
}

/* Apito do salva-vidas: dois sopros com o trinado da bolinha. */
static void gen_whistle(void)
{
    int n = (int)(SR * 1.0f), i;
    float *b = tmp_buf(n), ph = 0, hp = 0, prev = 0;
    for (i = 0; i < n; i++) {
        float t = i / SR, e = 0, trill;
        if (t < 0.22f) e = att(t, 0.015f) * (t > 0.19f ? (0.22f - t) / 0.03f : 1);
        else if (t > 0.32f && t < 0.95f)
            e = att(t - 0.32f, 0.015f) * (t > 0.9f ? (0.95f - t) / 0.05f : 1);
        trill = sinf(TWO_PI * 32.0f * t);
        ph += TWO_PI * 2750.0f * (1.0f + 0.03f * trill) / SR;
        hp = noise();
        b[i] = e * (sinf(ph) * (0.75f + 0.25f * trill) + 0.08f * (hp - prev));
        prev = hp;
    }
    store(SMP_WHISTLE, b, n, 60, 0.7f);
}

static void add_click(float *b, int n, float t0, float a)
{
    int i0 = (int)(t0 * SR), i;
    float f = frand(1500, 3200);
    for (i = 0; i < (int)(SR * 0.03f) && i0 + i < n; i++) {
        float t = i / SR;
        b[i0 + i] += a * (sinf(TWO_PI * f * t) * expf(-t * 180.0f) + noise() * expf(-t * 400.0f));
    }
}

static void gen_dice(void)
{
    int n = (int)(SR * 0.6f), k;
    float *b = tmp_buf(n);
    for (k = 0; k < 11; k++) add_click(b, n, frand(0.0f, 0.5f), frand(0.4f, 1.0f));
    store(SMP_DICE, b, n, 60, 0.8f);
}

static void gen_dice_land(void)
{
    int n = (int)(SR * 0.25f), i;
    float *b = tmp_buf(n);
    add_click(b, n, 0.0f, 1.0f);
    add_click(b, n, 0.06f, 0.6f);
    for (i = 0; i < n; i++) {
        float t = i / SR;
        b[i] += 0.8f * sinf(TWO_PI * 180.0f * t) * expf(-t * 35.0f);
    }
    store(SMP_DICE_LAND, b, n, 60, 0.85f);
}

static void gen_blip(void)
{
    int n = (int)(SR * 0.09f);
    float *b = tmp_buf(n);
    add_bubble(b, n, 0, 520, 1150, 0.09f, 1.0f);
    store(SMP_BLIP, b, n, 60, 0.7f);
}

static void gen_pop(void)
{
    int n = (int)(SR * 0.16f);
    float *b = tmp_buf(n);
    add_bubble(b, n, 0, 300, 1100, 0.12f, 1.0f);
    add_bubble(b, n, 0.07f, 500, 1500, 0.08f, 0.5f);
    store(SMP_POP, b, n, 60, 0.7f);
}

static void add_bell(float *b, int n, float t0, float f, float a)
{
    int i0 = (int)(t0 * SR), i;
    for (i = 0; i0 + i < n; i++) {
        float t = i / SR;
        b[i0 + i] += a * att(t, 0.002f) * (sinf(TWO_PI * f * t) * expf(-t * 3.0f)
                    + 0.5f * sinf(TWO_PI * f * 2.76f * t) * expf(-t * 7.0f)
                    + 0.25f * sinf(TWO_PI * f * 5.4f * t) * expf(-t * 12.0f));
    }
}

static void gen_chime(void)
{
    int n = (int)(SR * 1.1f);
    float *b = tmp_buf(n);
    add_bell(b, n, 0.0f, midi_hz(84), 1.0f);
    add_bell(b, n, 0.09f, midi_hz(91), 0.7f);
    store(SMP_CHIME, b, n, 60, 0.75f);
}

static void gen_sparkle(void)
{
    static const int notes[6] = { 84, 88, 91, 96, 100, 103 };
    int n = (int)(SR * 1.3f), k;
    float *b = tmp_buf(n);
    for (k = 0; k < 6; k++) add_bell(b, n, k * 0.07f, midi_hz(notes[k]), 1.0f - k * 0.1f);
    store(SMP_SPARKLE, b, n, 60, 0.7f);
}

/* ------------------------------------------------------------------ */
/* Musicas (composicoes originais)                                     */
/* ------------------------------------------------------------------ */

typedef struct { signed char bar, step, note; } NoteEv;

typedef struct {
    float bpm;
    int bars;
    int loop;
    int lead;                 /* amostra do solo */
    const signed char *chords;/* 2 por compasso: classe (0-11), +12 = menor */
    const NoteEv *melody;
    int n_melody;
    unsigned short kick, shaker, accent, bongo_hi, bongo_lo, comp;
    const signed char *bass;  /* 16 passos: intervalo+1 (0 = nada) */
    float lead_vol, comp_vol;
} Song;

#define MIN 12
static const signed char title_chords[16] = {
    0, 0, 9 + MIN, 9 + MIN, 5, 5, 7, 7, 0, 0, 9 + MIN, 9 + MIN, 2 + MIN, 7, 0, 0
};
static const NoteEv title_mel[] = {
    {0, 0, 76}, {0, 2, 79}, {0, 4, 84}, {0, 6, 79}, {0, 8, 81}, {0, 10, 79}, {0, 12, 76},
    {1, 0, 72}, {1, 3, 76}, {1, 6, 81}, {1, 8, 79}, {1, 10, 76}, {1, 12, 72}, {1, 14, 74},
    {2, 0, 77}, {2, 2, 81}, {2, 4, 84}, {2, 7, 81}, {2, 10, 77}, {2, 12, 79}, {2, 14, 81},
    {3, 0, 83}, {3, 3, 81}, {3, 6, 79}, {3, 8, 74}, {3, 10, 79}, {3, 12, 77}, {3, 14, 74},
    {4, 0, 76}, {4, 2, 76}, {4, 4, 79}, {4, 6, 84}, {4, 8, 83}, {4, 10, 84}, {4, 12, 79},
    {5, 0, 81}, {5, 2, 84}, {5, 4, 88}, {5, 6, 84}, {5, 8, 81}, {5, 11, 79}, {5, 14, 76},
    {6, 0, 77}, {6, 2, 81}, {6, 4, 86}, {6, 6, 81}, {6, 8, 83}, {6, 10, 79}, {6, 12, 74}, {6, 14, 77},
    {7, 0, 76}, {7, 4, 72}, {7, 6, 74}, {7, 8, 72},
};
static const signed char title_bass[16] = { 1, 0, 0, 0, 0, 0, 8, 0, 1, 0, 0, 13, 0, 0, 8, 0 };

static const signed char game_chords[16] = {
    5, 5, 2 + MIN, 2 + MIN, 10, 10, 0, 0, 5, 5, 2 + MIN, 2 + MIN, 7 + MIN, 0, 5, 5
};
static const NoteEv game_mel[] = {
    {0, 0, 81}, {0, 4, 84}, {0, 6, 81}, {0, 8, 77}, {0, 12, 79},
    {1, 0, 77}, {1, 4, 81}, {1, 6, 86}, {1, 10, 84}, {1, 12, 81},
    {2, 0, 82}, {2, 4, 86}, {2, 6, 84}, {2, 8, 82}, {2, 12, 81}, {2, 14, 79},
    {3, 0, 79}, {3, 4, 76}, {3, 8, 72}, {3, 12, 76}, {3, 14, 79},
    {4, 0, 81}, {4, 2, 84}, {4, 4, 89}, {4, 8, 88}, {4, 10, 84}, {4, 12, 81},
    {5, 0, 86}, {5, 4, 81}, {5, 8, 77}, {5, 12, 81},
    {6, 0, 82}, {6, 2, 81}, {6, 4, 79}, {6, 8, 76}, {6, 10, 79}, {6, 12, 84},
    {7, 0, 77}, {7, 4, 81}, {7, 8, 77},
};
static const signed char game_bass[16] = { 1, 0, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 1, 0, 0, 0 };

static const signed char win_chords[6] = { 0, 0, 7, 7, 0, 0 };
static const NoteEv win_mel[] = {
    {0, 0, 72}, {0, 2, 76}, {0, 4, 79}, {0, 6, 84}, {0, 8, 79}, {0, 10, 84}, {0, 12, 88},
    {1, 0, 86}, {1, 2, 83}, {1, 4, 79}, {1, 8, 84}, {1, 10, 84}, {1, 12, 86},
    {2, 0, 88}, {2, 0, 84}, {2, 0, 79},
};
static const signed char win_bass[16] = { 1, 0, 0, 0, 8, 0, 0, 0, 1, 0, 0, 0, 13, 0, 0, 0 };

#define NEV(a) ((int)(sizeof(a) / sizeof((a)[0])))
static const Song songs[SONG_COUNT] = {
    { 0 },
    { 112, 8, 1, SMP_STEEL, title_chords, title_mel, NEV(title_mel),
      0x0101, 0xFFFF, 0x4444, 0x8080, 0x0400, 0x4444, title_bass, 0.55f, 0.16f },
    { 92, 8, 1, SMP_MARIMBA, game_chords, game_mel, NEV(game_mel),
      0x0000, 0x5555, 0x4444, 0x0000, 0x4000, 0x1010, game_bass, 0.45f, 0.14f },
    { 132, 3, 0, SMP_STEEL, win_chords, win_mel, NEV(win_mel),
      0x1111, 0xFFFF, 0x4444, 0x0000, 0x0000, 0x0001, win_bass, 0.6f, 0.2f },
};

/* ------------------------------------------------------------------ */
/* Mixer                                                               */
/* ------------------------------------------------------------------ */

typedef struct {
    const Sample *s;
    float pos, step;
    float vl, vr;
    int delay;
    int music;
} Voice;

#define MAX_VOICES 32
static Voice voices[MAX_VOICES];

/* fila de pedidos (produtor: jogo, consumidor: audio_render) */
#define QSIZE 32
static volatile int q_cmd[QSIZE], q_arg[QSIZE];
static volatile unsigned int q_head, q_tail;

static int cur_song = SONG_NONE, music_on = 1, requested_song = SONG_NONE;
static int seq_step_n;
static float seq_acc;
static float music_gain = 1.0f;

static void play(int id, float note, float vol, float pan, int delay, int music)
{
    int i, best = -1;
    Voice *v;
    for (i = 0; i < MAX_VOICES; i++)
        if (!voices[i].s) { best = i; break; }
    if (best < 0) {
        /* rouba a voz mais adiantada */
        float far = -1;
        for (i = 0; i < MAX_VOICES; i++) {
            float p = voices[i].pos / (float)voices[i].s->len;
            if (p > far) { far = p; best = i; }
        }
    }
    v = &voices[best];
    v->s = &smp[id];
    v->pos = 0;
    v->step = powf(2.0f, (note - smp[id].base) / 12.0f);
    v->vl = vol * (pan <= 0 ? 1.0f : 1.0f - pan);
    v->vr = vol * (pan >= 0 ? 1.0f : 1.0f + pan);
    v->delay = delay;
    v->music = music;
}

static void trigger_sfx(int id)
{
    int ms = AUDIO_RATE / 1000;
    switch (id) {
    case SFX_MOVE: play(SMP_BLIP, 60, 0.5f, 0, 0, 0); break;
    case SFX_CONFIRM:
        play(SMP_MARIMBA, 79, 0.7f, -0.1f, 0, 0);
        play(SMP_MARIMBA, 84, 0.7f, 0.1f, 70 * ms, 0);
        break;
    case SFX_BACK:
        play(SMP_MARIMBA, 76, 0.6f, 0, 0, 0);
        play(SMP_MARIMBA, 69, 0.6f, 0, 70 * ms, 0);
        break;
    case SFX_PAUSE:
        play(SMP_MARIMBA, 72, 0.55f, 0, 0, 0);
        play(SMP_MARIMBA, 79, 0.55f, 0, 90 * ms, 0);
        break;
    case SFX_DICE: play(SMP_DICE, 60, 0.5f, 0, 0, 0); break;
    case SFX_DICE_LAND: play(SMP_DICE_LAND, 60, 0.8f, 0, 0, 0); break;
    case SFX_SPLASH: play(SMP_SPLASH, 60 + frand(-1.5f, 1.5f), 0.8f, frand(-0.3f, 0.3f), 0, 0); break;
    case SFX_BOING:
        play(SMP_BOING, 60, 0.75f, 0, 0, 0);
        play(SMP_SPLASH, 64, 0.4f, 0.2f, 0, 0);
        break;
    case SFX_WHISTLE: play(SMP_WHISTLE, 60, 0.6f, 0, 0, 0); break;
    case SFX_CHIME:
        play(SMP_CHIME, 60, 0.6f, 0, 0, 0);
        play(SMP_SPLASH, 66, 0.3f, 0, 0, 0);
        break;
    case SFX_SPARKLE: play(SMP_SPARKLE, 60, 0.65f, 0, 0, 0); break;
    case SFX_POP: play(SMP_POP, 60, 0.45f, 0, 0, 0); break;
    }
}

static void stop_music_voices(void)
{
    int i;
    for (i = 0; i < MAX_VOICES; i++)
        if (voices[i].music) voices[i].s = 0;
}

static void chord_tones(int c, int out[3])
{
    int pc = c % 12, minor = c >= 12;
    out[0] = pc;
    out[1] = pc + (minor ? 3 : 4);
    out[2] = pc + 7;
}

static void seq_step(void)
{
    const Song *s = &songs[cur_song];
    int bar = seq_step_n / 16, st = seq_step_n % 16, i;
    int ch, tones[3];
    unsigned short bit = (unsigned short)(1u << st);

    if (bar >= s->bars) {
        if (!s->loop) {
            cur_song = SONG_NONE;
            return;
        }
        seq_step_n = 0;
        bar = 0;
    }
    ch = s->chords[bar * 2 + (st >= 8)];
    chord_tones(ch, tones);

    for (i = 0; i < s->n_melody; i++)
        if (s->melody[i].bar == bar && s->melody[i].step == st)
            play(s->lead, s->melody[i].note, s->lead_vol, 0.15f, 0, 1);

    if (s->bass[st]) {
        int note = 36 + tones[0] + s->bass[st] - 1;
        play(SMP_BASS, (float)note, 0.7f, 0, 0, 1);
    }
    if (s->comp & bit)
        for (i = 0; i < 3; i++) {
            int n = 60 + tones[i];
            if (n > 71) n -= 12;
            play(SMP_MARIMBA, (float)n, s->comp_vol, -0.35f + 0.35f * i, 0, 1);
        }
    if (s->kick & bit) play(SMP_KICK, 60, 0.6f, 0, 0, 1);
    if (s->shaker & bit) play(SMP_SHAKER, 60, (s->accent & bit) ? 0.12f : 0.05f, 0.4f, 0, 1);
    if (s->bongo_hi & bit) play(SMP_BONGO, 67, 0.35f, -0.4f, 0, 1);
    if (s->bongo_lo & bit) play(SMP_BONGO, 60, 0.35f, -0.4f, 0, 1);
    seq_step_n++;
}

void audio_init(void)
{
    memset(voices, 0, sizeof(voices));
    gen_steel();
    gen_marimba();
    gen_bass();
    gen_shaker();
    gen_kick();
    gen_bongo();
    gen_splash();
    gen_boing();
    gen_whistle();
    gen_dice();
    gen_dice_land();
    gen_blip();
    gen_pop();
    gen_chime();
    gen_sparkle();
}

static void push(int cmd, int arg)
{
    unsigned int h = q_head;
    if (h - q_tail >= QSIZE) return; /* fila cheia: descarta */
    q_cmd[h % QSIZE] = cmd;
    q_arg[h % QSIZE] = arg;
    q_head = h + 1;
}

void audio_sfx(int id) { push(0, id); }

void audio_music(int song)
{
    if (song == requested_song) return;
    requested_song = song;
    push(1, song);
}

void audio_set_music_on(int on)
{
    music_on = on;
    push(2, on);
}

int audio_music_on(void) { return music_on; }

void audio_render(short *out, int frames)
{
    int i, k;
    float spb;
    while (q_tail != q_head) {
        int cmd = q_cmd[q_tail % QSIZE], arg = q_arg[q_tail % QSIZE];
        q_tail++;
        if (cmd == 0) {
            trigger_sfx(arg);
        } else if (cmd == 1) {
            stop_music_voices();
            cur_song = arg;
            seq_step_n = 0;
            seq_acc = 0;
            if (arg == SONG_WIN) trigger_sfx(SFX_SPARKLE);
        } else if (cmd == 2) {
            music_gain = arg ? 1.0f : 0.0f;
        }
    }

    spb = cur_song ? SR * 60.0f / songs[cur_song].bpm / 4.0f : 0;
    for (i = 0; i < frames; i++) {
        float l = 0, r = 0;
        if (cur_song) {
            seq_acc -= 1.0f;
            if (seq_acc <= 0) {
                seq_acc += spb;
                seq_step();
            }
        }
        for (k = 0; k < MAX_VOICES; k++) {
            Voice *v = &voices[k];
            int p;
            float fr, x, g;
            if (!v->s) continue;
            if (v->delay > 0) { v->delay--; continue; }
            p = (int)v->pos;
            if (p + 1 >= v->s->len) { v->s = 0; continue; }
            fr = v->pos - p;
            x = v->s->data[p] + (v->s->data[p + 1] - v->s->data[p]) * fr;
            g = v->music ? music_gain * 0.55f : 1.0f;
            l += x * v->vl * g;
            r += x * v->vr * g;
            v->pos += v->step;
        }
        /* saturacao suave para nunca estourar */
        l /= 32768.0f;
        r /= 32768.0f;
        l = l * (27 + l * l) / (27 + 9 * l * l);
        r = r * (27 + r * r) / (27 + 9 * r * r);
        if (l > 1) l = 1;
        if (l < -1) l = -1;
        if (r > 1) r = 1;
        if (r < -1) r = -1;
        out[2 * i] = (short)(l * 32000.0f);
        out[2 * i + 1] = (short)(r * 32000.0f);
    }
}
