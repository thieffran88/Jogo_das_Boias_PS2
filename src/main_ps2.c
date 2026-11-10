/*
 * Jogo das Boias PS2 - backend do PlayStation 2 (gsKit + libpad).
 */
#include <kernel.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <libpad.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <audsrv.h>
#include <sbv_patches.h>

#include <gsKit.h>
#include <dmaKit.h>

#include "render.h"
#include "atlas.h"
#include "assets.h"
#include "ui.h"
#include "audio.h"

static GSGLOBAL *gs;
static GSTEXTURE atlas_tex;
static GSTEXTURE page_tex[ASSET_NUM_PAGES];
static float sx = 1.0f, sy = 1.0f;

static char pad_buf[2][256] __attribute__((aligned(64)));
static int pad_ok[2];

unsigned int platform_ticks(void)
{
    unsigned int c;
    asm volatile("mfc0 %0, $9" : "=r"(c));
    return c;
}

/* ---- cores: 0xRRGGBBAA -> GS (alfa 0x80 = opaco) ---- */
static u64 gs_color(rgba_t c)
{
    u32 r = (c >> 24) & 255, g = (c >> 16) & 255, b = (c >> 8) & 255, a = c & 255;
    return GS_SETREG_RGBAQ(r, g, b, (a + 1) >> 1, 0x00);
}

/* modulacao de textura: 0x80 = 100% */
static u64 gs_tint(rgba_t c)
{
    u32 r = (c >> 24) & 255, g = (c >> 16) & 255, b = (c >> 8) & 255, a = c & 255;
    return GS_SETREG_RGBAQ((r + 1) >> 1, (g + 1) >> 1, (b + 1) >> 1, (a + 1) >> 1, 0x00);
}

void r_begin(rgba_t clear)
{
    gsKit_clear(gs, gs_color(clear));
}

void r_end(void)
{
    gsKit_queue_exec(gs);
    gsKit_sync_flip(gs);
}

void r_rect(float x, float y, float w, float h, rgba_t c)
{
    if (w <= 0 || h <= 0) return;
    gsKit_prim_sprite(gs, x * sx, y * sy, (x + w) * sx, (y + h) * sy, 1, gs_color(c));
}

void r_rect_grad(float x, float y, float w, float h, rgba_t top, rgba_t bot)
{
    u64 a = gs_color(top), b = gs_color(bot);
    if (w <= 0 || h <= 0) return;
    gsKit_prim_quad_gouraud(gs,
                            x * sx, y * sy,
                            (x + w) * sx, y * sy,
                            x * sx, (y + h) * sy,
                            (x + w) * sx, (y + h) * sy,
                            1, a, a, b, b);
}

void r_atlas(float x, float y, float w, float h,
             float u0, float v0, float u1, float v1, rgba_t c)
{
    if (w <= 0 || h <= 0) return;
    gsKit_prim_sprite_texture(gs, &atlas_tex,
                              x * sx, y * sy, u0, v0,
                              (x + w) * sx, (y + h) * sy, u1, v1,
                              1, gs_tint(c));
}

void r_image(int page, float x, float y, float w, float h,
             float u0, float v0, float u1, float v1, rgba_t c)
{
    if (w <= 0 || h <= 0) return;
    gsKit_prim_sprite_texture(gs, &page_tex[page],
                              x * sx, y * sy, u0, v0,
                              (x + w) * sx, (y + h) * sy, u1, v1,
                              1, gs_tint(c));
}

/* ---- inicializacao ---- */

/* Posicao da entrada i na CLUT de 256 cores (modo CSM1 do GS). */
static int clut_pos(int i)
{
    return (i & 0xE7) | ((i & 0x08) << 1) | ((i & 0x10) >> 1);
}

/* Envia uma textura de 8 bits (PSMT8) com paleta RGBA de 256 cores. */
static int upload_t8(GSTEXTURE *t, const unsigned char *idx, int w, int h,
                     const unsigned char *pal_rgba, int alpha_only)
{
    int i;
    u32 *clut = (u32 *)memalign(64, 256 * 4);
    if (!clut) return -1;
    for (i = 0; i < 256; i++) {
        u32 r, g, b, a;
        if (alpha_only) {
            r = g = b = 255;
            a = i;
        } else {
            r = pal_rgba[i * 4 + 0];
            g = pal_rgba[i * 4 + 1];
            b = pal_rgba[i * 4 + 2];
            a = pal_rgba[i * 4 + 3];
        }
        clut[clut_pos(i)] = (((a + 1) >> 1) << 24) | (b << 16) | (g << 8) | r;
    }
    memset(t, 0, sizeof(*t));
    t->Width = w;
    t->Height = h;
    t->PSM = GS_PSM_T8;
    t->ClutPSM = GS_PSM_CT32;
    t->Filter = GS_FILTER_LINEAR;
    t->Mem = (u32 *)idx;
    t->Clut = clut;
    t->Vram = gsKit_vram_alloc(gs, gsKit_texture_size(w, h, GS_PSM_T8), GSKIT_ALLOC_USERBUFFER);
    t->VramClut = gsKit_vram_alloc(gs, gsKit_texture_size(16, 16, GS_PSM_CT32), GSKIT_ALLOC_USERBUFFER);
    if (t->Vram == GSKIT_ALLOC_ERROR || t->VramClut == GSKIT_ALLOC_ERROR) return -1;
    FlushCache(0);
    gsKit_texture_upload(gs, t);
    return 0;
}

/* ---- audio: thread propria que mistura e envia blocos ao audsrv ---- */

extern unsigned char audsrv_irx[], audsrv_irx_end[];

