/*
 * HAL implementation — SDL2 desktop simulator.
 * Compiled when building the grumblequest_sim target.
 *
 * The 240×240 game surface is scaled up SIM_SCALE× in the window.
 * Colors stay in RGB565 internally; we convert on hal_show().
 */

#include "hal.h"
#include "config.h"

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

/* ---- SDL state ---- */
static SDL_Window   *win;
static SDL_Renderer *rend;
static SDL_Texture  *tex;           /* 240×240 streaming RGB565 texture */
static uint16_t      fb[DISPLAY_W * DISPLAY_H];  /* CPU-side framebuffer */

/* ---- Input edge-detection ---- */
static bool prev_a, prev_b, prev_start, prev_sel, prev_cam_l, prev_cam_r;

/* ---- Frame timing ---- */
static uint32_t frame_start_ms;

/* ------------------------------------------------------------------ */

static inline void fb_clip_rect(int *x, int *y, int *w, int *h)
{
    if (*x < 0) { *w += *x; *x = 0; }
    if (*y < 0) { *h += *y; *y = 0; }
    if (*x + *w > DISPLAY_W) *w = DISPLAY_W - *x;
    if (*y + *h > DISPLAY_H) *h = DISPLAY_H - *y;
}

/* ================================================================== */
/* Display                                                             */
/* ================================================================== */

void hal_display_init(void)
{
    /* Window size: game pixels scaled up, plus a small debug strip */
    int win_w = DISPLAY_W * SIM_SCALE;
    int win_h = DISPLAY_H * SIM_SCALE + 24;

    win = SDL_CreateWindow(
        "GrumbleQuest [SIM]  Move: Arrows/WSQD  Tab: Menu/Tabs  A: confirm  B: back  Cam: [ ]",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        win_w, win_h,
        SDL_WINDOW_SHOWN
    );
    if (!win) { fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError()); exit(1); }

    rend = SDL_CreateRenderer(win, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!rend) { fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError()); exit(1); }

    /* RGB565 texture — matches ST7789 native format exactly */
    tex = SDL_CreateTexture(rend,
        SDL_PIXELFORMAT_RGB565,
        SDL_TEXTUREACCESS_STREAMING,
        DISPLAY_W, DISPLAY_H);
    if (!tex) { fprintf(stderr, "SDL_CreateTexture: %s\n", SDL_GetError()); exit(1); }

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

void hal_blit(int x, int y, int w, int h, const uint16_t *buf)
{
    for (int row = 0; row < h; row++) {
        int dy = y + row;
        if (dy < 0 || dy >= DISPLAY_H) continue;
        for (int col = 0; col < w; col++) {
            int dx = x + col;
            if (dx < 0 || dx >= DISPLAY_W) continue;
            fb[dy * DISPLAY_W + dx] = buf[row * w + col];
        }
    }
}

void hal_show(void)
{
    /* Push framebuffer to texture (RGB565 → RGB565, no conversion needed) */
    SDL_UpdateTexture(tex, NULL, fb, DISPLAY_W * sizeof(uint16_t));

    SDL_RenderClear(rend);

    /* Scale to window (nearest-neighbour — keep pixel-art crisp) */
    SDL_Rect dst = { 0, 0, DISPLAY_W * SIM_SCALE, DISPLAY_H * SIM_SCALE };
    SDL_SetTextureScaleMode(tex, SDL_ScaleModeNearest);
    SDL_RenderCopy(rend, tex, NULL, &dst);

    /* Debug strip — dark bar at bottom with SIM label */
    SDL_SetRenderDrawColor(rend, 10, 5, 20, 255);
    SDL_Rect strip = { 0, DISPLAY_H * SIM_SCALE, DISPLAY_W * SIM_SCALE, 24 };
    SDL_RenderFillRect(rend, &strip);

    SDL_RenderPresent(rend);

    /* Frame-rate cap — sleep for remaining time in the 33ms budget */
    uint32_t elapsed = SDL_GetTicks() - frame_start_ms;
    if (elapsed < (uint32_t)FRAME_MS)
        SDL_Delay(FRAME_MS - elapsed);
    frame_start_ms = SDL_GetTicks();
}

