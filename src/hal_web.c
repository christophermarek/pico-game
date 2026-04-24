/*
 * HAL — Emscripten / browser implementation. Renders to a <canvas> via
 * Emscripten's SDL2 port; persistence uses window.localStorage through
 * EM_ASM. Frame pacing is handled by main_web.c (the browser main-loop
 * driver), so hal_show does no blocking delay.
 *
 * Touch controls live entirely in web/shell.html: the HTML layer
 * dispatches synthetic KeyboardEvents on the window, which SDL2 receives
 * just like physical key presses. This keeps hal_web.c free of any
 * touch-specific code.
 */

#include "hal.h"
#include "config.h"

#include <SDL2/SDL.h>
#include <emscripten.h>
#include <emscripten/html5.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define LS_KEY "grumblequest_save_v1"

static SDL_Window   *win;
static SDL_Renderer *rend;
static SDL_Texture  *tex;
static uint16_t      fb[DISPLAY_W * DISPLAY_H];

static bool prev_a, prev_b, prev_start, prev_cam_l, prev_cam_r;

static inline void fb_clip_rect(int *x, int *y, int *w, int *h)
{
    if (*x < 0) { *w += *x; *x = 0; }
    if (*y < 0) { *h += *y; *y = 0; }
    if (*x + *w > DISPLAY_W) *w = DISPLAY_W - *x;
    if (*y + *h > DISPLAY_H) *h = DISPLAY_H - *y;
}

void hal_display_init(void)
{
    /*
     * The canvas logical size is whatever shell.html sets; SDL creates a
     * window that matches. The shell also sets CSS width/height so the
     * canvas stretches to fill its flex slot while the internal 240×240
     * framebuffer stays pixel-perfect via SDL_ScaleModeNearest.
     */
    win = SDL_CreateWindow(
        "GrumbleQuest",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        DISPLAY_W, DISPLAY_H, SDL_WINDOW_SHOWN);
    if (!win) { fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError()); exit(1); }

    rend = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    if (!rend) { fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError()); exit(1); }

    tex = SDL_CreateTexture(rend,
        SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING,
        DISPLAY_W, DISPLAY_H);
    if (!tex) { fprintf(stderr, "SDL_CreateTexture: %s\n", SDL_GetError()); exit(1); }

    SDL_SetTextureScaleMode(tex, SDL_ScaleModeNearest);
    SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);
    memset(fb, 0, sizeof(fb));
}

void hal_fill(uint16_t color)
{
    for (int i = 0; i < DISPLAY_W * DISPLAY_H; i++)
        fb[i] = color;
}

void hal_fill_rect(int x, int y, int w, int h, uint16_t color)
{
    fb_clip_rect(&x, &y, &w, &h);
    if (w <= 0 || h <= 0) return;
    for (int row = y; row < y + h; row++) {
        uint16_t *dst = fb + row * DISPLAY_W + x;
        for (int col = 0; col < w; col++)
            dst[col] = color;
    }
}

void hal_pixel(int x, int y, uint16_t color)
{
    if (x >= 0 && x < DISPLAY_W && y >= 0 && y < DISPLAY_H)
        fb[y * DISPLAY_W + x] = color;
}

void hal_show(void)
{
    SDL_UpdateTexture(tex, NULL, fb, DISPLAY_W * sizeof(uint16_t));
    SDL_RenderClear(rend);
    SDL_RenderCopy(rend, tex, NULL, NULL);
    SDL_RenderPresent(rend);
}

bool hal_image_load_rgba(const char *path, HalImageRGBA *out)
{
    if (!path || !out) return false;
    memset(out, 0, sizeof(*out));
    int w = 0, h = 0;
    unsigned char *pix = stbi_load(path, &w, &h, NULL, 4);
    if (!pix || w <= 0 || h <= 0) {
        if (pix) stbi_image_free(pix);
        return false;
    }
    out->w    = w;
    out->h    = h;
    out->rgba = (uint8_t *)pix;
    return true;
}

void hal_image_free(HalImageRGBA *img)
{
    if (!img) return;
    if (img->rgba) stbi_image_free(img->rgba);
    img->rgba = NULL;
    img->w = img->h = 0;
}

void hal_input_init(void)
{
    prev_a = prev_b = prev_start = false;
    prev_cam_l = prev_cam_r = false;
}

