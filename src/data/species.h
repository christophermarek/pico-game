#pragma once
#include <stdint.h>

/*
 * Monster species definitions (reserved for future pet system).
 * evo_levels[stage] = level required to reach that stage (stage 0 = base)
 */
typedef struct {
    uint8_t  id;
    char     name[12];
    char     evo_names[4][12];
    uint8_t  base_hp, base_atk, base_def, base_spd;
    uint8_t  grow_hp, grow_atk, grow_def, grow_spd;
    char     type[12];
    uint8_t  evo_levels[4];
} Species;

/* Species IDs */
#define SP_GLUB      0
#define SP_BRAMBLE   1
#define SP_KORVAX    2
#define SP_WISP      3
#define SP_SHADOWKIN 4
#define SPECIES_COUNT 5

static const Species SPECIES_TABLE[SPECIES_COUNT] = {
    /* 0: Glub — dark type */
    {
        .id        = SP_GLUB,
        .name      = "Glub",
        .evo_names = {"Glub", "Glubmaw", "Glublord", "Abysgore"},
        .base_hp   = 30, .base_atk = 12, .base_def = 10, .base_spd = 11,
        .grow_hp   =  5, .grow_atk =  2, .grow_def =  2, .grow_spd =  2,
        .type      = "dark",
        .evo_levels = {0, 16, 32, 48},
    },
    /* 1: Bramble — grass type */
    {
        .id        = SP_BRAMBLE,
        .name      = "Bramble",
        .evo_names = {"Bramble", "Thornling", "Briarclaw", "Verdantwrath"},
        .base_hp   = 28, .base_atk = 11, .base_def = 13, .base_spd = 10,
        .grow_hp   =  5, .grow_atk =  2, .grow_def =  3, .grow_spd =  2,
        .type      = "grass",
        .evo_levels = {0, 16, 32, 48},
    },
    /* 2: Korvax — fire type */
    {
        .id        = SP_KORVAX,
        .name      = "Korvax",
        .evo_names = {"Korvax", "Emberclaw", "Scorchfang", "Infernolord"},
        .base_hp   = 26, .base_atk = 15, .base_def =  9, .base_spd = 13,
        .grow_hp   =  4, .grow_atk =  3, .grow_def =  2, .grow_spd =  3,
        .type      = "fire",
        .evo_levels = {0, 16, 32, 48},
    },
    /* 3: Wisp — electric type */
    {
        .id        = SP_WISP,
        .name      = "Wisp",
        .evo_names = {"Wisp", "Sparklet", "Voltghost", "Stormwraith"},
        .base_hp   = 24, .base_atk = 13, .base_def =  8, .base_spd = 16,
        .grow_hp   =  4, .grow_atk =  2, .grow_def =  2, .grow_spd =  3,
        .type      = "electric",
        .evo_levels = {0, 16, 32, 48},
    },
    /* 4: Shadowkin — dark type */
    {
        .id        = SP_SHADOWKIN,
        .name      = "Shadowkin",
        .evo_names = {"Shadowkin", "Darkspawn", "Voidwalker", "Shadowlord"},
        .base_hp   = 28, .base_atk = 14, .base_def = 12, .base_spd = 12,
        .grow_hp   =  5, .grow_atk =  3, .grow_def =  2, .grow_spd =  2,
        .type      = "dark",
        .evo_levels = {0, 18, 36, 54},
    },
};

static inline const Species *get_species(uint8_t id) {
    if (id >= SPECIES_COUNT) id = 0;
    return &SPECIES_TABLE[id];
}

static inline void calc_stats(uint8_t species_id, uint8_t level,
                               int16_t *out_hp, int8_t *out_atk,
                               int8_t *out_def, int8_t *out_spd) {
    const Species *sp = get_species(species_id);
    if (out_hp)  *out_hp  = (int16_t)(sp->base_hp  + sp->grow_hp  * level);
    if (out_atk) *out_atk = (int8_t) (sp->base_atk + sp->grow_atk * level);
    if (out_def) *out_def = (int8_t) (sp->base_def + sp->grow_def * level);
    if (out_spd) *out_spd = (int8_t) (sp->base_spd + sp->grow_spd * level);
}
