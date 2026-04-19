#include "pets.h"
#include "../data/species.h"
#include "../data/equipment.h"
#include <string.h>

/* ------------------------------------------------------------------ */
/* Initialise a pet from species data                                   */
/* ------------------------------------------------------------------ */
void pet_init(Pet *p, uint8_t species_id, uint8_t level) {
    memset(p, 0, sizeof(*p));
    const Species *sp = get_species(species_id);
    p->species_id = species_id;
    p->evo_stage  = 0;
    p->level      = level;
    p->happiness  = 60;

    /* Determine evo stage from level */
    for (int stage = 3; stage >= 1; stage--) {
        if (sp->evo_levels[stage] > 0 && level >= sp->evo_levels[stage]) {
            p->evo_stage = (uint8_t)stage;
            break;
        }
    }

    calc_stats(species_id, level, &p->max_hp, &p->atk, &p->def, &p->spd);
    p->hp = p->max_hp;

    pet_learn_moves(p, p->evo_stage);
}

/* ------------------------------------------------------------------ */
/* Learn moves for a given evo stage                                   */
/* ------------------------------------------------------------------ */
void pet_learn_moves(Pet *p, uint8_t evo_stage) {
    const Species *sp = get_species(p->species_id);
    if (evo_stage > 3) evo_stage = 3;
    for (int i = 0; i < MOVE_SLOTS; i++) {
        p->moves[i]   = sp->moves_by_evo[evo_stage][i];
        p->move_pp[i] = (p->moves[i] > 0) ? 30 : 0;
    }
}

/* ------------------------------------------------------------------ */
/* XP → level helper                                                    */
/* ------------------------------------------------------------------ */
static uint8_t pet_xp_to_level(uint32_t xp) {
    uint8_t lv = 1;
    while (lv < 99 && xp >= xp_for_level((uint8_t)(lv + 1)))
        lv++;
    return lv;
}

/* ------------------------------------------------------------------ */
/* Add XP to a party pet                                                */
/* ------------------------------------------------------------------ */
void pet_add_xp(GameState *s, uint8_t party_idx, uint32_t xp) {
    if (party_idx >= s->party_count) return;
    Pet *p = &s->party[party_idx];
    uint8_t old_level = p->level;
    p->xp += xp;
    p->level = pet_xp_to_level(p->xp);
    if (p->level > old_level) {
        /* Recalc stats */
        int16_t old_max = p->max_hp;
        calc_stats(p->species_id, p->level, &p->max_hp, &p->atk, &p->def, &p->spd);
        p->hp += (p->max_hp - old_max); /* gain HP diff */
        if (p->hp > p->max_hp) p->hp = p->max_hp;
        state_log(s, "Pet levelled up!");
        pet_check_evo(s, party_idx);
    }
}

/* ------------------------------------------------------------------ */
/* Check and perform evolution                                          */
/* ------------------------------------------------------------------ */
void pet_check_evo(GameState *s, uint8_t party_idx) {
    if (party_idx >= s->party_count) return;
    Pet *p = &s->party[party_idx];
    const Species *sp = get_species(p->species_id);
    if (p->evo_stage >= 3) return;

    uint8_t next = (uint8_t)(p->evo_stage + 1);
    if (sp->evo_levels[next] > 0 && p->level >= sp->evo_levels[next]) {
        p->evo_stage = next;
        pet_learn_moves(p, next);

        char msg[32];
        const char *pname = sp->evo_names[next];
        int n = 0;
        while (*pname && n < 20) msg[n++] = *pname++;
        const char *suf = " evolved!";
        while (*suf && n < 30) msg[n++] = *suf++;
        msg[n] = '\0';
        state_log(s, msg);
    }
}

/* ------------------------------------------------------------------ */
/* Happiness combat modifiers                                           */
/* ------------------------------------------------------------------ */
void pet_apply_happiness_combat(const Pet *p, int8_t *atk_bonus, uint8_t *evade_chance) {
    *atk_bonus    = 0;
    *evade_chance = 0;
    if (p->happiness >= 80) {
        *atk_bonus    = 2;
        *evade_chance = 5;
    } else if (p->happiness >= 50) {
        *atk_bonus = 0;
    } else if (p->happiness >= 30) {
        *atk_bonus = -1;
    } else {
        *atk_bonus = -3;
    }
}

/* ------------------------------------------------------------------ */
/* Equip an item                                                        */
/* ------------------------------------------------------------------ */
void pet_equip(Pet *p, uint8_t equip_slot) {
    p->equip_id = equip_slot;
    const Equipment *e = get_equipment(equip_slot);
    if (!e) return;
    /* Apply happiness bonus */
    int h = p->happiness + e->happy_bonus;
    if (h > 100) h = 100;
    if (h < 0)   h = 0;
    p->happiness = (uint8_t)h;
}

/* ------------------------------------------------------------------ */
/* Auto-recruit at total level milestones                               */
/* ------------------------------------------------------------------ */
void pets_check_recruit(GameState *s) {
    uint16_t tl = total_level(s);
    /* At 15: recruit Bramble (sp=1) */
    /* At 30: recruit Korvax (sp=2) */
    /* At 50: recruit Shadowkin (sp=4) */
    static const uint16_t thresholds[3] = {15, 30, 50};
    static const uint8_t  species[3]    = {1, 2, 4};
    static bool recruited[3] = {false, false, false};

    for (int i = 0; i < 3; i++) {
        if (!recruited[i] && tl >= thresholds[i]) {
            if (s->party_count < PARTY_MAX) {
                Pet *p = &s->party[s->party_count++];
                pet_init(p, species[i], (uint8_t)(thresholds[i] / 3));
                p->trail_x = s->td.x;
                p->trail_y = s->td.y;
                state_log(s, "A new friend joined!");
                recruited[i] = true;
            }
        }
    }
}
