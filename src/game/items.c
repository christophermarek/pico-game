#include "items.h"
#include <string.h>

/* Resource stack cap — tuned so one serious gathering run doesn't fill a
 * slot, forcing chest trips. 99 matches the skill cap by coincidence. */
#define RESOURCE_STACK 99

const item_def_t ITEM_DEFS[ITEM_COUNT] = {
    [ITEM_NONE]           = { "None",           0,              TOOL_NONE,    0 },

    /* Wood */
    [ITEM_OAK_LOG]        = { "Oak Log",        RESOURCE_STACK, TOOL_NONE,    0 },
    [ITEM_PINE_LOG]       = { "Pine Log",       RESOURCE_STACK, TOOL_NONE,    0 },
    [ITEM_MAPLE_LOG]      = { "Maple Log",      RESOURCE_STACK, TOOL_NONE,    0 },
    [ITEM_YEW_LOG]        = { "Yew Log",        RESOURCE_STACK, TOOL_NONE,    0 },

    /* Mined */
    [ITEM_STONE]          = { "Stone",          RESOURCE_STACK, TOOL_NONE,    0 },
    [ITEM_COPPER_ORE]     = { "Copper Ore",     RESOURCE_STACK, TOOL_NONE,    0 },
    [ITEM_TIN_ORE]        = { "Tin Ore",        RESOURCE_STACK, TOOL_NONE,    0 },
    [ITEM_IRON_ORE]       = { "Iron Ore",       RESOURCE_STACK, TOOL_NONE,    0 },
    [ITEM_SILVER_ORE]     = { "Silver Ore",     RESOURCE_STACK, TOOL_NONE,    0 },
    [ITEM_GOLD_ORE]       = { "Gold Ore",       RESOURCE_STACK, TOOL_NONE,    0 },

    /* Ingots */
    [ITEM_BRONZE_INGOT]   = { "Bronze Ingot",   RESOURCE_STACK, TOOL_NONE,    0 },
    [ITEM_IRON_INGOT]     = { "Iron Ingot",     RESOURCE_STACK, TOOL_NONE,    0 },
    [ITEM_STEEL_INGOT]    = { "Steel Ingot",    RESOURCE_STACK, TOOL_NONE,    0 },
    [ITEM_DIAMOND_INGOT]  = { "Diamond Ingot",  RESOURCE_STACK, TOOL_NONE,    0 },

    /* Raw food */
    [ITEM_RAW_FISH]       = { "Raw Fish",       RESOURCE_STACK, TOOL_NONE,    0 },
    [ITEM_RAW_TROUT]      = { "Raw Trout",      RESOURCE_STACK, TOOL_NONE,    0 },
    [ITEM_RAW_SALMON]     = { "Raw Salmon",     RESOURCE_STACK, TOOL_NONE,    0 },
    [ITEM_RAW_SHARK]      = { "Raw Shark",      RESOURCE_STACK, TOOL_NONE,    0 },

    /* Cooked food */
    [ITEM_COOKED_FISH]    = { "Cooked Fish",    RESOURCE_STACK, TOOL_NONE,    0 },
    [ITEM_COOKED_TROUT]   = { "Cooked Trout",   RESOURCE_STACK, TOOL_NONE,    0 },
    [ITEM_COOKED_SALMON]  = { "Cooked Salmon",  RESOURCE_STACK, TOOL_NONE,    0 },
    [ITEM_COOKED_SHARK]   = { "Cooked Shark",   RESOURCE_STACK, TOOL_NONE,    0 },

    /* Axes */
    [ITEM_BRONZE_AXE]     = { "Bronze Axe",     1, TOOL_AXE,     1 },
    [ITEM_IRON_AXE]       = { "Iron Axe",       1, TOOL_AXE,     2 },
    [ITEM_STEEL_AXE]      = { "Steel Axe",      1, TOOL_AXE,     3 },
    [ITEM_DIAMOND_AXE]    = { "Diamond Axe",    1, TOOL_AXE,     4 },

    /* Pickaxes */
    [ITEM_BRONZE_PICKAXE] = { "Bronze Pickaxe", 1, TOOL_PICKAXE, 1 },
    [ITEM_IRON_PICKAXE]   = { "Iron Pickaxe",   1, TOOL_PICKAXE, 2 },
    [ITEM_STEEL_PICKAXE]  = { "Steel Pickaxe",  1, TOOL_PICKAXE, 3 },
    [ITEM_DIAMOND_PICKAXE]= { "Diamond Pickaxe",1, TOOL_PICKAXE, 4 },

    /* Rods */
    [ITEM_BRONZE_ROD]     = { "Bronze Rod",     1, TOOL_ROD,     1 },
    [ITEM_IRON_ROD]       = { "Iron Rod",       1, TOOL_ROD,     2 },
    [ITEM_STEEL_ROD]      = { "Steel Rod",      1, TOOL_ROD,     3 },
    [ITEM_DIAMOND_ROD]    = { "Diamond Rod",    1, TOOL_ROD,     4 },

    /* Swords */
    [ITEM_BRONZE_SWORD]   = { "Bronze Sword",   1, TOOL_SWORD,   1 },
    [ITEM_IRON_SWORD]     = { "Iron Sword",     1, TOOL_SWORD,   2 },
    [ITEM_STEEL_SWORD]    = { "Steel Sword",    1, TOOL_SWORD,   3 },
    [ITEM_DIAMOND_SWORD]  = { "Diamond Sword",  1, TOOL_SWORD,   4 },

    [ITEM_SHEARS]         = { "Shears",         1, TOOL_SHEARS,  1 },

    [ITEM_COIN]           = { "Coin",           RESOURCE_STACK, TOOL_NONE,    0 },
    [ITEM_DIAMOND_CORE]   = { "Diamond Core",   RESOURCE_STACK, TOOL_NONE,    0 },
};

