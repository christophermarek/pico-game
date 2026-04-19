#pragma once
/*
 * Sprite atlas — a PNG-backed RGB565 image held in memory, blitted per-cell
 * into the framebuffer via the HAL. Layout is authored by tools/bake_sprites.
 *
 * Atlas load order (first one that succeeds wins):
 *   1) path passed explicitly to atlas_load_png()
 *   2) "assets/sprites.png" relative to the current working directory
 *
 * If no atlas is loaded, atlas_is_loaded() returns false and callers should
 * fall back to the procedural renderers in sprites.c.
 *
 * Transparent cells use an explicit color key (C_TRANSPARENT_KEY below)
 * because we're targeting RGB565 on the Pico — no per-pixel alpha channel.
 */

#include <stdbool.h>
#include <stdint.h>

#include "colors.h"

/* Reserve a color that never appears in the game palette as the key for
 * transparent atlas pixels. Pure magenta (R=0xFF, G=0, B=0xFF). */
#define C_TRANSPARENT_KEY RGB565(0xFF, 0x00, 0xFF)

bool atlas_load_png(const char *path);
bool atlas_is_loaded(void);
void atlas_free(void);

/* Blit a source rect from the atlas to the framebuffer. Transparent pixels
 * (matching C_TRANSPARENT_KEY) are skipped. */
void atlas_blit(int src_x, int src_y, int src_w, int src_h,
                int dst_x, int dst_y);

/* Same as atlas_blit but without transparency — faster for opaque sprites. */
void atlas_blit_opaque(int src_x, int src_y, int src_w, int src_h,
                       int dst_x, int dst_y);

/* Report atlas dimensions (0,0 if nothing is loaded). */
void atlas_size(int *w, int *h);
