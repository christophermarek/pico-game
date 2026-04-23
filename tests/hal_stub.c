/* Minimal HAL for grumblequest_tests — no SDL, no hardware; save + framebuffer in RAM. */

#include "hal_stub.h"
#include "hal.h"
#include "config.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <string.h>
#include <stdlib.h>

#define SAVE_BUF_MAX (256 * 1024u)

static uint16_t fb[DISPLAY_W * DISPLAY_H];
static uint32_t ticks_ms;
static uint8_t  save_buf[SAVE_BUF_MAX];
static size_t   save_len;
static bool     save_valid;

void hal_stub_reset(void)
{
    memset(fb, 0, sizeof(fb));
    ticks_ms   = 0;
    save_len   = 0;
    save_valid = false;
}

void hal_display_init(void)
{
    memset(fb, 0, sizeof(fb));
}

void hal_fill(uint16_t color)
{
    for (int i = 0; i < DISPLAY_W * DISPLAY_H; i++)
        fb[i] = color;
}

void hal_fill_rect(int x, int y, int w, int h, uint16_t color)
{
    if (w <= 0 || h <= 0) return;
    for (int row = y; row < y + h; row++) {
        if (row < 0 || row >= DISPLAY_H) continue;
        for (int col = x; col < x + w; col++) {
            if (col < 0 || col >= DISPLAY_W) continue;
            fb[row * DISPLAY_W + col] = color;
        }
    }
}

void hal_pixel(int x, int y, uint16_t color)
{
    if (x >= 0 && x < DISPLAY_W && y >= 0 && y < DISPLAY_H)
        fb[y * DISPLAY_W + x] = color;
}

void hal_show(void) { }

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

void hal_input_init(void) { }

void hal_input_poll(Input *inp)
{
    memset(inp, 0, sizeof(*inp));
}

uint32_t hal_ticks_ms(void) { return ticks_ms; }

bool hal_save(const void *data, size_t len)
{
    if (len > SAVE_BUF_MAX) return false;
    memcpy(save_buf, data, len);
    save_len   = len;
    save_valid = true;
    return true;
}

bool hal_load(void *data, size_t len)
{
    if (!save_valid || len != save_len) return false;
    memcpy(data, save_buf, len);
    return true;
}

void hal_init(void)
{
    hal_stub_reset();
    hal_display_init();
    hal_input_init();
}

void hal_deinit(void) { }
