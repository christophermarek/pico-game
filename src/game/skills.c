#include "skills.h"
#include "player.h"
#include "hal.h"

const SkillInfo SKILL_INFO[SKILL_COUNT] = {
    [SK_MINING]  = { "Mining",  "[M]" },
    [SK_FISHING] = { "Fishing", "[F]" },
    [SK_WOODCUT] = { "Woodcut", "[W]" },
    [SK_COOKING] = { "Cooking", "[K]" },
};

/* LCG — seeded lazily from hal_ticks_ms so drops aren't identical across runs. */
static uint32_t rng_state;

static uint32_t rng_next(void) {
    if (rng_state == 0) {
        rng_state = hal_ticks_ms() ^ 0xA1B2C3D4u;
        if (rng_state == 0) rng_state = 0xDEADBEEFu;
    }
    rng_state = rng_state * 1664525u + 1013904223u;
    return rng_state;
}

static uint8_t xp_to_level(uint32_t xp) {
    uint8_t lv = 1;
    while (lv < SKILL_LEVEL_CAP && xp >= xp_for_level((uint8_t)(lv + 1)))
        lv++;
    return lv;
}

static uint8_t skill_get_drop(uint8_t tile_id) {
    uint32_t r = rng_next() % 4;
    switch (tile_id) {
        case T_TREE:  return (r < 3) ? ITEM_LOG   : ITEM_BRANCH;
        case T_ROCK:  return (r < 2) ? ITEM_STONE : ITEM_ORE;
        case T_ORE:   return (r < 3) ? ITEM_ORE   : ITEM_GEM;
        case T_WATER: return (r < 3) ? ITEM_FISH  : ITEM_SEAWEED;
        default:      return ITEM_NONE;
    }
}

void skill_add_xp(GameState *s, uint8_t skill_id, uint32_t xp) {
    if (skill_id >= SKILL_COUNT) return;
    Skill *sk = &s->skills[skill_id];
    uint8_t old_level = sk->level;
    sk->xp += xp;
    sk->level = xp_to_level(sk->xp);
    if (sk->level > old_level) {
        char msg[36];
        const char *sname = SKILL_INFO[skill_id].name;
        int n = 0;
        while (*sname && n < 16) msg[n++] = *sname++;
        const char *lv = " lv up!";
        while (*lv && n < 34) msg[n++] = *lv++;
        msg[n] = '\0';
        state_log(s, msg);
    }
}

void skill_complete_action(GameState *s, World *w) {
    int tx = s->action_node_x;
    int ty = s->action_node_y;
    uint8_t tile = world_tile(w, tx, ty);

    uint8_t  drop   = skill_get_drop(tile);
    uint32_t xp_val;
    switch (s->active_skill) {
        case SK_WOODCUT: xp_val = XP_WOODCUT_BASE + (rng_next() % XP_WOODCUT_RAND); break;
        case SK_MINING:  xp_val = XP_MINING_BASE  + (rng_next() % XP_MINING_RAND);  break;
        case SK_FISHING: xp_val = XP_FISHING_BASE + (rng_next() % XP_FISHING_RAND); break;
        default:         xp_val = XP_DEFAULT; break;
    }

    if (drop != ITEM_NONE) inv_add(s, drop, 1);
    skill_add_xp(s, s->active_skill, xp_val);

    world_deplete_node(w, tx, ty);
    player_stop_action(s);
}
