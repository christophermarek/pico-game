#include "tick.h"
#include "config.h"

void game_tick(GameState *s, World *w) {
    s->tick_count++;

    /* Energy regen when not skilling */
    if (!s->skilling) {
        if (s->energy < STAT_MAX) s->energy += ENERGY_REGEN_PER_TICK;
    }

    /* Day counter */
    if (s->tick_count % TICKS_PER_DAY == 0)
        s->day++;

    /* World node respawns */
    world_tick(w);
}
