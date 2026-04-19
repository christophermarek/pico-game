#pragma once
/*
 * Hardware Abstraction Layer
 *
 * This is the ONLY interface the game code ever calls.
 * Two implementations exist:
 *   hal_sdl.c  — SDL2 desktop simulator (build target: grumblequest_sim)
 *   hal_pico.c — Raspberry Pi Pico + ST7789 SPI display (build target: grumblequest)
 *
 * Adding a new platform means writing one new hal_*.c — zero game code changes.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    int w, h;
    uint8_t *rgba; /* owned heap buffer: w*h*4 bytes, RGBA8888 */
} HalImageRGBA;

/* ====================================================================
 * Display  (240×240 RGB565 framebuffer abstraction)
 * ==================================================================== */

/* Initialise display hardware / SDL window. Call once at startup. */
void hal_display_init(void);

/* Fill entire screen with one colour. */
void hal_fill(uint16_t color);

/* Fill a rectangle. Clipped to display bounds. */
void hal_fill_rect(int x, int y, int w, int h, uint16_t color);

/* Set a single pixel. Clipped. */
void hal_pixel(int x, int y, uint16_t color);

/*
 * Push a raw RGB565 framebuffer region.
 * buf: array of w*h uint16_t values, row-major, top-left first.
 * On Pico this maps directly to SPI DMA; on SDL it updates the texture.
 */
void hal_blit(int x, int y, int w, int h, const uint16_t *buf);

/*
 * Flush the current frame to the screen.
 * On Pico: no-op if using DMA auto-flush; or trigger SPI burst.
 * On SDL : SDL_RenderPresent + frame-rate cap.
 */
void hal_show(void);

/* Load a PNG file as RGBA8888 (desktop sim). Returns false on failure. */
bool hal_image_load_rgba(const char *path, HalImageRGBA *out);
void hal_image_free(HalImageRGBA *img);


/* ====================================================================
 * Input
 * ==================================================================== */

typedef struct {
    /* Held state (true while button is down) */
    bool up, down, left, right;
    bool a, b, start, sel;
    /* Camera snap (90° × 4) — desktop: [ ] ; Pico: optional GPIO later */
    bool cam_l, cam_r;

    /* Edge-triggered: true for exactly ONE frame on press */
    bool a_press, b_press, start_press, sel_press;
    bool cam_l_press, cam_r_press;
} Input;

void hal_input_init(void);
void hal_input_poll(Input *inp);   /* call once per frame */


/* ====================================================================
 * Time
 * ==================================================================== */

/* Milliseconds since boot. Wraps at ~49 days — fine for a handheld. */
uint32_t hal_ticks_ms(void);

/* Sleep without burning CPU (SDL_Delay / sleep_ms on Pico). */
void hal_sleep_ms(uint32_t ms);


/* ====================================================================
 * Persistent storage
 * ==================================================================== */

/*
 * Write `len` bytes from `data` to persistent storage.
 * On Pico: writes to flash (last sector).
 * On SDL : writes to SAVE_PATH file.
 * Returns true on success.
 */
bool hal_save(const void *data, size_t len);

/*
 * Read `len` bytes into `data` from persistent storage.
 * Returns true if valid save data was found.
 */
bool hal_load(void *data, size_t len);


/* ====================================================================
 * Lifecycle
 * ==================================================================== */

/* Call once at program start. Inits display + input + time. */
void hal_init(void);

/* Call on exit (SDL_Quit etc.). No-op on Pico. */
void hal_deinit(void);
