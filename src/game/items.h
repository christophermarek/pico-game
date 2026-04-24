#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    ITEM_NONE = 0,
    ITEM_OAK_LOG,
    ITEM_STONE,
    ITEM_IRON_ORE,
    ITEM_RAW_FISH,
    ITEM_COIN,
    ITEM_AXE,
    ITEM_PICKAXE,
    ITEM_FISHING_ROD,
    ITEM_SHEARS,
    ITEM_COUNT,
} item_id_t;

typedef struct {
    const char *name;
    uint8_t     max_stack;
    bool        is_tool;
} item_def_t;

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
