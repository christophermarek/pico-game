#pragma once
#include <stdint.h>

/* RGB565 palette (ST7789 native). Layout: RRRRR GGGGGG BBBBB. */
#define RGB565(r, g, b) \
    ((uint16_t)((((r) & 0xF8u) << 8) | (((g) & 0xFCu) << 3) | ((b) >> 3)))

#define HEX(h) RGB565(((h) >> 16) & 0xFF, ((h) >> 8) & 0xFF, (h) & 0xFF)

/* UI */
#define C_BG           HEX(0x0d0d1a)
#define C_BG_DARK      HEX(0x050510)
#define C_PANEL        HEX(0x1a1030)
#define C_BORDER       HEX(0x3730a3)
#define C_BORDER_ACT   HEX(0xfbbf24)
#define C_TEXT_MAIN    HEX(0xc084fc)
#define C_TEXT_DIM     HEX(0x6b7280)
#define C_TEXT_WHITE   HEX(0xffffff)
#define C_GOLD         HEX(0xfbbf24)
#define C_HP_GREEN     HEX(0x22c55e)
#define C_ENERGY_BLUE  HEX(0x3b82f6)
#define C_XP_PURPLE    HEX(0x7c3aed)
#define C_WARN_RED     HEX(0xef4444)

/* Background */
#define C_SKY_BOT      HEX(0x4c1d95)

#define C_BLACK        ((uint16_t)0x0000)
#define C_WHITE        ((uint16_t)0xFFFF)
