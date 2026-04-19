#include "state.h"
#include <string.h>
#include <stddef.h>

/* ------------------------------------------------------------------ */
/* XP table                                                             */
/* ------------------------------------------------------------------ */
uint32_t xp_for_level(uint8_t level) {
    return (uint32_t)level * level * 10u + (uint32_t)level * 20u;
}

/* ------------------------------------------------------------------ */
/* Initialise game state to defaults                                    */
/* ------------------------------------------------------------------ */
void state_init(GameState *s) {
    memset(s, 0, sizeof(*s));

    s->mode     = MODE_CHAR_CREATE;
    s->hp       = 100;
    s->max_hp   = 100;
    s->hunger   = 100;
    s->energy   = 100;
    s->running_locked = false;

    /* All skills at level 1 */
    for (int i = 0; i < SKILL_COUNT; i++) {
        s->skills[i].level = 1;
        s->skills[i].xp    = 0;
    }

    /* Starting position (top-down, near HOME tile) */
    s->td.x        = 14.0f * TILE + TILE / 2.0f;
    s->td.y        = 10.0f * TILE + TILE / 2.0f;
    s->td.dir        = DIR_DOWN;
    s->td.screen_dir = DIR_DOWN;
    s->td.tile_x   = 14;
    s->td.tile_y   = 10;

    /* Side-view starting position */
    s->sv.x        = TILE * 2.0f;
    s->sv.y        = (float)(7 * TILE);  /* just above ground */
    s->sv.dir      = DIR_RIGHT;

    /* Default player name */
    strncpy(s->custom.name, "Hero", sizeof(s->custom.name) - 1);

    /* Starting pet: Glub level 5 */
    {
        Pet *p       = &s->party[0];
        p->species_id = 0;    /* SP_GLUB */
        p->evo_stage  = 0;
        p->level      = 5;
        p->happiness  = 60;
        p->max_hp     = (int16_t)(30 + 5 * 5);   /* base_hp + grow_hp * level */
        p->hp         = p->max_hp;
        p->atk        = (int8_t)(12 + 2 * 5);
        p->def        = (int8_t)(10 + 2 * 5);
        p->spd        = (int8_t)(11 + 2 * 5);
        p->moves[0]   = 1;   /* Tackle */
        p->moves[1]   = 2;   /* Shadow Bite */
        p->moves[2]   = 0;
        p->moves[3]   = 0;
        p->move_pp[0] = 35;
        p->move_pp[1] = 25;
        p->trail_x    = s->td.x;
        p->trail_y    = s->td.y;
    }
    s->party_count = 1;
    s->active_pet  = 0;

    /* Starting inventory: 3 bread, 3 orbs */
    s->inv[0].item_id = ITEM_BREAD; s->inv[0].count = 3;
    s->inv[1].item_id = ITEM_ORB;   s->inv[1].count = 3;
}

/* ------------------------------------------------------------------ */
/* Sum all skill levels                                                 */
/* ------------------------------------------------------------------ */
uint16_t total_level(const GameState *s) {
    uint16_t total = 0;
    for (int i = 0; i < SKILL_COUNT; i++)
        total += s->skills[i].level;
    return total;
}

/* ------------------------------------------------------------------ */
/* Prepend message to log ring buffer                                   */
/* ------------------------------------------------------------------ */
void state_log(GameState *s, const char *msg) {
    /* Shift entries down */
    for (int i = 3; i > 0; i--)
        memcpy(s->log[i], s->log[i - 1], sizeof(s->log[0]));
    strncpy(s->log[0], msg, sizeof(s->log[0]) - 1);
    s->log[0][sizeof(s->log[0]) - 1] = '\0';
    if (s->log_count < 4) s->log_count++;
}

/* ------------------------------------------------------------------ */
/* Spawn a floating XP drop effect                                      */
/* ------------------------------------------------------------------ */
void state_add_xp_drop(GameState *s, int x, int y, uint16_t amount) {
    if (s->xp_drop_count >= 8) return;
    XpDrop *d = &s->xp_drops[s->xp_drop_count++];
    d->x      = x;
    d->y      = y;
    d->amount = amount;
    d->timer  = 40;
}

/* ------------------------------------------------------------------ */
/* Inventory helpers                                                    */
/* ------------------------------------------------------------------ */
int inv_count(const GameState *s, uint8_t item_id) {
    int total = 0;
    for (int i = 0; i < INV_SLOTS; i++) {
        if (s->inv[i].item_id == item_id)
            total += s->inv[i].count;
    }
    return total;
}

bool inv_add(GameState *s, uint8_t item_id, uint8_t qty) {
    /* Try to stack onto existing slot */
    for (int i = 0; i < INV_SLOTS; i++) {
        if (s->inv[i].item_id == item_id) {
            s->inv[i].count += qty;
            return true;
        }
    }
    /* Find empty slot */
    for (int i = 0; i < INV_SLOTS; i++) {
        if (s->inv[i].item_id == ITEM_NONE || s->inv[i].count == 0) {
            s->inv[i].item_id = item_id;
            s->inv[i].count   = qty;
            return true;
        }
    }
    return false; /* inventory full */
}

bool inv_remove(GameState *s, uint8_t item_id, uint8_t qty) {
    /* Check we have enough */
    if (inv_count(s, item_id) < (int)qty) return false;
    uint8_t remaining = qty;
    for (int i = 0; i < INV_SLOTS && remaining > 0; i++) {
        if (s->inv[i].item_id == item_id) {
            if (s->inv[i].count <= remaining) {
                remaining -= s->inv[i].count;
                s->inv[i].count   = 0;
                s->inv[i].item_id = ITEM_NONE;
            } else {
                s->inv[i].count -= remaining;
                remaining = 0;
            }
        }
    }
    return true;
}
