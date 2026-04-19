#include "skills.h"
#include "pets.h"
#include <string.h>

/* ------------------------------------------------------------------ */
/* Skill metadata                                                       */
/* ------------------------------------------------------------------ */
const SkillInfo SKILL_INFO[SKILL_COUNT] = {
    [SK_MINING]   = {"Mining",   "[M]"},
    [SK_FISHING]  = {"Fishing",  "[F]"},
    [SK_WOODCUT]  = {"Woodcut",  "[W]"},
    [SK_COMBAT]   = {"Combat",   "[C]"},
    [SK_COOKING]  = {"Cooking",  "[K]"},
    [SK_MAGIC]    = {"Magic",    "[G]"},
    [SK_FARMING]  = {"Farming",  "[A]"},
    [SK_CRAFTING] = {"Crafting", "[R]"},
    [SK_SMITHING] = {"Smithing", "[S]"},
    [SK_HERBLORE] = {"Herblore", "[H]"},
};

/* ------------------------------------------------------------------ */
/* RNG (shared with player.c via separate state) */
/* ------------------------------------------------------------------ */
static uint32_t rng_state = 54321u;
static uint32_t rng_next(void) {
    rng_state = rng_state * 1664525u + 1013904223u;
    return rng_state;
}

/* ------------------------------------------------------------------ */
/* XP to level conversion                                               */
/* ------------------------------------------------------------------ */
static uint8_t xp_to_level(uint32_t xp) {
    uint8_t lv = 1;
    while (lv < 99 && xp >= xp_for_level((uint8_t)(lv + 1)))
        lv++;
    return lv;
}

/* ------------------------------------------------------------------ */
/* Add XP to a skill                                                    */
/* ------------------------------------------------------------------ */
void skill_add_xp(GameState *s, uint8_t skill_id, uint32_t xp) {
    if (skill_id >= SKILL_COUNT) return;
    Skill *sk = &s->skills[skill_id];
    uint8_t old_level = sk->level;
    sk->xp += xp;
    sk->level = xp_to_level(sk->xp);
    if (sk->level > old_level) {
        char msg[36];
        const char *sname = SKILL_INFO[skill_id].name;
        /* Simple string build without printf */
        int n = 0;
        while (*sname && n < 16) msg[n++] = *sname++;
        const char *lv = " lv up!";
        while (*lv && n < 34) msg[n++] = *lv++;
        msg[n] = '\0';
        state_log(s, msg);
    }

    /* Active pet gains half the skill XP */
    if (s->party_count > 0)
        pet_add_xp(s, s->active_pet, xp / 2);
}

/* ------------------------------------------------------------------ */
/* Random drop table                                                    */
/* ------------------------------------------------------------------ */
uint8_t skill_get_drop(uint8_t tile_id) {
    uint32_t r = rng_next() % 4;
    switch (tile_id) {
        case T_TREE:
            return (r < 3) ? ITEM_LOG : ITEM_BRANCH;
        case T_ROCK:
            return (r < 2) ? ITEM_STONE : ITEM_ORE;
        case T_ORE:
            return (r < 3) ? ITEM_ORE : ITEM_GEM;
        case T_WATER:
            return (r < 3) ? ITEM_FISH : ITEM_SEAWEED;
        default:
            return ITEM_HERB;
    }
}

/* ------------------------------------------------------------------ */
/* Complete a skilling action                                           */
/* ------------------------------------------------------------------ */
void skill_complete_action(GameState *s, World *w) {
    int tx = s->action_node_x;
    int ty = s->action_node_y;
    uint8_t tile = world_tile(w, tx, ty);

    uint8_t drop    = skill_get_drop(tile);
    uint32_t xp_val = 0;

    switch (s->active_skill) {
        case SK_WOODCUT: xp_val = 25 + (uint32_t)(rng_next() % 15); break;
        case SK_MINING:  xp_val = 30 + (uint32_t)(rng_next() % 20); break;
        case SK_FISHING: xp_val = 20 + (uint32_t)(rng_next() % 10); break;
        default:         xp_val = 10; break;
    }

    inv_add(s, drop, 1);
    skill_add_xp(s, s->active_skill, xp_val);

    /* XP drop effect at player tile */
    int px = (int)s->td.x;
    int py = (int)s->td.y;
    /* Convert to screen coords roughly */
    state_add_xp_drop(s, px % DISPLAY_W, py % DISPLAY_H, (uint16_t)xp_val);

    world_deplete_node(w, tx, ty);

    /* Queue another action */
    s->action_ticks_left = ACTION_TICKS;
}
