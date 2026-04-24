#pragma once
#include <stdint.h>
#include <stdbool.h>

/*
 * Tool kinds. Nodes in actions.c require a kind, not a specific item —
 * any axe chops any tree, tier just sets speed. Kind = TOOL_NONE for
 * non-tool items (resources, ingots, food, coin, diamond core).
 */
typedef enum {
    TOOL_NONE = 0,
    TOOL_AXE,
    TOOL_PICKAXE,
    TOOL_ROD,
    TOOL_SHEARS,
    TOOL_SWORD,
} tool_kind_t;

typedef enum {
    ITEM_NONE = 0,

    /* Resources */
    ITEM_OAK_LOG, ITEM_PINE_LOG, ITEM_MAPLE_LOG, ITEM_YEW_LOG,
    ITEM_STONE,
    ITEM_COPPER_ORE, ITEM_TIN_ORE, ITEM_IRON_ORE,
    ITEM_SILVER_ORE, ITEM_GOLD_ORE,

    /* Ingots */
    ITEM_BRONZE_INGOT, ITEM_IRON_INGOT, ITEM_STEEL_INGOT, ITEM_DIAMOND_INGOT,

    /* Food — raw (eating = damage) */
    ITEM_RAW_FISH, ITEM_RAW_TROUT, ITEM_RAW_SALMON, ITEM_RAW_SHARK,

    /* Food — cooked (eating = heal) */
    ITEM_COOKED_FISH, ITEM_COOKED_TROUT, ITEM_COOKED_SALMON, ITEM_COOKED_SHARK,

    /* Tools — axes */
    ITEM_BRONZE_AXE, ITEM_IRON_AXE, ITEM_STEEL_AXE, ITEM_DIAMOND_AXE,
    /* Tools — pickaxes */
    ITEM_BRONZE_PICKAXE, ITEM_IRON_PICKAXE, ITEM_STEEL_PICKAXE, ITEM_DIAMOND_PICKAXE,
    /* Tools — rods */
    ITEM_BRONZE_ROD, ITEM_IRON_ROD, ITEM_STEEL_ROD, ITEM_DIAMOND_ROD,
    /* Tools — swords */
    ITEM_BRONZE_SWORD, ITEM_IRON_SWORD, ITEM_STEEL_SWORD, ITEM_DIAMOND_SWORD,
    /* Tools — unique */
    ITEM_SHEARS,

    /* Currency + special */
    ITEM_COIN,
    ITEM_DIAMOND_CORE,   /* boss drop — consumed by diamond-tier smelting */

    ITEM_COUNT,
} item_id_t;

typedef struct {
    const char  *name;
    uint8_t      max_stack;
    tool_kind_t  tool;   /* TOOL_NONE for non-tools */
    uint8_t      tier;   /* 1..4 for tools, 0 otherwise */
} item_def_t;

/* Convenience predicates derived from item_def_t. */
static inline bool item_is_tool(const item_def_t *d) { return d->tool != TOOL_NONE; }

extern const item_def_t ITEM_DEFS[ITEM_COUNT];

typedef struct {
    item_id_t id;
    uint8_t   count;
} inv_slot_t;

#define HOTBAR_SLOTS   8
#define BAG_SLOTS      20
#define INV_SLOTS      (HOTBAR_SLOTS + BAG_SLOTS)

/* Slots 4–7 (0-indexed) are reserved for tools in the hotbar. */
#define TOOL_SLOT_START 4

typedef struct {
    inv_slot_t slots[INV_SLOTS];
} Inventory;

void inventory_init_default(Inventory *inv);
int  inventory_add(Inventory *inv, item_id_t id, uint8_t count); /* slot touched, or -1 */
bool inventory_has_tool(const Inventory *inv, item_id_t tool_id);
int  inventory_count(const Inventory *inv, item_id_t id);

/* Highest tool tier of a given kind the player owns, or 0 if none. Used
 * by actions.c — nodes accept any tool of their kind, tier sets speed. */
uint8_t inventory_best_tool_tier(const Inventory *inv, tool_kind_t kind);
