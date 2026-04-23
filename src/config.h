#pragma once

/* ---- Display ---- */
#define DISPLAY_W   240
#define DISPLAY_H   240
#define TILE        16

/* ---- World (orthogonal grid; overworld drawn isometric — see td_cam.h) ---- */
#define MAP_W       30
#define MAP_H       20

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
#define INV_SLOTS   20

/* ---- Movement speeds (pixels / frame) ---- */
#define TD_SPEED    1.5f

/* ---- Walk animation ---- */
#define TD_WALK_ANIM_STEP  0.15f   /* walk_frame advance per frame (top-down) */
#define WALK_FRAME_WRAP    2.0f    /* animation frames per cycle */

/* ---- Player collision (world px) ---- */
#define PL_RADIUS   4   /* player is a circle centred on the foot anchor */
/* Aliases kept for the broad-phase tile-iteration AABB (still axis-aligned). */
#define PL_HALF_W   PL_RADIUS
#define PL_HALF_H   PL_RADIUS

/*
 * Obstacle collision — shape is chosen per tile type to match the visual:
 *   T_TREE               → circle of radius OBSTACLE_R (rounded trunk)
 *   T_ROCK/T_ORE/T_WATER → axis-aligned square of half-extent OBSTACLE_HALF
 * Player is always a circle (PL_RADIUS) anchored on the foot.
 *
 * Circles slide smoothly around trunks; squares give rocks/ore the blocky,
 * tile-aligned feel that fits their sprites. Keep both sizes small enough
 * to leave walkable lanes between adjacent solid tiles.
 */
#define OBSTACLE_R     3   /* circle radius (trees)            */
#define OBSTACLE_HALF  5   /* square half-extent (rocks, etc.) */

/* ---- Running (hold B) ---- */
#define TD_RUN_SPEED_MULT     1.70f  /* speed multiplier while running */
#define TD_RUN_ENERGY_RESUME  12     /* energy threshold before running unlocks again */
#define TD_RUN_DRAIN_STEPS    4u     /* drain 1 energy every this many movement steps */

/* ---- Skill IDs ---- */
#define SK_MINING   0
#define SK_FISHING  1
#define SK_WOODCUT  2
#define SK_COOKING  3
#define SKILL_COUNT 4

/* ---- Level caps ---- */
#define SKILL_LEVEL_CAP  99

/* ---- Skill XP gains per action ---- */
#define XP_WOODCUT_BASE    25
#define XP_WOODCUT_RAND    15
#define XP_MINING_BASE     30
#define XP_MINING_RAND     20
#define XP_FISHING_BASE    20
#define XP_FISHING_RAND    10
#define XP_DEFAULT         10   /* fallback XP for unlisted skills */

/* ---- Game economy (per tick) ---- */
#define ENERGY_REGEN_PER_TICK    1   /* energy gained per tick while not skilling */
#define TICKS_PER_DAY           20   /* game ticks per in-game day */

/* ---- Default starting values ---- */
#define START_INV_BREAD_COUNT    3

/* ---- HUD ---- */
#define HUD_LOG_TIMEOUT_MS  3000u   /* how long a log message stays visible (ms) */
#define HUD_STRIP_H         18      /* bottom toolbar strip height (px) */

/* ---- Renderer ---- */
#define TD_FEET_OFF  7.0f   /* iso depth-sort Y offset for player feet */
#define TD_ISO_CULL  100    /* extra pixels beyond screen edge for tile culling */

/* ---- Node respawn ---- */
#define NODE_RESPAWN_TICKS 60  /* game ticks before a harvested node reappears */

/* ---- Tile IDs (top-down) ---- */
#define T_GRASS      0
#define T_PATH       1
#define T_WATER      2
#define T_SAND       3
#define T_TREE       4
#define T_ROCK       5
#define T_FLOWER     6
#define T_TGRASS     7   /* tall grass */
#define T_ORE        9

/* ---- Game mode ---- */
typedef enum {
    MODE_TOPDOWN = 0,
    MODE_MENU,
} GameMode;

/* ---- Player direction ---- */
#define DIR_DOWN    0
#define DIR_UP      1
#define DIR_LEFT    2
#define DIR_RIGHT   3

/* ---- Save file path (SDL simulator only; Pico uses flash) ---- */
#define SAVE_PATH   "grumble_save.bin"