void hal_input_poll(Input *inp)
{
    /*
     * Drain events so SDL's internal keyboard state stays fresh. Ignore
     * SDL_QUIT — in the browser there's no "close window" gesture; the
     * tab itself is the lifecycle.
     */
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) { (void)ev; }

    const uint8_t *k = SDL_GetKeyboardState(NULL);

    inp->up    = k[SDL_SCANCODE_UP]    || k[SDL_SCANCODE_W];
    inp->down  = k[SDL_SCANCODE_DOWN]  || k[SDL_SCANCODE_S];
    inp->left  = k[SDL_SCANCODE_LEFT]  || k[SDL_SCANCODE_Q] || k[SDL_SCANCODE_A];
    inp->right = k[SDL_SCANCODE_RIGHT] || k[SDL_SCANCODE_D];
    inp->a     = k[SDL_SCANCODE_Z]     || k[SDL_SCANCODE_SPACE] ||
                 k[SDL_SCANCODE_RETURN] || k[SDL_SCANCODE_KP_ENTER];
    inp->b     = k[SDL_SCANCODE_X]     || k[SDL_SCANCODE_B];
    inp->start = k[SDL_SCANCODE_M]     || k[SDL_SCANCODE_TAB];
    inp->cam_l = k[SDL_SCANCODE_LEFTBRACKET];
    inp->cam_r = k[SDL_SCANCODE_RIGHTBRACKET];

    inp->a_press     = inp->a     && !prev_a;
    inp->b_press     = inp->b     && !prev_b;
    inp->start_press = inp->start && !prev_start;
    inp->cam_l_press = inp->cam_l && !prev_cam_l;
    inp->cam_r_press = inp->cam_r && !prev_cam_r;

    prev_a     = inp->a;
    prev_b     = inp->b;
    prev_start = inp->start;
    prev_cam_l = inp->cam_l;
    prev_cam_r = inp->cam_r;
}

uint32_t hal_ticks_ms(void) { return SDL_GetTicks(); }

/*
 * Persistence — localStorage keyed by LS_KEY. The save blob is base64-
 * encoded because localStorage stores UTF-16 strings; a raw binary copy
 * would corrupt on non-ASCII bytes. The header ([u32 len][data…]) matches
 * the desktop format so a user could in principle copy saves across.
 */
EM_JS(int, js_save_bytes, (const char *ptr, int len), {
    try {
        const view = HEAPU8.subarray(ptr, ptr + len);
        let bin = '';
        for (let i = 0; i < len; i++) bin += String.fromCharCode(view[i]);
        localStorage.setItem('grumblequest_save_v1', btoa(bin));
        return 1;
    } catch (e) { return 0; }
});

EM_JS(int, js_load_bytes, (char *ptr, int max_len), {
    try {
        const s = localStorage.getItem('grumblequest_save_v1');
        if (!s) return -1;
        const bin = atob(s);
        const n = Math.min(bin.length, max_len);
        for (let i = 0; i < n; i++) HEAPU8[ptr + i] = bin.charCodeAt(i);
        return n;
    } catch (e) { return -1; }
});

bool hal_save(const void *data, size_t len)
{
    uint32_t len32 = (uint32_t)len;
    size_t   total = sizeof(len32) + len;
    uint8_t *buf = (uint8_t *)malloc(total);
    if (!buf) return false;
    memcpy(buf, &len32, sizeof(len32));
    memcpy(buf + sizeof(len32), data, len);
    int ok = js_save_bytes((const char *)buf, (int)total);
    free(buf);
    return ok != 0;
}

bool hal_load(void *data, size_t len)
{
    uint32_t len32;
    size_t   total = sizeof(len32) + len;
    uint8_t *buf = (uint8_t *)malloc(total);
    if (!buf) return false;
    int got = js_load_bytes((char *)buf, (int)total);
    if (got < (int)total) { free(buf); return false; }
    memcpy(&len32, buf, sizeof(len32));
    bool ok = (len32 == (uint32_t)len);
    if (ok) memcpy(data, buf + sizeof(len32), len);
    free(buf);
    return ok;
}

void hal_init(void)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        exit(1);
    }
    hal_display_init();
    hal_input_init();
}

void hal_deinit(void)
{
    if (tex)  SDL_DestroyTexture(tex);
    if (rend) SDL_DestroyRenderer(rend);
    if (win)  SDL_DestroyWindow(win);
    SDL_Quit();
}
