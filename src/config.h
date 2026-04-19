#pragma once

/* ---- Display ---- */
#define DISPLAY_W   240
#define DISPLAY_H   240
#define TILE        16

/* ---- World (orthogonal grid; overworld drawn isometric — see td_cam.h) ---- */
#define MAP_W       30
#define MAP_H       20

/* ---- Side-view world ---- */
#define SV_W        30
#define SV_H        9

/* ---- Simulator ---- */
#define SIM_SCALE   3

/* ---- Timing ---- */
#define TARGET_FPS  30
#define FRAME_MS    (1000 / TARGET_FPS)
#define TICK_MS     10000   /* game-state decay every 10 s */

/* ---- Action ---- */
#define ACTION_TICKS 30     /* frames per skill action */

/* ---- Stats ---- */
#define STAT_MAX    100
#define PARTY_MAX   4
#define BOX_MAX     12
#define INV_SLOTS   20
#define MOVE_SLOTS  4

/* ---- Movement speeds (pixels / frame) ---- */
#define TD_SPEED    1.5f
#define SV_SPEED    1.8f
#define SV_GRAVITY  0.35f
#define SV_JUMP    -5.5f
#define SV_MAX_FALL 5.0f

/* ---- Skill IDs ---- */
#define SK_MINING   0
#define SK_FISHING  1
#define SK_WOODCUT  2
#define SK_COMBAT   3
#define SK_COOKING  4
#define SK_MAGIC    5
#define SK_FARMING  6
#define SK_CRAFTING 7
#define SK_SMITHING 8
#define SK_HERBLORE 9
#define SKILL_COUNT 10

/* ---- Tile IDs (top-down) ---- */
#define T_GRASS     0
#define T_PATH      1
#define T_WATER     2
#define T_SAND      3
#define T_TREE      4
#define T_ROCK      5
#define T_FLOWER    6
#define T_TGRASS    7   /* tall grass — encounter zone */
#define T_HOME        8   /* door tile — enter base (see MANSION_DOOR_*) */
#define T_ORE         9
#define T_HOME_SOLID 18   /* mansion lot collision — grass iso; spr_mansion_td paints building */

/* Top-down mansion: north end of vertical path (x = 15). One spr_mansion_td() draw. */
#define MANSION_TX0      12
#define MANSION_TY0       0
#define MANSION_TW        6
#define MANSION_TH        5   /* body rows (map tiles) */
#define MANSION_PORCH_ROWS 1
#define MANSION_DOOR_TX   15
#define MANSION_DOOR_TY   4
#define mansion_tile(tx, ty) \
    ((tx) >= MANSION_TX0 && (tx) < MANSION_TX0 + MANSION_TW && \
     (ty) >= MANSION_TY0 && (ty) < MANSION_TY0 + MANSION_TH + (MANSION_PORCH_ROWS))

/* ---- Side-view tile IDs ---- */
#define SVT_AIR      0
#define SVT_GROUND   1
#define SVT_DIRT     2
#define SVT_PLATFORM 3
#define SVT_ORE      4
#define SVT_TREE     5

/* ---- Game mode ---- */
typedef enum {
    MODE_CHAR_CREATE = 0,
    MODE_TOPDOWN,
    MODE_SIDE,
    MODE_BATTLE,
    MODE_MENU,
    MODE_BASE,
} GameMode;

/* ---- Player direction ---- */
#define DIR_DOWN    0
#define DIR_UP      1
#define DIR_LEFT    2
#define DIR_RIGHT   3

/* ---- Save file path (desktop) ---- */
#define SAVE_PATH   "grumble_save.bin"
