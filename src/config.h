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

/* Timing
 *
 * FRAME_MS drives rendering (33 ms @ 30 FPS).
 * TICK_MS drives world-economy updates (energy regen, respawn countdowns, the
 *   day counter). Everything timed in ticks scales with this — derived values
 *   below (TICKS_PER_DAY, NODE_RESPAWN_TICKS, SAVE_EVERY_TICKS) are tuned
 *   against TICK_MS = 2000, so a full in-game day is ~2 minutes and the
 *   energy bar fills in ~3.3 minutes of idle time.
 * ACTION_TICKS is counted in *render frames*, not game ticks, so skill
 *   actions stay a constant ~1 s regardless of TICK_MS tuning.
 */
#define TARGET_FPS   30
#define FRAME_MS     (1000 / TARGET_FPS)
#define TICK_MS      2000
#define ACTION_TICKS 30

/* Stats */
#define STAT_MAX    100

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
#define SKILL_COUNT      3
#define SKILL_LEVEL_CAP  99

#define XP_WOODCUT_BASE  25
#define XP_WOODCUT_RAND  15
#define XP_MINING_BASE   30
#define XP_MINING_RAND   20
#define XP_FISHING_BASE  20
#define XP_FISHING_RAND  10

/* Per-tick economy (TICK_MS = 2000 ⇒ 1 tick = 2 s) */
#define ENERGY_REGEN_PER_TICK   1      /* 1/2 s ⇒ 200 s to refill full energy */
#define TICKS_PER_DAY           60     /* day = 120 s (2 min)                 */
#define SAVE_EVERY_TICKS        30u    /* autosave every 60 s                 */

/* HUD — overlay mode (no reserved strip; world renders full 240x240) */
#define HUD_LOG_TIMEOUT_MS   3000u

/* Stat block (top-left overlay) */
#define HUD_STAT_X           4
#define HUD_STAT_Y           4
#define HUD_BAR_W            60
#define HUD_BAR_H            6
#define HUD_BAR_ROW_GAP      10   /* y distance between HP and energy rows */

/* Hotbar (bottom overlay) */
#define HUD_HOTBAR_SLOT_W    22
#define HUD_HOTBAR_SLOT_GAP  2
#define HUD_HOTBAR_Y         (DISPLAY_H - HUD_HOTBAR_SLOT_W - 2)

/* Renderer */
#define TD_FEET_OFF  7.0f    /* iso depth-sort Y offset for player feet */
#define TD_ISO_CULL  100     /* extra pixels beyond screen edge for tile culling */

/* Node respawn (TICK_MS = 2000 ⇒ 60 s before a depleted tile regrows) */
#define NODE_RESPAWN_TICKS 30

/* Item fly animation */
#define MAX_ITEM_FLIES    4
#define ITEM_FLY_FRAMES   20
#define SLOT_FLASH_FRAMES 8

/* Tile IDs */
#define T_GRASS      0
#define T_PATH       1
#define T_WATER      2
#define T_SAND       3
#define T_TREE       4
#define T_ROCK       5
#define T_FLOWER     6
#define T_TGRASS     7
#define T_ORE        8

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
