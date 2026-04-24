#include "actions.h"
#include "config.h"
#include <stddef.h>

static const NodeAction NODE_ACTIONS[] = {
    /* -------- Woodcut (4 tiers) -------- */
    { T_TREE,       SK_WOODCUT, 1,  TOOL_AXE,     ITEM_OAK_LOG,   0, ITEM_NONE,
      "Chopping oak...",   "Got Oak Log!",    NULL },
    { T_TREE_PINE,  SK_WOODCUT, 15, TOOL_AXE,     ITEM_PINE_LOG,  0, ITEM_NONE,
      "Chopping pine...",  "Got Pine Log!",   NULL },
    { T_TREE_MAPLE, SK_WOODCUT, 30, TOOL_AXE,     ITEM_MAPLE_LOG, 0, ITEM_NONE,
      "Chopping maple...", "Got Maple Log!",  NULL },
    { T_TREE_YEW,   SK_WOODCUT, 50, TOOL_AXE,     ITEM_YEW_LOG,   0, ITEM_NONE,
      "Chopping yew...",   "Got Yew Log!",    NULL },

    /* -------- Mining (tier-1 has three nodes: stone/copper/tin) -------- */
    { T_ROCK,       SK_MINING,  1,  TOOL_PICKAXE, ITEM_STONE,      0, ITEM_NONE,
      "Mining stone...",   "Got Stone!",      NULL },
    { T_ORE_COPPER, SK_MINING,  1,  TOOL_PICKAXE, ITEM_COPPER_ORE, 0, ITEM_NONE,
      "Mining copper...",  "Got Copper Ore!", NULL },
    { T_ORE_TIN,    SK_MINING,  1,  TOOL_PICKAXE, ITEM_TIN_ORE,    0, ITEM_NONE,
      "Mining tin...",     "Got Tin Ore!",    NULL },
    { T_ORE,        SK_MINING,  15, TOOL_PICKAXE, ITEM_IRON_ORE,   0, ITEM_NONE,
      "Mining iron...",    "Got Iron Ore!",   NULL },
    { T_ORE_SILVER, SK_MINING,  30, TOOL_PICKAXE, ITEM_SILVER_ORE, 0, ITEM_NONE,
      "Mining silver...",  "Got Silver Ore!", NULL },
    { T_ORE_GOLD,   SK_MINING,  50, TOOL_PICKAXE, ITEM_GOLD_ORE,   0, ITEM_NONE,
      "Mining gold...",    "Got Gold Ore!",   NULL },

    /* -------- Fishing (4 tiers) -------- */
    { T_WATER,       SK_FISHING, 1,  TOOL_ROD, ITEM_RAW_FISH,   0, ITEM_NONE,
      "Fishing...",        "Got Raw Fish!",   NULL },
    { T_WATER_RIVER, SK_FISHING, 15, TOOL_ROD, ITEM_RAW_TROUT,  0, ITEM_NONE,
      "Fishing river...",  "Got Raw Trout!",  NULL },
    { T_WATER_DEEP,  SK_FISHING, 30, TOOL_ROD, ITEM_RAW_SALMON, 0, ITEM_NONE,
      "Fishing deep...",   "Got Raw Salmon!", NULL },
    { T_WATER_DARK,  SK_FISHING, 50, TOOL_ROD, ITEM_RAW_SHARK,  0, ITEM_NONE,
      "Fishing dark water...", "Got Raw Shark!", NULL },

    /* -------- Shearing (tall grass: chance drop of a coin) -------- */
    { T_TGRASS,     SK_WOODCUT, 1,  TOOL_SHEARS, ITEM_NONE, 10, ITEM_COIN,
      "Shearing grass...", NULL,              "Found a coin!" },
};

const NodeAction *action_for_tile(uint8_t tile) {
    for (size_t i = 0; i < sizeof(NODE_ACTIONS) / sizeof(NODE_ACTIONS[0]); i++)
        if (NODE_ACTIONS[i].tile == tile) return &NODE_ACTIONS[i];
    return NULL;
}

uint8_t action_ticks_for_tier(uint8_t tier) {
    /* Speed multipliers: 1× / 1.5× / 2× / 3×. Returned as ACTION_TICKS
     * divided appropriately; caller uses this directly. */
    switch (tier) {
    case 4:  return (uint8_t)(ACTION_TICKS / 3);   /* diamond — 3× */
    case 3:  return (uint8_t)(ACTION_TICKS / 2);   /* steel   — 2× */
    case 2:  return (uint8_t)((ACTION_TICKS * 2) / 3); /* iron — 1.5× */
    case 1:
    default: return (uint8_t)ACTION_TICKS;          /* bronze / none */
    }
}
