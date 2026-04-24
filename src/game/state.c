#include "state.h"
#include "items.h"
#include "hal.h"
#include <string.h>

uint32_t xp_for_level(uint8_t level) {
    return (uint32_t)level * level * 10u + (uint32_t)level * 20u;
}

void state_init(GameState *s) {
    memset(s, 0, sizeof(*s));

    s->mode        = MODE_TOPDOWN;
    s->hp          = STAT_MAX;
    s->max_hp      = STAT_MAX;
    s->energy      = STAT_MAX;
    s->active_slot = TOOL_SLOT_START; /* axe by default */

    for (int i = 0; i < SKILL_COUNT; i++)
        s->skills[i].level = 1;

    inventory_init_default(&s->inv);

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

void state_spawn_item_fly(GameState *s, float wx, float wy, item_id_t item, int slot) {
    for (int i = 0; i < MAX_ITEM_FLIES; i++) {
        if (!s->item_flies[i].active) {
            s->item_flies[i].wx     = wx;
            s->item_flies[i].wy     = wy;
            s->item_flies[i].slot   = (int8_t)slot;
            s->item_flies[i].item   = item;
            s->item_flies[i].timer  = ITEM_FLY_FRAMES;
            s->item_flies[i].active = true;
            return;
        }
    }
    /* Pool full — drop silently. */
}

void state_anim_tick(GameState *s) {
    /* Decrement slot flashes BEFORE processing flies so a just-landed fly
     * gets its full SLOT_FLASH_FRAMES visible, not SLOT_FLASH_FRAMES-1. */
    for (int i = 0; i < HOTBAR_SLOTS; i++) {
        if (s->slot_flash[i] > 0) s->slot_flash[i]--;
    }
    for (int i = 0; i < MAX_ITEM_FLIES; i++) {
        ItemFly *fly = &s->item_flies[i];
        if (!fly->active) continue;
        if (fly->timer > 0) fly->timer--;
        if (fly->timer == 0) {
            fly->active = false;
            int sl = (int)fly->slot;
            if (sl >= 0 && sl < HOTBAR_SLOTS)
                s->slot_flash[sl] = SLOT_FLASH_FRAMES;
        }
    }
}
