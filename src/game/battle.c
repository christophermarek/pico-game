#include "battle.h"
#include "pets.h"
#include "skills.h"
#include "../data/species.h"
#include "../data/moves.h"
#include <string.h>

/* ------------------------------------------------------------------ */
/* RNG                                                                  */
/* ------------------------------------------------------------------ */
static uint32_t rng_state = 99991u;
static uint32_t rng_next(void) {
    rng_state = rng_state * 1664525u + 1013904223u;
    return rng_state;
}

/* ------------------------------------------------------------------ */
/* Log helper                                                           */
/* ------------------------------------------------------------------ */
void battle_add_log(BattleState *b, const char *msg) {
    /* Shift down */
    for (int i = 2; i > 0; i--)
        memcpy(b->log[i], b->log[i - 1], sizeof(b->log[0]));
    strncpy(b->log[0], msg, sizeof(b->log[0]) - 1);
    b->log[0][sizeof(b->log[0]) - 1] = '\0';
    if (b->log_count < 3) b->log_count++;
}

/* ------------------------------------------------------------------ */
/* Damage formula                                                       */
/* ------------------------------------------------------------------ */
int battle_calc_damage(int atk, int def, int power, float tm, int8_t atk_mod) {
    /* Apply stat modifier: each stage is ±50% */
    float mod = 1.0f + atk_mod * 0.5f;
    if (mod < 0.25f) mod = 0.25f;
    if (mod > 4.0f)  mod = 4.0f;
    float eff_atk = (float)atk * mod;
    if (def < 1) def = 1;
    float dmg = (eff_atk * (float)power / (float)def / 50.0f + 2.0f) * tm;
    /* ±15% random variance */
    float var = 0.85f + (float)(rng_next() % 31) / 100.0f;
    dmg *= var;
    if (dmg < 1.0f) dmg = 1.0f;
    return (int)dmg;
}

/* ------------------------------------------------------------------ */
/* Start a wild battle                                                  */
/* ------------------------------------------------------------------ */
void battle_start_wild(GameState *s, uint8_t species_id, uint8_t level) {
    if (s->party_count == 0) return;

    s->prev_mode = s->mode;
    s->mode      = MODE_BATTLE;
    s->total_battles++;

    BattleState *b = &s->battle;
    memset(b, 0, sizeof(*b));

    b->active       = true;
    b->player_turn  = true;
    b->enemy_species = species_id;
    b->enemy_level  = level;
    b->can_catch    = true;

    /* Determine evo stage */
    const Species *sp = get_species(species_id);
    uint8_t evo = 0;
    for (int st = 3; st >= 1; st--) {
        if (sp->evo_levels[st] > 0 && level >= sp->evo_levels[st]) {
            evo = (uint8_t)st;
            break;
        }
    }
    b->enemy_evo = evo;

    calc_stats(species_id, level,
               &b->enemy_max_hp, &b->enemy_atk,
               &b->enemy_def,    &b->enemy_spd);
    b->enemy_hp = b->enemy_max_hp;

    for (int i = 0; i < MOVE_SLOTS; i++) {
        b->enemy_moves[i] = sp->moves_by_evo[evo][i];
        b->enemy_pp[i]    = (b->enemy_moves[i] > 0) ? 20 : 0;
    }

    /* Speed determines first turn */
    Pet *pet = &s->party[s->active_pet];
    b->player_turn = (pet->spd >= b->enemy_spd);

    battle_add_log(b, "A wild monster appeared!");
    b->anim_timer = 0;
}

/* ------------------------------------------------------------------ */
/* Enemy AI: pick highest-power available move                          */
/* ------------------------------------------------------------------ */
static uint8_t enemy_pick_move(BattleState *b) {
    uint8_t best_idx = 0;
    uint8_t best_pow = 0;
    for (int i = 0; i < MOVE_SLOTS; i++) {
        if (b->enemy_moves[i] == 0 || b->enemy_pp[i] == 0) continue;
        const Move *m = get_move(b->enemy_moves[i]);
        if (m->power > best_pow) {
            best_pow = m->power;
            best_idx = (uint8_t)i;
        }
    }
    return best_idx;
}

/* ------------------------------------------------------------------ */
/* Resolve a single attack                                              */
/* ------------------------------------------------------------------ */
typedef enum { ATTACKER_PLAYER, ATTACKER_ENEMY } Attacker;

