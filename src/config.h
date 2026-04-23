#pragma once

/* Display */
#define DISPLAY_W   240
#define DISPLAY_H   240
#define TILE        16

/* Overworld grid (drawn via iso projection — see td_cam.h). */
#define MAP_W       30
#define MAP_H       20

/* Simulator */
#define SIM_SCALE   3

/* Timing */
#define TARGET_FPS   30
#define FRAME_MS     (1000 / TARGET_FPS)
#define TICK_MS      10000   /* game tick every 10 s */
#define ACTION_TICKS 30      /* frames per skill action (~1 s at 30 FPS) */

/* Stats / inventory */
#define STAT_MAX    100
#define INV_SLOTS   20

/* Movement (pixels per frame) */
#define TD_SPEED            1.5f
#define TD_WALK_ANIM_STEP   0.15f
#define WALK_FRAME_WRAP     2.0f

/*
 * Collision — the player is a circle (PL_RADIUS) anchored on the foot.
 * Obstacle shape is picked per tile to match its sprite:
 *   T_TREE                      → circle of radius OBSTACLE_R (rounded trunk)
 *   T_ROCK / T_ORE / T_WATER    → square of half-extent OBSTACLE_HALF
 * Circles slide smoothly around trunks; squares give rocks/ore the tile-
 * aligned feel their sprites want. PL_HALF_W/H are the broad-phase AABB
 * bounds used to iterate candidate tiles and happen to equal PL_RADIUS.
 */
#define PL_RADIUS       4
#define PL_HALF_W       PL_RADIUS
#define PL_HALF_H       PL_RADIUS
#define OBSTACLE_R      3
#define OBSTACLE_HALF   5

/* Running (hold B) */
#define TD_RUN_SPEED_MULT     1.70f
#define TD_RUN_ENERGY_RESUME  12     /* energy threshold before running re-engages */
#define TD_RUN_DRAIN_STEPS    4u     /* 1 energy per N movement steps while running */

/* Skills */
#define SK_MINING        0
#define SK_FISHING       1
#define SK_WOODCUT       2
#define SK_COOKING       3
#define SKILL_COUNT      4
#define SKILL_LEVEL_CAP  99

#define XP_WOODCUT_BASE  25
#define XP_WOODCUT_RAND  15
#define XP_MINING_BASE   30
#define XP_MINING_RAND   20
#define XP_FISHING_BASE  20
#define XP_FISHING_RAND  10
#define XP_DEFAULT       10

/* Per-tick economy */
#define ENERGY_REGEN_PER_TICK    1
#define TICKS_PER_DAY           20

/* Starting inventory */
#define START_INV_BREAD_COUNT    3

/* HUD */
#define HUD_LOG_TIMEOUT_MS  3000u
#define HUD_STRIP_H         18

/* Renderer */
#define TD_FEET_OFF  7.0f    /* iso depth-sort Y offset for player feet */
#define TD_ISO_CULL  100     /* extra pixels beyond screen edge for tile culling */

/* Node respawn */
#define NODE_RESPAWN_TICKS 60

/* Tile IDs */
#define T_GRASS      0
#define T_PATH       1
#define T_WATER      2
#define T_SAND       3
#define T_TREE       4
#define T_ROCK       5
#define T_FLOWER     6
#define T_TGRASS     7
#define T_ORE        9

/* Game mode */
typedef enum {
    MODE_TOPDOWN = 0,
    MODE_MENU,
} GameMode;

/* Player direction */
#define DIR_DOWN    0
#define DIR_UP      1
#define DIR_LEFT    2
#define DIR_RIGHT   3

/* Save file path (SDL simulator only; Pico uses flash). */
#define SAVE_PATH   "grumble_save.bin"