bool hal_image_load_rgba(const char *path, HalImageRGBA *out)
{
    if (!path || !out)
        return false;
    memset(out, 0, sizeof(*out));
    int w = 0, h = 0;
    unsigned char *pix = stbi_load(path, &w, &h, NULL, 4);
    if (!pix || w <= 0 || h <= 0) {
        if (pix)
            stbi_image_free(pix);
        return false;
    }
    out->w    = w;
    out->h    = h;
    out->rgba = (uint8_t *)pix;
    return true;
}

void hal_image_free(HalImageRGBA *img)
{
    if (!img)
        return;
    if (img->rgba)
        stbi_image_free(img->rgba);
    img->rgba = NULL;
    img->w = 0;
    img->h = 0;
}

/* ================================================================== */
/* Input                                                               */
/* ================================================================== */

void hal_input_init(void)
{
    prev_a = prev_b = prev_start = prev_sel = false;
    prev_cam_l = prev_cam_r = false;
}

void hal_input_poll(Input *inp)
{
    /* Process SDL event queue — needed for window close and quit */
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_QUIT) {
            hal_deinit();
            exit(0);
        }
    }

    const uint8_t *k = SDL_GetKeyboardState(NULL);

    /*
     * Desktop layout (avoid WASD vs. face-button "A" clash):
     *   Move: arrows or W S Q D  (Q/D strafe — keyboard A is the A face button)
     *   A:    Z Space Return KeypadEnter A
     *   B:    X B
     */
    inp->up    = k[SDL_SCANCODE_UP]    || k[SDL_SCANCODE_W];
    inp->down  = k[SDL_SCANCODE_DOWN]  || k[SDL_SCANCODE_S];
    inp->left  = k[SDL_SCANCODE_LEFT]  || k[SDL_SCANCODE_Q];
    inp->right = k[SDL_SCANCODE_RIGHT] || k[SDL_SCANCODE_D];
    inp->a     = k[SDL_SCANCODE_Z]     || k[SDL_SCANCODE_SPACE] ||
                 k[SDL_SCANCODE_RETURN] || k[SDL_SCANCODE_KP_ENTER] ||
                 k[SDL_SCANCODE_A];
    inp->b     = k[SDL_SCANCODE_X]     || k[SDL_SCANCODE_B];
    inp->start = k[SDL_SCANCODE_M] || k[SDL_SCANCODE_TAB];
    inp->sel   = k[SDL_SCANCODE_V];
    inp->cam_l = k[SDL_SCANCODE_LEFTBRACKET];
    inp->cam_r = k[SDL_SCANCODE_RIGHTBRACKET];

    /* Edge-triggered press (one frame only) */
    inp->a_press     = inp->a     && !prev_a;
    inp->b_press     = inp->b     && !prev_b;
    inp->start_press = inp->start && !prev_start;
    inp->sel_press   = inp->sel   && !prev_sel;
    inp->cam_l_press = inp->cam_l && !prev_cam_l;
    inp->cam_r_press = inp->cam_r && !prev_cam_r;

    prev_a     = inp->a;
    prev_b     = inp->b;
    prev_start = inp->start;
    prev_sel   = inp->sel;
    prev_cam_l = inp->cam_l;
    prev_cam_r = inp->cam_r;
}

/* ================================================================== */
/* Time                                                                */
/* ================================================================== */

uint32_t hal_ticks_ms(void)
{
    return SDL_GetTicks();
}

void hal_sleep_ms(uint32_t ms)
{
    SDL_Delay(ms);
}

/* ================================================================== */
/* Persistent storage — writes a flat binary file                      */
/* ================================================================== */

bool hal_save(const void *data, size_t len)
{
    FILE *f = fopen(SAVE_PATH, "wb");
    if (!f) return false;
    size_t written = fwrite(data, 1, len, f);
    fclose(f);
    return written == len;
}

bool hal_load(void *data, size_t len)
{
    FILE *f = fopen(SAVE_PATH, "rb");
    if (!f) return false;
    size_t got = fread(data, 1, len, f);
    fclose(f);
    return got == len;
}

/* ================================================================== */
/* Lifecycle                                                           */
/* ================================================================== */

void hal_init(void)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        exit(1);
    }
    hal_display_init();
    hal_input_init();
    frame_start_ms = SDL_GetTicks();
}

void hal_deinit(void)
{
    if (tex)  SDL_DestroyTexture(tex);
    if (rend) SDL_DestroyRenderer(rend);
    if (win)  SDL_DestroyWindow(win);
    SDL_Quit();
}
