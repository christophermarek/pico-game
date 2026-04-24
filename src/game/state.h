#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "config.h"
#include "items.h"

typedef struct {
    float     wx, wy;   /* world-pixel spawn position (node center) */
    int8_t    slot;     /* destination hotbar slot; -1 = bag/ignore */
    item_id_t item;
    uint8_t   timer;    /* ITEM_FLY_FRAMES → 0; 0 = inactive */
    bool      active;
} ItemFly;

typedef struct {
    uint32_t xp;
    uint8_t  level;
} Skill;

typedef struct {
    float   x, y;
    uint8_t dir;        /* world-space facing (for interactions and facing_tile) */
    uint8_t screen_dir; /* monitor-relative facing (for sprite under rotated camera) */
    float   walk_frame;
    int16_t tile_x, tile_y;
} PlayerTD;

typedef enum {
    MTAB_SKILLS    = 0,
    MTAB_SETTINGS  = 1,
    MTAB_INVENTORY = 2,
} MenuTab;

typedef struct GameState {
    GameMode mode;
    GameMode prev_mode;

    PlayerTD td;
    uint8_t  td_cam_bearing;

    int16_t  hp, max_hp;
    uint8_t  energy;
    bool     running_locked;

    Skill    skills[SKILL_COUNT];

    bool     skilling;
    uint8_t  active_skill;
    int16_t  action_node_x, action_node_y;
    uint8_t  action_ticks_left;

    uint32_t tick_count;
    uint32_t frame_count;
    uint16_t day;

    MenuTab  menu_tab;
    uint8_t  menu_cursor;

    char     log_msg[36];
    uint32_t log_ms;   /* hal_ticks_ms when log_msg was posted; 0 = none */

    uint32_t total_steps;

    Inventory inv;

    ItemFly  item_flies[MAX_ITEM_FLIES];
    uint8_t  slot_flash[HOTBAR_SLOTS];  /* per-slot border flash countdown */

    bool     debug_mode;
    uint8_t  settings_cursor;

    /* Debug telemetry — written each frame by player_update_td. */
    float    dbg_dx, dbg_dy;
    bool     dbg_blocked_x, dbg_blocked_y;
} GameState;

void     state_init(GameState *s);
uint32_t xp_for_level(uint8_t level);
void     state_log(GameState *s, const char *msg);
void     state_spawn_item_fly(GameState *s, float wx, float wy, item_id_t item, int slot);
void     state_anim_tick(GameState *s);
