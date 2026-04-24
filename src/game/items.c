#include "items.h"
#include <string.h>

const item_def_t ITEM_DEFS[ITEM_COUNT] = {
    [ITEM_NONE]        = { "None",        0,  false },
    [ITEM_OAK_LOG]     = { "Oak Log",     99, false },
    [ITEM_STONE]       = { "Stone",       99, false },
    [ITEM_IRON_ORE]    = { "Iron Ore",    99, false },
    [ITEM_RAW_FISH]    = { "Raw Fish",    99, false },
    [ITEM_COIN]        = { "Coin",        99, false },
    [ITEM_AXE]         = { "Axe",         1,  true  },
    [ITEM_PICKAXE]     = { "Pickaxe",     1,  true  },
    [ITEM_FISHING_ROD] = { "Fishing Rod", 1,  true  },
    [ITEM_SHEARS]      = { "Shears",      1,  true  },
};

void inventory_init_default(Inventory *inv) {
    memset(inv, 0, sizeof(*inv));
    /* Pre-load tools into fixed hotbar slots 4–7. */
    inv->slots[TOOL_SLOT_START + 0] = (inv_slot_t){ ITEM_AXE,         1 };
    inv->slots[TOOL_SLOT_START + 1] = (inv_slot_t){ ITEM_PICKAXE,     1 };
    inv->slots[TOOL_SLOT_START + 2] = (inv_slot_t){ ITEM_FISHING_ROD, 1 };
    inv->slots[TOOL_SLOT_START + 3] = (inv_slot_t){ ITEM_SHEARS,      1 };
}

int inventory_add(Inventory *inv, item_id_t id, uint8_t count) {
    if (id == ITEM_NONE || id >= ITEM_COUNT || count == 0) return -1;
    uint8_t max_stack = ITEM_DEFS[id].max_stack;
    int first_slot = -1;

    /* Try to stack into existing slots first (resource slots 0–3, then bag). */
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

    /* Fill empty slots. */
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

    return first_slot; /* -1 if inventory full */
}

bool inventory_has_tool(const Inventory *inv, item_id_t tool_id) {
    for (int i = 0; i < INV_SLOTS; i++) {
        if (inv->slots[i].id == tool_id) return true;
    }
    return false;
}

int inventory_count(const Inventory *inv, item_id_t id) {
    int total = 0;
    for (int i = 0; i < INV_SLOTS; i++) {
        if (inv->slots[i].id == id) total += inv->slots[i].count;
    }
    return total;
}