static void resolve_attack(GameState *s, uint8_t move_id,
                            Attacker attacker) {
    BattleState *b   = &s->battle;
    Pet         *pet = &s->party[s->active_pet];
    const Move  *mv  = get_move(move_id);

    if (mv->power == 0) {
        /* Status move */
        if (attacker == ATTACKER_PLAYER) {
            switch (mv->effect_id) {
                case EFF_ATK_UP:  b->e_atk_mod--; battle_add_log(b, "Enemy atk fell!"); break;
                case EFF_DEF_UP:  pet->b_def_mod++;  battle_add_log(b, "Def rose!"); break;
                case EFF_ATK_DOWN: b->e_atk_mod--;   battle_add_log(b, "Enemy atk fell!"); break;
                case EFF_DEF_DOWN: b->e_def_mod--;   battle_add_log(b, "Enemy def fell!"); break;
                case EFF_SPD_DOWN: b->e_spd_mod--;   battle_add_log(b, "Enemy spd fell!"); break;
                case EFF_HEAL: {
                    int heal = pet->max_hp / 2;
                    pet->hp += (int16_t)heal;
                    if (pet->hp > pet->max_hp) pet->hp = pet->max_hp;
                    battle_add_log(b, "Pet recovered HP!");
                    break;
                }
                default: battle_add_log(b, "..."); break;
            }
        }
        return;
    }

    /* Accuracy check */
    if (mv->acc > 0 && (rng_next() % 100) >= mv->acc) {
        battle_add_log(b, "Attack missed!");
        return;
    }

    /* Type effectiveness */
    const Species *def_sp = (attacker == ATTACKER_PLAYER)
                            ? get_species(b->enemy_species)
                            : get_species(pet->species_id);
    float tm = type_mult(mv->type, def_sp->type);

    int8_t hap_atk = 0;
    uint8_t evade  = 0;
    if (attacker == ATTACKER_PLAYER)
        pet_apply_happiness_combat(pet, &hap_atk, &evade);

    /* Evade chance */
    if (attacker == ATTACKER_ENEMY && evade > 0) {
        if ((rng_next() % 100) < evade) {
            battle_add_log(b, "Pet evaded!");
            return;
        }
    }

    int dmg = 0;
    if (attacker == ATTACKER_PLAYER) {
        int8_t eff_mod = (int8_t)(pet->b_atk_mod + hap_atk);
        dmg = battle_calc_damage(pet->atk, b->enemy_def, mv->power, tm, eff_mod);
        b->enemy_hp -= (int16_t)dmg;
        if (b->enemy_hp < 0) b->enemy_hp = 0;
    } else {
        dmg = battle_calc_damage(b->enemy_atk, pet->def, mv->power, tm, b->e_atk_mod);
        pet->hp -= (int16_t)dmg;
        if (pet->hp < 0) pet->hp = 0;
    }

    /* Log */
    static char dmg_buf[32];
    const char *who = (attacker == ATTACKER_PLAYER) ? "Pet" : "Enemy";
    int n = 0;
    const char *s1 = who;
    while (*s1 && n < 8) dmg_buf[n++] = *s1++;
    const char *s2 = " hit for ";
    while (*s2 && n < 18) dmg_buf[n++] = *s2++;
    /* Simple int to chars */
    if (dmg >= 100) { dmg_buf[n++] = (char)('0' + dmg/100); dmg %= 100; }
    if (dmg >= 10)  { dmg_buf[n++] = (char)('0' + dmg/10);  dmg %= 10; }
    dmg_buf[n++] = (char)('0' + dmg);
    dmg_buf[n++] = '!';
    dmg_buf[n]   = '\0';
    battle_add_log(b, dmg_buf);

    b->anim_timer = 8;
}

/* ------------------------------------------------------------------ */
/* Catch attempt                                                        */
/* ------------------------------------------------------------------ */
bool battle_try_catch(GameState *s) {
    BattleState *b = &s->battle;
    if (!b->can_catch) return false;

    /* Base catch rate: higher when enemy HP is lower */
    int rate = 30 + (int)(70 * (1.0f - (float)b->enemy_hp / (float)b->enemy_max_hp));
    /* Higher evo = harder */
    rate -= b->enemy_evo * 15;
    if (rate < 5) rate = 5;

    if ((int)(rng_next() % 100) < rate) {
        /* Catch success */
        if (s->party_count < PARTY_MAX) {
            Pet *p = &s->party[s->party_count++];
            pet_init(p, b->enemy_species, b->enemy_level);
            p->trail_x = s->td.x;
            p->trail_y = s->td.y;
        } else if (s->box_count < BOX_MAX) {
            Pet *p = &s->box[s->box_count++];
            pet_init(p, b->enemy_species, b->enemy_level);
        } else {
            battle_add_log(b, "Party and box full!");
            return false;
        }
        s->total_catches++;
        battle_add_log(b, "Caught!");
        battle_end(s, true);
        return true;
    }
    battle_add_log(b, "It broke free!");
    return false;
}

/* ------------------------------------------------------------------ */
/* End battle                                                           */
/* ------------------------------------------------------------------ */
void battle_end(GameState *s, bool won) {
    BattleState *b = &s->battle;

    if (won && b->enemy_hp == 0) {
        /* Award XP */
        uint32_t xp = (uint32_t)(b->enemy_level * 5 + b->enemy_evo * 10);
        pet_add_xp(s, s->active_pet, xp);
        skill_add_xp(s, SK_COMBAT, xp / 2);
        state_log(s, "Victory!");
    } else if (!won) {
        state_log(s, "Ran away safely.");
    }

    /* Reset battle mods */
    if (s->party_count > 0) {
        Pet *p = &s->party[s->active_pet];
        p->b_atk_mod = 0;
        p->b_def_mod = 0;
        p->b_spd_mod = 0;
    }

    b->active = false;
    s->mode   = s->prev_mode;
}

