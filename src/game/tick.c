#include "tick.h"
#include "pets.h"
#include "../data/equipment.h"

void game_tick(GameState *s, World *w) {
    s->tick_count++;

    /* Hunger decay */
    if (s->hunger >= 2)
        s->hunger -= 2;
    else
        s->hunger = 0;

    /* HP damage from starvation */
    if (s->hunger == 0) {
        if (s->hp >= 3) s->hp -= 3;
        else            s->hp  = 0;
    }

    /* Energy regen when not skilling */
    if (!s->skilling) {
        if (s->energy < 100) s->energy++;
    }

    /* Per-pet tick */
    for (int i = 0; i < s->party_count; i++) {
        Pet *p = &s->party[i];

        /* Happiness decay */
        if (p->happiness > 0) p->happiness--;

        /* Equipment happy_per_tick bonus */
        const Equipment *e = get_equipment(p->equip_id);
        if (e && e->happy_per_tick > 0) {
            int h = p->happiness + e->happy_per_tick;
            if (h > 100) h = 100;
            p->happiness = (uint8_t)h;
        }

        /* Farm berries check */
        /* (simple: just decay grow_ticks) */
    }

    /* Farm patches */
    for (int i = 0; i < 3; i++) {
        FarmPatch *fp = &s->farm[i];
        if (fp->berry_id > 0 && !fp->ready) {
            if (fp->grow_ticks > 0) fp->grow_ticks--;
            else                     fp->ready = true;
        }
    }

    /* Day counter — every 20 ticks */
    if (s->tick_count % 20 == 0)
        s->day++;

    /* World node respawns */
    world_tick(w);

    /* XP drop timers */
    for (int i = 0; i < s->xp_drop_count; i++) {
        if (s->xp_drops[i].timer > 0)
            s->xp_drops[i].timer--;
    }
    /* Compact expired drops */
    {
        int out = 0;
        for (int i = 0; i < s->xp_drop_count; i++) {
            if (s->xp_drops[i].timer > 0)
                s->xp_drops[out++] = s->xp_drops[i];
        }
        s->xp_drop_count = (uint8_t)out;
    }

    /* Check new recruits */
    pets_check_recruit(s);
}
