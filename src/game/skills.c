#include "skills.h"
#include "items.h"
#include "player.h"
#include "hal.h"

const SkillInfo SKILL_INFO[SKILL_COUNT] = {
    [SK_MINING]  = { "Mining",  "[M]" },
    [SK_FISHING] = { "Fishing", "[F]" },
    [SK_WOODCUT] = { "Woodcut", "[W]" },
};

/* LCG seeded lazily from hal_ticks_ms so XP rolls vary across runs.
 * RNG_SEED_FALLBACK guards the rare case hal_ticks_ms() returns 0 at
 * startup — LCGs lock up if seeded with zero. */
#define RNG_SEED_MIX      0xA1B2C3D4u
#define RNG_SEED_FALLBACK 0x13579BDFu
static uint32_t rng_state;

static uint32_t rng_next(void) {
    if (rng_state == 0) {
        rng_state = hal_ticks_ms() ^ RNG_SEED_MIX;
        if (rng_state == 0) rng_state = RNG_SEED_FALLBACK;
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

static void grant_item(GameState *s, World *w, int tx, int ty) {
    uint8_t   tile = world_tile(w, tx, ty);
    item_id_t item = ITEM_NONE;
    bool      coin_chance = false;

    switch (tile) {
        case T_TREE:   item = ITEM_OAK_LOG;  break;
        case T_ROCK:   item = ITEM_STONE;    break;
        case T_ORE:    item = ITEM_IRON_ORE; break;
        case T_WATER:  item = ITEM_RAW_FISH; break;
        case T_TGRASS: coin_chance = true;   break;
    }

    if (coin_chance) {
        if ((rng_next() % 10) == 0) item = ITEM_COIN;
    }

    if (item == ITEM_NONE) return;

    int slot = inventory_add(&s->inv, item, 1);
    float wx = (float)(tx * TILE + TILE / 2);
    float wy = (float)(ty * TILE + TILE / 2);
    state_spawn_item_fly(s, wx, wy, item, slot);

    if (item == ITEM_COIN) {
        state_log(s, "Found a coin!");
    } else {
        char msg[36];
        const char *n = ITEM_DEFS[item].name;
        int i = 0;
        msg[i++] = 'G'; msg[i++] = 'o'; msg[i++] = 't'; msg[i++] = ' ';
        while (*n && i < 33) msg[i++] = *n++;
        msg[i++] = '!'; msg[i] = '\0';
        state_log(s, msg);
    }
}

void skill_complete_action(GameState *s, World *w) {
    int tx = s->action_node_x;
    int ty = s->action_node_y;

    uint32_t xp_val = 0;
    switch (s->active_skill) {
        case SK_WOODCUT: xp_val = XP_WOODCUT_BASE + (rng_next() % XP_WOODCUT_RAND); break;
        case SK_MINING:  xp_val = XP_MINING_BASE  + (rng_next() % XP_MINING_RAND);  break;
        case SK_FISHING: xp_val = XP_FISHING_BASE + (rng_next() % XP_FISHING_RAND); break;
    }
    skill_add_xp(s, s->active_skill, xp_val);
    grant_item(s, w, tx, ty);
    world_hit_node(w, tx, ty);
    world_anim_on_hit(w, tx, ty);
    player_stop_action(s);
}
