#include "recipes.h"
#include "structures.h"
#include "../config.h"
#include <stddef.h>

/* Cost table straight from GAME_DESIGN.md §"Crafting & recipes". Edit
 * there first, then here — the doc is the contract. */
const Recipe RECIPES[] = {
    /* -------- Smelting (campfire: bronze; forge: iron/steel/diamond) -------- */
    {
        .name = "Bronze Ingot",
        .station = STATION_CAMPFIRE,
        .inputs = { { ITEM_COPPER_ORE, 1 }, { ITEM_TIN_ORE, 1 } },
        .output = ITEM_BRONZE_INGOT, .output_count = 1, .skill_xp = 5,
    },
    {
        .name = "Iron Ingot",
        .station = STATION_FORGE,
        .inputs = { { ITEM_IRON_ORE, 1 } },
        .output = ITEM_IRON_INGOT, .output_count = 1, .skill_xp = 10,
    },
    {
        .name = "Steel Ingot",
        .station = STATION_FORGE,
        .inputs = { { ITEM_IRON_INGOT, 1 }, { ITEM_STONE, 1 } },
        .output = ITEM_STEEL_INGOT, .output_count = 1, .skill_xp = 25,
    },
    {
        .name = "Diamond Ingot",
        .station = STATION_FORGE,
        .inputs = { { ITEM_GOLD_ORE, 1 }, { ITEM_DIAMOND_CORE, 1 } },
        .output = ITEM_DIAMOND_INGOT, .output_count = 1, .skill_xp = 50,
    },

    /* -------- Bronze tools (workbench) -------- */
    {
        .name = "Bronze Axe",
        .station = STATION_WORKBENCH,
        .inputs = { { ITEM_BRONZE_INGOT, 1 }, { ITEM_OAK_LOG, 2 } },
        .output = ITEM_BRONZE_AXE, .output_count = 1,
    },
    {
        .name = "Bronze Pickaxe",
        .station = STATION_WORKBENCH,
        .inputs = { { ITEM_BRONZE_INGOT, 1 }, { ITEM_OAK_LOG, 2 } },
        .output = ITEM_BRONZE_PICKAXE, .output_count = 1,
    },
    {
        .name = "Bronze Rod",
        .station = STATION_WORKBENCH,
        .inputs = { { ITEM_BRONZE_INGOT, 1 }, { ITEM_OAK_LOG, 1 } },
        .output = ITEM_BRONZE_ROD, .output_count = 1,
    },
    {
        .name = "Bronze Sword",
        .station = STATION_WORKBENCH,
        .inputs = { { ITEM_BRONZE_INGOT, 1 }, { ITEM_OAK_LOG, 2 } },
        .output = ITEM_BRONZE_SWORD, .output_count = 1,
    },

    /* -------- Iron tools (workbench) -------- */
    {
        .name = "Iron Axe",
        .station = STATION_WORKBENCH,
        .inputs = { { ITEM_IRON_INGOT, 1 }, { ITEM_OAK_LOG, 2 } },
        .output = ITEM_IRON_AXE, .output_count = 1,
    },
    {
        .name = "Iron Pickaxe",
        .station = STATION_WORKBENCH,
        .inputs = { { ITEM_IRON_INGOT, 1 }, { ITEM_OAK_LOG, 2 } },
        .output = ITEM_IRON_PICKAXE, .output_count = 1,
    },
    {
        .name = "Iron Rod",
        .station = STATION_WORKBENCH,
        .inputs = { { ITEM_IRON_INGOT, 1 }, { ITEM_OAK_LOG, 1 } },
        .output = ITEM_IRON_ROD, .output_count = 1,
    },
    {
        .name = "Iron Sword",
        .station = STATION_WORKBENCH,
        .inputs = { { ITEM_IRON_INGOT, 1 }, { ITEM_OAK_LOG, 2 } },
        .output = ITEM_IRON_SWORD, .output_count = 1,
    },

    /* -------- Steel tools (workshop) -------- */
    {
        .name = "Steel Axe",
        .station = STATION_WORKSHOP,
        .inputs = { { ITEM_STEEL_INGOT, 1 }, { ITEM_OAK_LOG, 2 } },
        .output = ITEM_STEEL_AXE, .output_count = 1,
    },
    {
        .name = "Steel Pickaxe",
        .station = STATION_WORKSHOP,
        .inputs = { { ITEM_STEEL_INGOT, 1 }, { ITEM_OAK_LOG, 2 } },
        .output = ITEM_STEEL_PICKAXE, .output_count = 1,
    },
    {
        .name = "Steel Rod",
        .station = STATION_WORKSHOP,
        .inputs = { { ITEM_STEEL_INGOT, 1 }, { ITEM_OAK_LOG, 1 } },
        .output = ITEM_STEEL_ROD, .output_count = 1,
    },
    {
        .name = "Steel Sword",
        .station = STATION_WORKSHOP,
        .inputs = { { ITEM_STEEL_INGOT, 1 }, { ITEM_OAK_LOG, 2 } },
        .output = ITEM_STEEL_SWORD, .output_count = 1,
    },

    /* -------- Diamond tools (workshop + boss drop) -------- */
    {
        .name = "Diamond Axe",
        .station = STATION_WORKSHOP,
        .inputs = { { ITEM_DIAMOND_INGOT, 1 }, { ITEM_YEW_LOG, 2 } },
        .output = ITEM_DIAMOND_AXE, .output_count = 1,
    },
    {
        .name = "Diamond Pickaxe",
        .station = STATION_WORKSHOP,
        .inputs = { { ITEM_DIAMOND_INGOT, 1 }, { ITEM_YEW_LOG, 2 } },
        .output = ITEM_DIAMOND_PICKAXE, .output_count = 1,
    },
    {
        .name = "Diamond Rod",
        .station = STATION_WORKSHOP,
        .inputs = { { ITEM_DIAMOND_INGOT, 1 }, { ITEM_YEW_LOG, 1 } },
        .output = ITEM_DIAMOND_ROD, .output_count = 1,
    },
    {
        .name = "Diamond Sword",
        .station = STATION_WORKSHOP,
        .inputs = { { ITEM_DIAMOND_INGOT, 1 }, { ITEM_YEW_LOG, 2 } },
        .output = ITEM_DIAMOND_SWORD, .output_count = 1,
    },

    /* -------- Cooking (campfire: raw fish → cooked) -------- */
    {
        .name = "Cook Fish",
        .station = STATION_CAMPFIRE,
        .inputs = { { ITEM_RAW_FISH, 1 } },
        .output = ITEM_COOKED_FISH, .output_count = 1,
    },
    {
        .name = "Cook Trout",
        .station = STATION_CAMPFIRE,
        .inputs = { { ITEM_RAW_TROUT, 1 } },
        .output = ITEM_COOKED_TROUT, .output_count = 1,
    },
    {
        .name = "Cook Salmon",
        .station = STATION_CAMPFIRE,
        .inputs = { { ITEM_RAW_SALMON, 1 } },
        .output = ITEM_COOKED_SALMON, .output_count = 1,
    },
    {
        .name = "Cook Shark",
        .station = STATION_CAMPFIRE,
        .inputs = { { ITEM_RAW_SHARK, 1 } },
        .output = ITEM_COOKED_SHARK, .output_count = 1,
    },

    /* -------- Structures -------- *
     * .places carries the StructureKind that gets placed in front of
     * the player on craft completion. .output stays ITEM_NONE — the
     * crafted "thing" is the structure on the map, not an inventory
     * item. */
    {
        .name = "Workbench",
        .station = STATION_HAND,
        .inputs = { { ITEM_OAK_LOG, 10 }, { ITEM_STONE, 5 } },
        .places = STK_WORKBENCH,
    },
    {
        .name = "Shack",
        .station = STATION_WORKBENCH,
        .inputs = { { ITEM_OAK_LOG, 20 }, { ITEM_STONE, 15 } },
        .places = STK_SHACK,
    },
    {
        .name = "Chest",
        .station = STATION_WORKBENCH,
        .inputs = { { ITEM_OAK_LOG, 10 }, { ITEM_STONE, 2 } },
        .places = STK_CHEST,
    },
    {
        .name = "Campfire",
        .station = STATION_WORKBENCH,
        .inputs = { { ITEM_OAK_LOG, 8 }, { ITEM_STONE, 3 } },
        .places = STK_CAMPFIRE,
    },
    {
        .name = "Forge",
        .station = STATION_WORKBENCH,
        .inputs = { { ITEM_STONE, 25 }, { ITEM_IRON_ORE, 5 } },
        .places = STK_FORGE,
    },
    {
        .name = "Workshop",
        .station = STATION_WORKBENCH,
        .inputs = { { ITEM_STONE, 20 }, { ITEM_IRON_INGOT, 15 }, { ITEM_MAPLE_LOG, 10 } },
        .places = STK_WORKSHOP,
    },
};
const int RECIPES_COUNT = (int)(sizeof(RECIPES) / sizeof(RECIPES[0]));

