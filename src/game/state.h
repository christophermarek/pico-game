#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "config.h"

/* ------------------------------------------------------------------ */
/* Item IDs                                                             */
/* ------------------------------------------------------------------ */
typedef enum {
    ITEM_NONE    = 0,
    ITEM_ORE     = 1,
    ITEM_STONE   = 2,
    ITEM_FISH    = 3,
    ITEM_SEAWEED = 4,
    ITEM_LOG     = 5,
    ITEM_BRANCH  = 6,
    ITEM_GEM     = 7,
    ITEM_MEAL    = 8,
    ITEM_BREAD   = 9,
    ITEM_EGG     = 10,
    ITEM_COIN    = 11,
    ITEM_THREAD  = 12,
    ITEM_HIDE    = 13,
    ITEM_HERB    = 14,
    ITEM_POTION  = 15,
    ITEM_ORB     = 16,
    ITEM_ORAN    = 17,
    ITEM_SITRUS  = 18,
    ITEM_PECHA   = 19,
    ITEM_RAWST   = 20,
    ITEM_LEPPA   = 21,
    ITEM_WIKI    = 22,
    ITEM_MAGO    = 23,
    ITEM_LUM     = 24,
    ITEM_COUNT   = 25,
} ItemID;

/* ------------------------------------------------------------------ */
/* Skill                                                                */
/* ------------------------------------------------------------------ */
typedef struct {
    uint32_t xp;
    uint8_t  level;
} Skill;

/* ------------------------------------------------------------------ */
/* Pet                                                                  */
/* ------------------------------------------------------------------ */
typedef struct Pet {
    uint8_t  species_id;
    uint8_t  evo_stage;
    uint8_t  level;
    uint32_t xp;
    int16_t  hp, max_hp;
    int8_t   atk, def, spd;
    uint8_t  happiness;
    uint8_t  equip_id;          /* index into equipment table, 0=none */
    uint8_t  moves[MOVE_SLOTS];
    uint8_t  move_pp[MOVE_SLOTS];
    float    trail_x, trail_y;
} Pet;

/* ------------------------------------------------------------------ */
/* Inventory                                                            */
/* ------------------------------------------------------------------ */
typedef struct {
    uint8_t item_id;
    uint8_t count;
} InvSlot;

/* ------------------------------------------------------------------ */
/* Player customisation                                                 */
/* ------------------------------------------------------------------ */
typedef struct {
    char    name[12];
    uint8_t skin_idx;
    uint8_t hair_idx;
    uint8_t outfit_idx;
} PlayerCustom;

/* ------------------------------------------------------------------ */
/* Top-down player position                                             */
/* ------------------------------------------------------------------ */
typedef struct {
    float   x, y;
    uint8_t dir;        /* world-space facing (interactions, facing_tile) */
    uint8_t screen_dir; /* monitor-relative facing (sprite under rotated view) */
    float   walk_frame;
    int16_t tile_x, tile_y;
} PlayerTD;

/* ------------------------------------------------------------------ */
/* Side-view player position                                            */
/* ------------------------------------------------------------------ */
typedef struct {
    float   x, y, vy;
    bool    on_ground;
    uint8_t dir;
    float   walk_frame;
} PlayerSV;

/* ------------------------------------------------------------------ */
/* XP drop (floating text effect)                                       */
/* ------------------------------------------------------------------ */
typedef struct {
    int      x, y;
    uint16_t amount;
    int      timer;
} XpDrop;

/* ------------------------------------------------------------------ */
/* Home base / furniture                                                */
/* ------------------------------------------------------------------ */
#define FURNITURE_SLOTS 8

typedef enum {
    FURN_NONE      = 0,
    FURN_BED       = 1,
    FURN_WORKBENCH = 2,
    FURN_FORGE     = 3,
    FURN_FARM      = 4,
    FURN_CHEST     = 5,
    FURN_PET_BED   = 6,
    FURN_PET_HOUSE = 7,
} FurnitureType;

typedef struct {
    FurnitureType type;
    bool          placed;
} FurnitureSlot;

/* ------------------------------------------------------------------ */
/* Farming                                                              */
/* ------------------------------------------------------------------ */
typedef struct {
    uint8_t berry_id;
    uint8_t grow_ticks;
    bool    ready;
} FarmPatch;

/* ------------------------------------------------------------------ */
/* Menu                                                                 */
/* ------------------------------------------------------------------ */
typedef enum {
    MTAB_SKILLS   = 0,
    MTAB_ITEMS    = 1,
    MTAB_PARTY    = 2,
    MTAB_CRAFTING = 3,
} MenuTab;

/* ------------------------------------------------------------------ */
/* Full game state                                                      */
/* ------------------------------------------------------------------ */
typedef struct GameState {
    GameMode  mode;
    GameMode  prev_mode;

    PlayerCustom custom;
    PlayerTD     td;
    PlayerSV     sv;
    /* Top-down only: 0..TD_CAM_STEPS-1, 90° per step (see td_cam.h) */
    uint8_t      td_cam_bearing;

    int16_t  hp, max_hp;
    uint8_t  hunger, energy;
    bool     running_locked; /* energy depleted: must regen above threshold */

    Skill    skills[SKILL_COUNT];
    InvSlot  inv[INV_SLOTS];

    Pet      party[PARTY_MAX];
    Pet      box[BOX_MAX];
    uint8_t  party_count;
    uint8_t  box_count;
    uint8_t  active_pet;

    bool     skilling;
    uint8_t  active_skill;
    int16_t  action_node_x, action_node_y;
    uint8_t  action_ticks_left;

    uint32_t tick_count;
    uint16_t day;

    XpDrop   xp_drops[XP_DROP_MAX];
    uint8_t  xp_drop_count;

    FurnitureSlot furniture[FURNITURE_SLOTS];
    FarmPatch     farm[FARM_PATCH_COUNT];
    uint8_t       home_cursor_x, home_cursor_y;

    MenuTab  menu_tab;
    uint8_t  menu_cursor;
    uint8_t  menu_scroll;

    uint8_t  char_cursor;

    char     log[4][36];
    uint8_t  log_count;
    uint32_t log_ms;        /* hal_ticks_ms() when the latest message was added */

    uint8_t  recruited_flags; /* bitmask for auto-recruit milestones (bit 0/1/2) */

    uint32_t total_steps;
} GameState;

/* ------------------------------------------------------------------ */
/* API                                                                  */
/* ------------------------------------------------------------------ */
void     state_init(GameState *s);
uint16_t total_level(const GameState *s);
uint32_t xp_for_level(uint8_t level);
void     state_log(GameState *s, const char *msg);
void     state_add_xp_drop(GameState *s, int x, int y, uint16_t amount);
int      inv_count(const GameState *s, uint8_t item_id);
bool     inv_add(GameState *s, uint8_t item_id, uint8_t qty);
bool     inv_remove(GameState *s, uint8_t item_id, uint8_t qty);

/* Sentinel so berries.h / equipment.h know Pet is complete */
#define GRUMBLE_STATE_DEFINED 1
