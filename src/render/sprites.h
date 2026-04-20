#pragma once
#include <stdint.h>
#include "../game/state.h"
#include "../game/world.h"

void spr_tile_iso(uint8_t tile_id, int cx, int cy, uint32_t tick);
void spr_tile_iso_floor(uint8_t tile_id, int cx, int cy, uint32_t tick);
void spr_tile_iso_onlay(uint8_t tile_id, int cx, int cy, uint32_t tick);
void spr_iso_depleted_mark(int cx, int cy);

void spr_player_td(int sx, int sy, uint8_t dir, float walk_frame);
void spr_player_td_foot(int foot_sx, int foot_sy, uint8_t dir, float walk_frame);

void spr_skill_indicator(int sx, int sy, float progress);
