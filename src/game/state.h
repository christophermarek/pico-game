#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "config.h"

typedef enum {
    ITEM_NONE    = 0,
    ITEM_ORE     = 1,
    ITEM_STONE   = 2,
    ITEM_FISH    = 3,
    ITEM_SEAWEED = 4,
    ITEM_LOG     = 5,
    ITEM_BRANCH  = 6,
    ITEM_GEM     = 7,
    ITEM_BREAD   = 8,
    ITEM_COUNT   = 9,
} ItemID;

typedef struct {
    uint32_t xp;
    uint8_t  level;
} Skill;

typedef struct {
    uint8_t item_id;
    uint8_t count;
} InvSlot;

typedef struct {
    float   x, y;
    uint8_t dir;        /* world-space facing (for interactions and facing_tile) */
    uint8_t screen_dir; /* monitor-relative facing (for sprite under rotated camera) */
    float   walk_frame;
    int16_t tile_x, tile_y;
} PlayerTD;

typedef enum {
    MTAB_SKILLS   = 0,
    MTAB_ITEMS    = 1,
    MTAB_SETTINGS = 2,
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
    InvSlot  inv[INV_SLOTS];

    bool     skilling;
    uint8_t  active_skill;
    int16_t  action_node_x, action_node_y;
    uint8_t  action_ticks_left;

    uint32_t tick_count;
    uint32_t frame_count;
    uint16_t day;

    MenuTab  menu_tab;
    uint8_t  menu_cursor;
    uint8_t  menu_scroll;

    char     log[4][36];
    uint8_t  log_count;
    uint32_t log_ms;

    uint32_t total_steps;

    bool     debug_mode;
    uint8_t  settings_cursor;

    /* Debug telemetry — written each frame by player_update_td. */
    float    dbg_dx, dbg_dy;
    bool     dbg_blocked_x, dbg_blocked_y;
} GameState;

void     state_init(GameState *s);
uint16_t total_level(const GameState *s);
uint32_t xp_for_level(uint8_t level);
void     state_log(GameState *s, const char *msg);
int      inv_count(const GameState *s, uint8_t item_id);
bool     inv_add(GameState *s, uint8_t item_id, uint8_t qty);
bool     inv_remove(GameState *s, uint8_t item_id, uint8_t qty);
