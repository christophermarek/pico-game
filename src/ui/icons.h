#pragma once
#include <stdint.h>
#include "hal.h"

/*
 * 8x8 monochrome bitmaps, 1 bit per pixel, MSB-first.
 * draw_icon() renders them with a 1px dark drop shadow for legibility
 * over the world, then the icon in the given color.
 */

static const uint8_t ICON_HEART[8] = {
    0b01101100,
    0b11111110,
    0b11111110,
    0b11111110,
    0b01111100,
    0b00111000,
    0b00010000,
    0b00000000,
};

static const uint8_t ICON_BOLT[8] = {
    0b00011000,
    0b00111000,
    0b01111000,
    0b11111100,
    0b00111000,
    0b01110000,
    0b11100000,
    0b00000000,
};

static inline void draw_icon(const uint8_t *bitmap, int x, int y, uint16_t color) {
    /* Drop shadow */
    for (int row = 0; row < 8; row++) {
        uint8_t b = bitmap[row];
        for (int col = 0; col < 8; col++) {
            if (b & (0x80u >> col))
                hal_fill_rect(x + col + 1, y + row + 1, 1, 1, 0x0000);
        }
    }
    /* Icon */
    for (int row = 0; row < 8; row++) {
        uint8_t b = bitmap[row];
        for (int col = 0; col < 8; col++) {
            if (b & (0x80u >> col))
                hal_fill_rect(x + col, y + row, 1, 1, color);
        }
    }
}
