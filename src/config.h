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

/* ---- Walk animation ---- */
#define TD_WALK_ANIM_STEP  0.15f   /* walk_frame advance per frame (top-down) */
#define SV_WALK_ANIM_STEP  0.20f   /* walk_frame advance per frame (side-view) */
#define WALK_FRAME_WRAP    2.0f    /* animation frames per cycle (both views) */

/* ---- Pet following ---- */
#define TD_PET_FOLLOW_MULT  1.1f   /* pet follow speed as multiple of TD_SPEED */
#define SV_PET_FOLLOW_MULT  0.9f   /* pet follow speed as multiple of SV_SPEED */

/* ---- Player collision box ---- */
#define PL_HALF_W   5   /* half-width (px) of player AABB */
#define PL_HALF_H   7   /* half-height (px) of player AABB */

/* ---- Running (hold B) ---- */
#define TD_RUN_SPEED_MULT     1.70f  /* speed multiplier while running */
#define TD_RUN_ENERGY_RESUME  12     /* energy threshold before running unlocks again */
#define TD_RUN_DRAIN_STEPS    4u     /* drain 1 energy every this many movement steps */

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

/* ---- Level caps ---- */
#define SKILL_LEVEL_CAP  99
#define PET_LEVEL_CAP    99

/* ---- Skill XP gains per action ---- */
#define XP_WOODCUT_BASE    25
#define XP_WOODCUT_RAND    15
#define XP_MINING_BASE     30
#define XP_MINING_RAND     20
#define XP_FISHING_BASE    20
#define XP_FISHING_RAND    10
#define XP_DEFAULT         10   /* fallback XP for unlisted skills */
#define XP_SV_MINING       15   /* quick XP for side-view ore interaction */
#define XP_SV_WOODCUT      12   /* quick XP for side-view tree interaction */
#define PET_XP_SHARE_DIVISOR 2  /* active pet earns skill_xp / N */

/* ---- Game economy (per tick) ---- */
#define HUNGER_DECAY_PER_TICK    2   /* hunger reduced each game tick */
#define STARVATION_HP_LOSS       3   /* HP lost per tick when hunger == 0 */
#define ENERGY_REGEN_PER_TICK    1   /* energy gained per tick while not skilling */
#define TICKS_PER_DAY           20   /* game ticks per in-game day */

/* ---- Farm ---- */
#define FARM_PATCH_COUNT  3   /* number of farm patch slots */

/* ---- Default starting values ---- */
#define PET_DEFAULT_HAPPINESS   60
#define START_PET_LEVEL          5
#define START_INV_BREAD_COUNT    3
#define START_INV_ORB_COUNT      3

/* ---- XP drop effect ---- */
#define XP_DROP_MAX    8    /* max simultaneous floating XP indicators */
#define XP_DROP_TIMER  40   /* lifetime in frames */

/* ---- HUD ---- */
#define HUD_LOG_TIMEOUT_MS  3000u   /* how long a log message stays visible (ms) */
#define HUD_STRIP_H         18      /* bottom toolbar strip height (px) */

/* ---- Renderer ---- */
#define TD_FEET_OFF  7.0f   /* iso depth-sort Y offset for player feet */
#define TD_PET_FEET  5.0f   /* iso depth-sort Y offset for pet */
#define TD_ISO_CULL  72     /* extra pixels beyond screen edge for tile culling */

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
#define T_HOME       8   /* door tile — enter base (see MANSION_DOOR_*) */
#define T_ORE        9
#define T_HOME_SOLID 18  /* mansion lot collision — grass iso; spr_mansion_td paints building */

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
    MODE_MENU,
    MODE_BASE,
} GameMode;

/* ---- Player direction ---- */
#define DIR_DOWN    0
#define DIR_UP      1
#define DIR_LEFT    2
#define DIR_RIGHT   3

/* ---- Save file path (SDL simulator only; Pico uses flash) ---- */
#define SAVE_PATH   "grumble_save.bin"