#define AUDIO_CHUNK_FRAMES 512   /* 2 KB = um "feed" do audsrv a 48 kHz */
static short audio_buf[AUDIO_CHUNK_FRAMES * 2] __attribute__((aligned(64)));
static unsigned char audio_stack[16 * 1024] __attribute__((aligned(16)));

static void audio_thread(void *arg)
{
    (void)arg;
    for (;;) {
        audio_render(audio_buf, AUDIO_CHUNK_FRAMES);
        /* bloqueia (dormindo) ate caber no buffer de ~100 ms do audsrv */
        audsrv_wait_audio(sizeof(audio_buf));
        audsrv_play_audio((const char *)audio_buf, sizeof(audio_buf));
    }
}

static void init_audio(void)
{
    int ret = 0, id;
    struct audsrv_fmt_t fmt;
    ee_thread_t th;

    audio_init(); /* sintetiza os instrumentos e efeitos */

    if (SifLoadModule("rom0:LIBSD", 0, NULL) < 0) return;
    if (SifExecModuleBuffer(audsrv_irx, audsrv_irx_end - audsrv_irx, 0, NULL, &ret) < 0) return;
    if (audsrv_init() != 0) return;

    fmt.bits = 16;
    fmt.freq = AUDIO_RATE;
    fmt.channels = 2;
    if (audsrv_set_format(&fmt) != AUDSRV_ERR_NOERROR) return;
    audsrv_set_volume(MAX_VOLUME);

    /* a thread de audio tem prioridade maior que a principal */
    ChangeThreadPriority(GetThreadId(), 64);
    memset(&th, 0, sizeof(th));
    th.func = (void *)audio_thread;
    th.stack = audio_stack;
    th.stack_size = sizeof(audio_stack);
    th.gp_reg = &_gp;
    th.initial_priority = 40;
    id = CreateThread(&th);
    if (id >= 0) StartThread(id, NULL);
}

static void load_modules(void)
{
    int ret;
    SifInitRpc(0);
    /* permite carregar modulos do IOP a partir da memoria do EE */
    sbv_patch_enable_lmb();
    sbv_patch_disable_prefix_check();
    ret = SifLoadModule("rom0:SIO2MAN", 0, NULL);
    if (ret < 0) SleepThread();
    ret = SifLoadModule("rom0:PADMAN", 0, NULL);
    if (ret < 0) SleepThread();
}

static void init_pads(void)
{
    int port;
    padInit(0);
    for (port = 0; port < 2; port++)
        pad_ok[port] = padPortOpen(port, 0, pad_buf[port]) != 0;
}

static unsigned int read_pads(void)
{
    static unsigned int prev = 0;
    unsigned int now = 0, pressed;
    int port;
    for (port = 0; port < 2; port++) {
        struct padButtonStatus st;
        int state;
        unsigned short btn;
        if (!pad_ok[port]) continue;
        state = padGetState(port, 0);
        if (state != PAD_STATE_STABLE && state != PAD_STATE_FINDCTP1) continue;
        if (padRead(port, 0, &st) == 0) continue;
        btn = 0xffff ^ st.btns;
        if (btn & PAD_UP) now |= BTN_UP;
        if (btn & PAD_DOWN) now |= BTN_DOWN;
        if (btn & PAD_LEFT) now |= BTN_LEFT;
        if (btn & PAD_RIGHT) now |= BTN_RIGHT;
        if (btn & PAD_CROSS) now |= BTN_CROSS;
        if (btn & PAD_CIRCLE) now |= BTN_CIRCLE;
        if (btn & PAD_TRIANGLE) now |= BTN_TRIANGLE;
        if (btn & PAD_SQUARE) now |= BTN_SQUARE;
        if (btn & PAD_START) now |= BTN_START;
        if (btn & PAD_SELECT) now |= BTN_SELECT;
    }
    pressed = now & ~prev;
    prev = now;
    return pressed;
}

static void init_gfx(void)
{
    int i;

    gs = gsKit_init_global();
    gs->PSM = GS_PSM_CT24;
    gs->PSMZ = GS_PSMZ_16S;
    gs->ZBuffering = GS_SETTING_OFF;
    gs->DoubleBuffering = GS_SETTING_ON;
    gs->PrimAlphaEnable = GS_SETTING_ON;

    dmaKit_init(D_CTRL_RELE_OFF, D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC,
                D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);
    dmaKit_chan_init(DMA_CHANNEL_GIF);

    gsKit_init_screen(gs);
    gsKit_mode_switch(gs, GS_ONESHOT);
    gsKit_set_primalpha(gs, GS_SETREG_ALPHA(0, 1, 0, 1, 0), 0);
    gsKit_set_test(gs, GS_ZTEST_OFF);

    sx = gs->Width / (float)SCREEN_W;
    sy = gs->Height / (float)SCREEN_H;

    /* atlas da fonte/formas: indice = alfa */
    {
        unsigned char *buf = (unsigned char *)memalign(128, ATLAS_W * ATLAS_H_PX);
        memcpy(buf, atlas_alpha, ATLAS_W * ATLAS_H_PX);
        upload_t8(&atlas_tex, buf, ATLAS_W, ATLAS_H_PX, NULL, 1);
    }
    /* paginas de sprites */
    for (i = 0; i < ASSET_NUM_PAGES; i++)
        upload_t8(&page_tex[i], assets_bin + asset_pages[i].idx_off,
                  ASSET_PAGE_SIZE, asset_pages[i].h,
                  assets_bin + asset_pages[i].pal_off, 0);
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    load_modules();
    init_pads();
    init_gfx();
    init_audio();
    ui_init();

    for (;;) {
        unsigned int pressed = read_pads();
        ui_update(pressed);
        r_begin(RGBA(0, 0, 0, 255));
        ui_draw();
        r_end();
    }
    return 0;
}
