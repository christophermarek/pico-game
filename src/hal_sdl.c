/* HAL — SDL2 desktop simulator. 240×240 game surface scaled SIM_SCALE× in the window. */

#include "hal.h"
#include "config.h"

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static SDL_Window   *win;
static SDL_Renderer *rend;
static SDL_Texture  *tex;
static uint16_t      fb[DISPLAY_W * DISPLAY_H];

static bool prev_a, prev_b, prev_start, prev_sel, prev_cam_l, prev_cam_r;
static uint32_t frame_start_ms;

static inline void fb_clip_rect(int *x, int *y, int *w, int *h)
{
    if (*x < 0) { *w += *x; *x = 0; }
    if (*y < 0) { *h += *y; *y = 0; }
    if (*x + *w > DISPLAY_W) *w = DISPLAY_W - *x;
    if (*y + *h > DISPLAY_H) *h = DISPLAY_H - *y;
}

void hal_display_init(void)
{
    int win_w = DISPLAY_W * SIM_SCALE;
    int win_h = DISPLAY_H * SIM_SCALE + 24;

    win = SDL_CreateWindow(
        "GrumbleQuest [SIM]  Move: Arrows/WSQD  Tab: Menu/Tabs  A: confirm  B: back  Cam: [ ]",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        win_w, win_h, SDL_WINDOW_SHOWN);
    if (!win) { fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError()); exit(1); }

    /*
     * No PRESENTVSYNC — we pace manually below. Doubling up makes frame
     * times vary by one refresh cycle (33ms vs 50ms on 60 Hz), which the
     * player sees as jitter, most obvious on diagonal motion where the
     * per-axis step is already sub-pixel.
     */
    rend = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    if (!rend) { fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError()); exit(1); }

    tex = SDL_CreateTexture(rend,
        SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING,
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

void hal_show(void)
{
    SDL_UpdateTexture(tex, NULL, fb, DISPLAY_W * sizeof(uint16_t));
    SDL_RenderClear(rend);

    SDL_Rect dst = { 0, 0, DISPLAY_W * SIM_SCALE, DISPLAY_H * SIM_SCALE };
    SDL_SetTextureScaleMode(tex, SDL_ScaleModeNearest);
    SDL_RenderCopy(rend, tex, NULL, &dst);

    SDL_SetRenderDrawColor(rend, 10, 5, 20, 255);
    SDL_Rect strip = { 0, DISPLAY_H * SIM_SCALE, DISPLAY_W * SIM_SCALE, 24 };
    SDL_RenderFillRect(rend, &strip);

    SDL_RenderPresent(rend);

    uint32_t elapsed = SDL_GetTicks() - frame_start_ms;
    if (elapsed < (uint32_t)FRAME_MS)
        SDL_Delay(FRAME_MS - elapsed);
    frame_start_ms = SDL_GetTicks();
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
    prev_a = prev_b = prev_start = prev_sel = false;
    prev_cam_l = prev_cam_r = false;
}

void hal_input_poll(Input *inp)
{
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_QUIT) {
            hal_deinit();
            exit(0);
        }
    }

    const uint8_t *k = SDL_GetKeyboardState(NULL);

    /*
     * Desktop layout: keyboard A is the A face button (confirm), so Q strafes
     * left instead of a WASD-style A.
     */
    inp->up    = k[SDL_SCANCODE_UP]    || k[SDL_SCANCODE_W];
    inp->down  = k[SDL_SCANCODE_DOWN]  || k[SDL_SCANCODE_S];
    inp->left  = k[SDL_SCANCODE_LEFT]  || k[SDL_SCANCODE_Q];
    inp->right = k[SDL_SCANCODE_RIGHT] || k[SDL_SCANCODE_D];
    inp->a     = k[SDL_SCANCODE_Z]     || k[SDL_SCANCODE_SPACE] ||
                 k[SDL_SCANCODE_RETURN] || k[SDL_SCANCODE_KP_ENTER] ||
                 k[SDL_SCANCODE_A];
    inp->b     = k[SDL_SCANCODE_X]     || k[SDL_SCANCODE_B];
    inp->start = k[SDL_SCANCODE_M]     || k[SDL_SCANCODE_TAB];
    inp->sel   = k[SDL_SCANCODE_V];
    inp->cam_l = k[SDL_SCANCODE_LEFTBRACKET];
    inp->cam_r = k[SDL_SCANCODE_RIGHTBRACKET];

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

uint32_t hal_ticks_ms(void) { return SDL_GetTicks(); }

/*
 * On-disk format: [uint32_t len][data…]. hal_load fails fast if the length
 * header doesn't match the caller's expected size, catching stale saves
 * from before a GameState layout change on top of the magic/version check.
 */
bool hal_save(const void *data, size_t len)
{
    FILE *f = fopen(SAVE_PATH, "wb");
    if (!f) return false;
    uint32_t len32 = (uint32_t)len;
    bool ok = fwrite(&len32, sizeof(len32), 1, f) == 1
           && fwrite(data, 1, len, f) == len;
    fclose(f);
    return ok;
}

bool hal_load(void *data, size_t len)
{
    FILE *f = fopen(SAVE_PATH, "rb");
    if (!f) return false;
    uint32_t saved_len;
    bool ok = fread(&saved_len, sizeof(saved_len), 1, f) == 1
           && saved_len == (uint32_t)len
           && fread(data, 1, len, f) == len;
    fclose(f);
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
    frame_start_ms = SDL_GetTicks();
}

void hal_deinit(void)
{
    if (tex)  SDL_DestroyTexture(tex);
    if (rend) SDL_DestroyRenderer(rend);
    if (win)  SDL_DestroyWindow(win);
    SDL_Quit();
}
