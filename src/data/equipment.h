#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* Forward declaration */
typedef struct GameState GameState;

#define EQUIP_COUNT     7
#define RECIPE_MAX      4

typedef struct {
    uint8_t item_id;
    uint8_t qty;
} RecipeStep;

typedef struct {
    char      id[16];
    char      name[16];
    int8_t    atk_bonus;
    int8_t    def_bonus;
    int8_t    spd_bonus;
    RecipeStep recipe[RECIPE_MAX];
    uint8_t   recipe_count;
} Equipment;

/*
 * Item IDs referenced in recipes (must match enum in state.h).
 * We use literal numbers here to avoid the circular include.
 *  1=ORE, 2=STONE, 3=FISH, 4=SEAWEED, 5=LOG, 6=BRANCH,
 *  7=GEM, 8=MEAL, 9=BREAD, 10=EGG, 11=COIN, 12=THREAD,
 *  13=HIDE, 14=HERB
 */
static const Equipment EQUIPMENT[EQUIP_COUNT] = {
    {
        "cloth_vest", "Cloth Vest",
        .atk_bonus=0, .def_bonus=1, .spd_bonus=0,
        .recipe      = {{12,2},{5,1},{0,0},{0,0}},
        .recipe_count = 2,
    },
    {
        "leaf_crown", "Leaf Crown",
        .atk_bonus=1, .def_bonus=0, .spd_bonus=1,
        .recipe      = {{6,3},{14,1},{0,0},{0,0}},
        .recipe_count = 2,
    },
    {
        "iron_collar", "Iron Collar",
        .atk_bonus=2, .def_bonus=2, .spd_bonus=0,
        .recipe      = {{1,3},{2,2},{0,0},{0,0}},
        .recipe_count = 2,
    },
    {
        "berry_charm", "Berry Charm",
        .atk_bonus=0, .def_bonus=0, .spd_bonus=0,
        .recipe      = {{14,2},{6,1},{12,1},{0,0}},
        .recipe_count = 3,
    },
    {
        "silk_cape", "Silk Cape",
        .atk_bonus=0, .def_bonus=1, .spd_bonus=2,
        .recipe      = {{12,4},{6,2},{0,0},{0,0}},
        .recipe_count = 2,
    },
    {
        "gem_amulet", "Gem Amulet",
        .atk_bonus=2, .def_bonus=1, .spd_bonus=1,
        .recipe      = {{7,1},{12,2},{14,1},{0,0}},
        .recipe_count = 3,
    },
    {
        "rune_plate", "Rune Plate",
        .atk_bonus=3, .def_bonus=4, .spd_bonus=0,
        .recipe      = {{1,4},{2,4},{7,1},{0,0}},
        .recipe_count = 3,
    },
};

static inline const Equipment *get_equipment(uint8_t idx) {
    if (idx == 0 || idx > EQUIP_COUNT) return NULL;
    return &EQUIPMENT[idx - 1];  /* slot 0 = none, slot 1 = first equip */
}

/* Returns true if inventory has the required items */
#ifdef GRUMBLE_STATE_DEFINED
static inline bool can_craft(const GameState *s, uint8_t equip_idx) {
    const Equipment *e = get_equipment(equip_idx);
    if (!e) return false;
    for (int i = 0; i < e->recipe_count; i++) {
        if (inv_count(s, e->recipe[i].item_id) < e->recipe[i].qty)
            return false;
    }
    return true;
}

/* Consume recipe ingredients; returns false if not enough */
static inline bool consume_recipe(GameState *s, uint8_t equip_idx) {
    const Equipment *e = get_equipment(equip_idx);
    if (!e) return false;
    if (!can_craft(s, equip_idx)) return false;
    for (int i = 0; i < e->recipe_count; i++) {
        inv_remove(s, e->recipe[i].item_id, e->recipe[i].qty);
    }
    return true;
}
#endif /* GRUMBLE_STATE_DEFINED */
