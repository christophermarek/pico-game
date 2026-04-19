#pragma once
#include <stdint.h>
#include "state.h"
#include "world.h"

typedef struct {
    const char *name;
    const char *icon_str;
} SkillInfo;

extern const SkillInfo SKILL_INFO[SKILL_COUNT];

/* Add XP to skill; returns true if levelled up. Also awards pet XP. */
void skill_add_xp(GameState *s, uint8_t skill_id, uint32_t xp);

/* Called when action_ticks_left reaches 0 */
void skill_complete_action(GameState *s, World *w);

/* Returns a random item_id appropriate for the node type */
uint8_t skill_get_drop(uint8_t tile_id);
