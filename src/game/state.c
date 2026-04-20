#include "state.h"
#include "hal.h"
#include <string.h>

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

    s->mode     = MODE_TOPDOWN;
    s->hp       = 100;
    s->max_hp   = 100;
    s->energy   = 100;
    s->running_locked = false;

    for (int i = 0; i < SKILL_COUNT; i++) {
        s->skills[i].level = 1;
        s->skills[i].xp    = 0;
    }

    s->td.x        = 14.0f * TILE + TILE / 2.0f;
    s->td.y        = 10.0f * TILE + TILE / 2.0f;
    s->td.dir        = DIR_DOWN;
    s->td.screen_dir = DIR_DOWN;
    s->td.tile_x   = 14;
    s->td.tile_y   = 10;

    s->inv[0].item_id = ITEM_BREAD; s->inv[0].count = START_INV_BREAD_COUNT;
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
    for (int i = 3; i > 0; i--)
        memcpy(s->log[i], s->log[i - 1], sizeof(s->log[0]));
    strncpy(s->log[0], msg, sizeof(s->log[0]) - 1);
    s->log[0][sizeof(s->log[0]) - 1] = '\0';
    if (s->log_count < 4) s->log_count++;
    s->log_ms = hal_ticks_ms();
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
    for (int i = 0; i < INV_SLOTS; i++) {
        if (s->inv[i].item_id == item_id) {
            uint16_t sum = (uint16_t)s->inv[i].count + qty;
            s->inv[i].count = (sum > 255u) ? 255u : (uint8_t)sum;
            return true;
        }
    }
    for (int i = 0; i < INV_SLOTS; i++) {
        if (s->inv[i].item_id == ITEM_NONE || s->inv[i].count == 0) {
            s->inv[i].item_id = item_id;
            s->inv[i].count   = qty;
            return true;
        }
    }
    return false;
}

bool inv_remove(GameState *s, uint8_t item_id, uint8_t qty) {
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
