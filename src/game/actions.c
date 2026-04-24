#include "actions.h"
#include "config.h"
#include <stddef.h>

static const NodeAction NODE_ACTIONS[] = {
    { T_TREE,   SK_WOODCUT, ITEM_AXE,         ITEM_OAK_LOG,  0,  ITEM_NONE,
      "Chopping tree...",  "Got Oak Log!",   NULL },
    { T_ROCK,   SK_MINING,  ITEM_PICKAXE,     ITEM_STONE,    0,  ITEM_NONE,
      "Mining rock...",    "Got Stone!",     NULL },
    { T_ORE,    SK_MINING,  ITEM_PICKAXE,     ITEM_IRON_ORE, 0,  ITEM_NONE,
      "Mining ore...",     "Got Iron Ore!",  NULL },
    { T_WATER,  SK_FISHING, ITEM_FISHING_ROD, ITEM_RAW_FISH, 0,  ITEM_NONE,
      "Fishing...",        "Got Raw Fish!",  NULL },
    { T_TGRASS, SK_WOODCUT, ITEM_SHEARS,      ITEM_NONE,     10, ITEM_COIN,
      "Shearing grass...", NULL,             "Found a coin!" },
};

const NodeAction *action_for_tile(uint8_t tile) {
    for (size_t i = 0; i < sizeof(NODE_ACTIONS) / sizeof(NODE_ACTIONS[0]); i++)
        if (NODE_ACTIONS[i].tile == tile) return &NODE_ACTIONS[i];
    return NULL;
}
