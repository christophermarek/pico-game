#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* Forward declaration — Pet is defined in game/state.h */
typedef struct Pet Pet;

typedef struct {
    char    id[12];
    char    name[16];
    int8_t  hp_restore;
    int8_t  happy_bonus;
    int8_t  atk_bonus;
    int8_t  spd_bonus;
    uint8_t pp_restore;   /* PP restored per move slot */
} Berry;

#define BERRY_COUNT 8

static const Berry BERRY_TABLE[BERRY_COUNT] = {
    /* id          name            hp  hap atk spd pp */
    {"oran",   "Oran Berry",    10,   2,  0,  0,  0},
    {"sitrus", "Sitrus Berry",  25,   4,  0,  0,  0},
    {"pecha",  "Pecha Berry",    0,   5,  0,  0,  0},  /* cures burn */
    {"rawst",  "Rawst Berry",    5,   3,  0,  0,  0},  /* cures par */
    {"leppa",  "Leppa Berry",    0,   2,  0,  0, 10},
    {"wiki",   "Wiki Berry",    15,   3,  1,  0,  0},
    {"mago",   "Mago Berry",    15,   3,  0,  1,  0},
    {"lum",    "Lum Berry",     10,   6,  0,  0,  5},  /* cures all status */
};

static inline const Berry *get_berry(const char *berry_id) {
    for (int i = 0; i < BERRY_COUNT; i++) {
        if (strcmp(BERRY_TABLE[i].id, berry_id) == 0)
            return &BERRY_TABLE[i];
    }
    return NULL;
}

static inline const Berry *get_berry_by_idx(int idx) {
    if (idx < 0 || idx >= BERRY_COUNT) return NULL;
    return &BERRY_TABLE[idx];
}

/*
 * Apply a berry to a pet.
 * Returns a short result string (stored in a static buffer).
 * Requires Pet to be fully defined — include game/state.h before calling.
 */
#ifdef GRUMBLE_STATE_DEFINED
static inline const char *apply_berry(Pet *pet, const char *berry_id) {
    const Berry *b = get_berry(berry_id);
    if (!b) return "No effect";

    static char result[32];
    result[0] = '\0';

    if (b->hp_restore > 0) {
        pet->hp += b->hp_restore;
        if (pet->hp > pet->max_hp) pet->hp = pet->max_hp;
    }
    if (b->happy_bonus > 0) {
        int h = pet->happiness + b->happy_bonus;
        if (h > 100) h = 100;
        pet->happiness = (uint8_t)h;
    }
    if (b->atk_bonus != 0)
        pet->atk = (int8_t)(pet->atk + b->atk_bonus);
    if (b->spd_bonus != 0)
        pet->spd = (int8_t)(pet->spd + b->spd_bonus);
    if (b->pp_restore > 0) {
        for (int i = 0; i < 4; i++) {
            if (pet->move_pp[i] < 30)
                pet->move_pp[i] += b->pp_restore;
        }
    }

    /* Copy berry name as result */
    int n = 0;
    const char *src = b->name;
    while (*src && n < 28) result[n++] = *src++;
    const char *suf = " used!";
    while (*suf && n < 31) result[n++] = *suf++;
    result[n] = '\0';
    return result;
}
#endif /* GRUMBLE_STATE_DEFINED */