bool recipe_can_craft(const Recipe *r, const Inventory *inv) {
    for (int i = 0; i < RECIPE_MAX_INPUTS; i++) {
        if (r->inputs[i].id == ITEM_NONE) continue;
        if (inventory_count(inv, r->inputs[i].id) < r->inputs[i].count)
            return false;
    }
    return true;
}

/* Remove `want` of `id` from `inv`, walking any slots that hold it. */
static void take_from_inventory(Inventory *inv, item_id_t id, int want) {
    for (int i = 0; i < INV_SLOTS && want > 0; i++) {
        if (inv->slots[i].id != id) continue;
        uint8_t take = (inv->slots[i].count < want)
                       ? inv->slots[i].count : (uint8_t)want;
        inv->slots[i].count -= take;
        want -= take;
        if (inv->slots[i].count == 0) inv->slots[i].id = ITEM_NONE;
    }
}

bool recipe_craft(const Recipe *r, Inventory *inv) {
    if (!recipe_can_craft(r, inv)) return false;

    for (int i = 0; i < RECIPE_MAX_INPUTS; i++) {
        if (r->inputs[i].id == ITEM_NONE) continue;
        take_from_inventory(inv, r->inputs[i].id, r->inputs[i].count);
    }

    if (r->output != ITEM_NONE && r->output_count > 0)
        inventory_add(inv, r->output, r->output_count);
    return true;
}

const char *station_name(CraftStation st) {
    switch (st) {
    case STATION_HAND:      return "Hand";
    case STATION_WORKBENCH: return "Workbench";
    case STATION_WORKSHOP:  return "Workshop";
    case STATION_FORGE:     return "Forge";
    case STATION_CAMPFIRE:  return "Campfire";
    default:                return "?";
    }
}