void inventory_init_default(Inventory *inv) {
    memset(inv, 0, sizeof(*inv));
    /* Development loadout — bronze tools pre-equipped so tier-1 gathering
     * works without the shopkeeper tutorial. Beat 1 replaces this with an
     * empty start + shears handed over by the NPC. */
    inv->slots[TOOL_SLOT_START + 0] = (inv_slot_t){ ITEM_BRONZE_AXE,     1 };
    inv->slots[TOOL_SLOT_START + 1] = (inv_slot_t){ ITEM_BRONZE_PICKAXE, 1 };
    inv->slots[TOOL_SLOT_START + 2] = (inv_slot_t){ ITEM_BRONZE_ROD,     1 };
    inv->slots[TOOL_SLOT_START + 3] = (inv_slot_t){ ITEM_SHEARS,         1 };
}

int inventory_add(Inventory *inv, item_id_t id, uint8_t count) {
    if (id == ITEM_NONE || id >= ITEM_COUNT || count == 0) return -1;
    uint8_t max_stack = ITEM_DEFS[id].max_stack;
    int first_slot = -1;

    for (int i = 0; i < INV_SLOTS; i++) {
        if (i >= TOOL_SLOT_START && i < TOOL_SLOT_START + 4) continue;
        inv_slot_t *sl = &inv->slots[i];
        if (sl->id == id && sl->count < max_stack) {
            uint8_t space = max_stack - sl->count;
            uint8_t add   = (count < space) ? count : space;
            sl->count += add;
            count     -= add;
            if (first_slot < 0) first_slot = i;
            if (count == 0) return first_slot;
        }
    }

    for (int i = 0; i < INV_SLOTS; i++) {
        if (i >= TOOL_SLOT_START && i < TOOL_SLOT_START + 4) continue;
        if (inv->slots[i].id == ITEM_NONE) {
            uint8_t add   = (count < max_stack) ? count : max_stack;
            inv->slots[i] = (inv_slot_t){ id, add };
            count        -= add;
            if (first_slot < 0) first_slot = i;
            if (count == 0) return first_slot;
        }
    }

    return first_slot;
}

bool inventory_has_tool(const Inventory *inv, item_id_t tool_id) {
    for (int i = 0; i < INV_SLOTS; i++)
        if (inv->slots[i].id == tool_id) return true;
    return false;
}

int inventory_count(const Inventory *inv, item_id_t id) {
    int total = 0;
    for (int i = 0; i < INV_SLOTS; i++)
        if (inv->slots[i].id == id) total += inv->slots[i].count;
    return total;
}

uint8_t inventory_best_tool_tier(const Inventory *inv, tool_kind_t kind) {
    uint8_t best = 0;
    for (int i = 0; i < INV_SLOTS; i++) {
        item_id_t id = inv->slots[i].id;
        if (id == ITEM_NONE) continue;
        const item_def_t *d = &ITEM_DEFS[id];
        if (d->tool == kind && d->tier > best) best = d->tier;
    }
    return best;
}
