#include "state.h"
#include "hal.h"
#include <string.h>

uint32_t xp_for_level(uint8_t level) {
    return (uint32_t)level * level * 10u + (uint32_t)level * 20u;
}

void state_init(GameState *s) {
    memset(s, 0, sizeof(*s));

    s->mode    = MODE_TOPDOWN;
    s->hp      = STAT_MAX;
    s->max_hp  = STAT_MAX;
    s->energy  = STAT_MAX;

    for (int i = 0; i < SKILL_COUNT; i++)
        s->skills[i].level = 1;

    s->td.x          = 14.0f * TILE + TILE / 2.0f;
    s->td.y          = 10.0f * TILE + TILE / 2.0f;
    s->td.dir        = DIR_DOWN;
    s->td.screen_dir = DIR_DOWN;
    s->td.tile_x     = 14;
    s->td.tile_y     = 10;
}

void state_log(GameState *s, const char *msg) {
    strncpy(s->log_msg, msg, sizeof(s->log_msg) - 1);
    s->log_msg[sizeof(s->log_msg) - 1] = '\0';
    s->log_ms = hal_ticks_ms();
}
