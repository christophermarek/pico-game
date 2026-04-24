#include "state.h"
#include "items.h"
#include "hal.h"
#include <string.h>

/*
 * OSRS XP curve, levels 1..99. Reference formula:
 *   xp(L) = floor( sum_{i=1}^{L-1} floor(i + 300 * 2^(i/7)) / 4 )
 * Precomputed so runtime is a 4-byte table lookup, no float math.
 */
static const uint32_t XP_TABLE[99] = {
             0u,         83u,        174u,        276u,        388u,        512u,        650u,        801u,
           969u,       1154u,       1358u,       1584u,       1833u,       2107u,       2411u,       2746u,
          3115u,       3523u,       3973u,       4470u,       5018u,       5624u,       6291u,       7028u,
          7842u,       8740u,       9730u,      10824u,      12031u,      13363u,      14833u,      16456u,
         18247u,      20224u,      22406u,      24815u,      27473u,      30408u,      33648u,      37224u,
         41171u,      45529u,      50339u,      55649u,      61512u,      67983u,      75127u,      83014u,
         91721u,     101333u,     111945u,     123660u,     136594u,     150872u,     166636u,     184040u,
        203254u,     224466u,     247886u,     273742u,     302288u,     333804u,     368599u,     407015u,
        449428u,     496254u,     547953u,     605032u,     668051u,     737627u,     814445u,     899257u,
        992895u,    1096278u,    1210421u,    1336443u,    1475581u,    1629200u,    1798808u,    1986068u,
       2192818u,    2421087u,    2673114u,    2951373u,    3258594u,    3597792u,    3972294u,    4385776u,
       4842295u,    5346332u,    5902831u,    6517253u,    7195629u,    7944614u,    8771558u,    9684577u,
      10692629u,   11805606u,   13034431u,
};

uint32_t xp_for_level(uint8_t level) {
    if (level < 1)  level = 1;
    if (level > SKILL_LEVEL_CAP) level = SKILL_LEVEL_CAP;
    return XP_TABLE[level - 1u];
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
