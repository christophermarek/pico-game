#pragma once
#include <stdint.h>
#include "state.h"
#include "world.h"

typedef struct {
    const char *name;
    const char *icon_str;
} SkillInfo;

extern const SkillInfo SKILL_INFO[SKILL_COUNT];

void skill_add_xp(GameState *s, uint8_t skill_id, uint32_t xp);
void skill_complete_action(GameState *s, World *w);
