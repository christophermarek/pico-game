#pragma once
#include <stdint.h>

/*
 * RGB565 color palette — matches the ST7789 native format.
 * Macro expands at compile time to a constant uint16_t.
 *
 * Bit layout: RRRRRGGGGGGBBBBB
 *   bits 15-11 = red (5 bits)
 *   bits 10-5  = green (6 bits)
 *   bits  4-0  = blue (5 bits)
 */
#define RGB565(r, g, b) \
    ((uint16_t)((((r) & 0xF8u) << 8) | (((g) & 0xFCu) << 3) | ((b) >> 3)))

/* Convert 0xRRGGBB hex literal to RGB565 */
#define HEX(h) RGB565(((h) >> 16) & 0xFF, ((h) >> 8) & 0xFF, (h) & 0xFF)

/* ---- World / terrain ---- */
#define C_GRASS_DARK   HEX(0x2d5a2d)
#define C_GRASS_LIGHT  HEX(0x3a7a3a)
#define C_PATH_DARK    HEX(0x8b6f47)
#define C_PATH_LIGHT   HEX(0xa0845a)
#define C_WATER_DARK   HEX(0x2a5a8a)
#define C_WATER_MID    HEX(0x3a7cb5)
#define C_WATER_LIGHT  HEX(0x5ca0d8)
#define C_SAND         HEX(0xc9a876)
#define C_SAND_DARK    HEX(0xb39060)
#define C_TREE_TRUNK   HEX(0x5c3a1e)
#define C_TREE_DARK    HEX(0x1a4d1a)
#define C_TREE_MID     HEX(0x2d7a2d)
#define C_TREE_LIGHT   HEX(0x4a9a4a)
#define C_ROCK_DARK    HEX(0x4b5563)
#define C_ROCK_MID     HEX(0x6b7280)
#define C_ROCK_LIGHT   HEX(0x9ca3af)
#define C_ORE_VEIN     HEX(0xfbbf24)
#define C_ORE_SHINE    HEX(0xfde68a)
#define C_FLOWER_PINK  HEX(0xec4899)
#define C_TALL_GRASS   HEX(0x1a6b1a)

/* ---- Player ---- */
#define C_PLAYER_BODY  HEX(0x7c3aed)
#define C_PLAYER_DARK  HEX(0x6d28d9)
#define C_PLAYER_LEGS  HEX(0x3730a3)
/* ---- Monsters ---- */
#define C_MON_PURPLE_D HEX(0x6b21a8)
#define C_MON_PURPLE_M HEX(0xa21caf)
#define C_MON_PURPLE_B HEX(0x7c3aed)
#define C_MON_RED      HEX(0xdc2626)
#define C_MON_RED_L    HEX(0xef4444)
#define C_MON_DARK     HEX(0x1e1b4b)
#define C_MON_CROWN    HEX(0xfbbf24)

/* ---- HUD / UI ---- */
#define C_BG           HEX(0x0d0d1a)
#define C_BG_DARK      HEX(0x050510)
#define C_PANEL        HEX(0x1a1030)
#define C_BORDER       HEX(0x3730a3)
#define C_BORDER_ACT   HEX(0xfbbf24)
#define C_TEXT_MAIN    HEX(0xc084fc)
#define C_TEXT_DIM     HEX(0x6b7280)
#define C_TEXT_WHITE   HEX(0xffffff)
#define C_GOLD         HEX(0xfbbf24)
#define C_GOLD_LIGHT   HEX(0xfde68a)
#define C_HP_GREEN     HEX(0x22c55e)
#define C_HUNGER_ORG   HEX(0xf97316)
#define C_ENERGY_BLUE  HEX(0x3b82f6)
#define C_XP_PURPLE    HEX(0x7c3aed)
#define C_WARN_RED     HEX(0xef4444)
#define C_HAPPY_PINK   HEX(0xf472b6)

/* ---- Sky / side-view ---- */
#define C_SKY_TOP      HEX(0x1a0a2e)
#define C_SKY_MID      HEX(0x2d1b4e)
#define C_SKY_BOT      HEX(0x4c1d95)
#define C_MOUNTAIN     HEX(0x2d1b4e)

/* ---- Misc ---- */
#define C_BLACK        ((uint16_t)0x0000)
#define C_WHITE        ((uint16_t)0xFFFF)
