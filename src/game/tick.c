#include "tick.h"
#include "config.h"

void game_tick(GameState *s, World *w) {
    (void)w;
    s->tick_count++;

    if (!s->skilling && s->energy < STAT_MAX)
        s->energy += ENERGY_REGEN_PER_TICK;

    if (s->tick_count % TICKS_PER_DAY == 0)
        s->day++;
}