/* ------------------------------------------------------------------ */
/* Battle update (input handling)                                       */
/* ------------------------------------------------------------------ */
void battle_update(GameState *s, const Input *inp) {
    BattleState *b   = &s->battle;
    Pet         *pet = (s->party_count > 0) ? &s->party[s->active_pet] : NULL;

    if (!b->active) return;
    if (pet == NULL) { battle_end(s, false); return; }

    /* Animation cooldown */
    if (b->anim_timer > 0) {
        b->anim_timer--;
        return;
    }

    /* Check for battle end conditions */
    if (b->enemy_hp <= 0) { battle_end(s, true);  return; }
    if (pet->hp     <= 0) { battle_end(s, false); return; }

    /* Sub-menus */
    if (b->show_moves) {
        static bool up_prev = false, down_prev = false;
        if (inp->up    && !up_prev)   { if (b->move_cursor > 0)            b->move_cursor--; }
        if (inp->down  && !down_prev) { if (b->move_cursor < MOVE_SLOTS-1) b->move_cursor++; }
        up_prev   = inp->up;
        down_prev = inp->down;

        if (inp->b_press) { b->show_moves = false; return; }
        if (inp->a_press) {
            uint8_t mid = pet->moves[b->move_cursor];
            if (mid == 0 || pet->move_pp[b->move_cursor] == 0) {
                battle_add_log(b, "No PP left!");
                return;
            }
            b->show_moves = false;
            pet->move_pp[b->move_cursor]--;

            /* Player attacks */
            resolve_attack(s, mid, ATTACKER_PLAYER);
            if (b->enemy_hp <= 0) { battle_end(s, true); return; }

            /* Enemy turn */
            b->player_turn = false;
            uint8_t eidx = enemy_pick_move(b);
            uint8_t emid = b->enemy_moves[eidx];
            if (emid > 0 && b->enemy_pp[eidx] > 0) {
                b->enemy_pp[eidx]--;
                resolve_attack(s, emid, ATTACKER_ENEMY);
            }
            if (pet->hp <= 0) { battle_end(s, false); return; }
            b->player_turn = true;
        }
        return;
    }

    if (b->show_bag) {
        static bool b_up_prev = false, b_down_prev = false;
        if (inp->up    && !b_up_prev)   { if (b->move_cursor > 0)  b->move_cursor--; }
        if (inp->down  && !b_down_prev) { if (b->move_cursor < 7)  b->move_cursor++; }
        b_up_prev   = inp->up;
        b_down_prev = inp->down;

        if (inp->b_press) { b->show_bag = false; return; }
        if (inp->a_press) {
            /* Use oran berry if we have one (simplified) */
            if (s->inv[b->move_cursor].count > 0) {
                uint8_t iid = s->inv[b->move_cursor].item_id;
                if (iid == ITEM_ORB) {
                    /* Catch attempt */
                    inv_remove(s, ITEM_ORB, 1);
                    b->show_bag = false;
                    battle_try_catch(s);
                    return;
                } else if (iid >= ITEM_ORAN && iid <= ITEM_LUM) {
                    int bidx = iid - ITEM_ORAN;
                    if (bidx >= 0 && bidx < 8) {
                        /* apply healing */
                        pet->hp += 15;
                        if (pet->hp > pet->max_hp) pet->hp = pet->max_hp;
                        inv_remove(s, iid, 1);
                        battle_add_log(b, "Used berry!");
                    }
                    b->show_bag = false;
                }
            }
        }
        return;
    }

    if (b->show_swap) {
        static bool s_up_prev = false, s_down_prev = false;
        if (inp->up    && !s_up_prev)   { if (b->move_cursor > 0)            b->move_cursor--; }
        if (inp->down  && !s_down_prev) { if (b->move_cursor < s->party_count - 1) b->move_cursor++; }
        s_up_prev   = inp->up;
        s_down_prev = inp->down;

        if (inp->b_press) { b->show_swap = false; return; }
        if (inp->a_press) {
            if (b->move_cursor != s->active_pet &&
                b->move_cursor < s->party_count) {
                s->active_pet = b->move_cursor;
                battle_add_log(b, "Swapped pet!");
            }
            b->show_swap = false;
        }
        return;
    }

    /* Main menu navigation */
    static bool m_left_prev = false, m_right_prev = false;
    if (inp->left  && !m_left_prev)  { if (b->menu_cursor > 0) b->menu_cursor--; }
    if (inp->right && !m_right_prev) { if (b->menu_cursor < 3) b->menu_cursor++; }
    m_left_prev  = inp->left;
    m_right_prev = inp->right;

    if (inp->a_press) {
        b->move_cursor = 0;
        switch (b->menu_cursor) {
            case 0: b->show_moves = true; break;
            case 1: b->show_bag   = true; break;
            case 2: b->show_swap  = true; break;
            case 3:
                battle_end(s, false);
                break;
        }
    }
}
