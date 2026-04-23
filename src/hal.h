#pragma once
/*
 * Hardware Abstraction Layer — the only interface game code calls.
 * Implementations: hal_sdl.c (desktop sim), hal_pico.c (ST7789 on RP2040),
 * hal_stub.c (headless test harness).
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    int w, h;
    uint8_t *rgba;  /* w*h*4 bytes, RGBA8888 */
} HalImageRGBA;

/* Display — 240×240 RGB565 framebuffer */
void hal_display_init(void);
void hal_fill(uint16_t color);
void hal_fill_rect(int x, int y, int w, int h, uint16_t color);
void hal_pixel(int x, int y, uint16_t color);
void hal_show(void);

/* PNG loader (desktop/test via stb_image; Pico returns flash-embedded atlases). */
bool hal_image_load_rgba(const char *path, HalImageRGBA *out);
void hal_image_free(HalImageRGBA *img);

/* Input — 8 buttons + edge-triggered press flags. */
typedef struct {
    bool up, down, left, right;
    bool a, b, start, sel;
    bool cam_l, cam_r;

    bool a_press, b_press, start_press, sel_press;
    bool cam_l_press, cam_r_press;
} Input;

void hal_input_init(void);
void hal_input_poll(Input *inp);

/* Time — ms since boot (wraps at ~49 days). */
uint32_t hal_ticks_ms(void);

/* Persistent storage — flat byte buffer, file on desktop / last flash sector on Pico. */
bool hal_save(const void *data, size_t len);
bool hal_load(void *data, size_t len);

/* Lifecycle — hal_init sets up display+input+time; hal_deinit tears down. */
void hal_init(void);
void hal_deinit(void);
