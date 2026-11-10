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

#include <gsKit.h>
#include <dmaKit.h>

#include "render.h"
#include "atlas.h"
#include "ui.h"

static GSGLOBAL *gs;
static GSTEXTURE atlas_tex;
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

/* ---- inicializacao ---- */

static void load_modules(void)
{
    int ret;
    SifInitRpc(0);
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
    u32 *px;

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

    /* atlas: branco com alfa (0..0x80) */
    memset(&atlas_tex, 0, sizeof(atlas_tex));
    atlas_tex.Width = ATLAS_W;
    atlas_tex.Height = ATLAS_H_PX;
    atlas_tex.PSM = GS_PSM_CT32;
    atlas_tex.Filter = GS_FILTER_LINEAR;
    atlas_tex.Mem = (u32 *)memalign(128, ATLAS_W * ATLAS_H_PX * 4);
    px = atlas_tex.Mem;
    for (i = 0; i < ATLAS_W * ATLAS_H_PX; i++) {
        u32 a = (atlas_alpha[i] + 1) >> 1;
        px[i] = (a << 24) | 0x00FFFFFF;
    }
    FlushCache(0);
    atlas_tex.Vram = gsKit_vram_alloc(gs,
        gsKit_texture_size(ATLAS_W, ATLAS_H_PX, GS_PSM_CT32), GSKIT_ALLOC_USERBUFFER);
    gsKit_texture_upload(gs, &atlas_tex);
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    load_modules();
    init_pads();
    init_gfx();
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
