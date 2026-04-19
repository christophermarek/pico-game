#pragma once
#include <stdint.h>
#include "../game/state.h"
#include "../game/world.h"

/* Tile drawing */
void spr_tile(uint8_t tile_id, int sx, int sy, uint32_t tick);
void spr_tile_iso(uint8_t tile_id, int cx, int cy, uint32_t tick);
void spr_tile_iso_floor(uint8_t tile_id, int cx, int cy, uint32_t tick);
void spr_tile_iso_onlay(uint8_t tile_id, int cx, int cy, uint32_t tick);
void spr_iso_depleted_mark(int cx, int cy);

void spr_mansion_td(const GameState *s, const World *w);

void spr_player_td(int sx, int sy, uint8_t dir, float walk_frame,
                   uint8_t skin_idx, uint8_t hair_idx, uint8_t outfit_idx);
void spr_player_td_foot(int foot_sx, int foot_sy, uint8_t dir, float walk_frame,
                        uint8_t skin_idx, uint8_t hair_idx, uint8_t outfit_idx);
void spr_player_sv(int sx, int sy, uint8_t dir, float walk_frame, bool jumping,
                   uint8_t skin_idx, uint8_t hair_idx, uint8_t outfit_idx);

/* Monsters */
void spr_monster(int sx, int sy, uint8_t evo_stage, float walk_frame);
void spr_monster_foot(int foot_sx, int foot_sy, uint8_t evo_stage, float walk_frame);

/* Furniture */
void spr_furniture(FurnitureType type, int sx, int sy);

/* HUD elements */
void spr_xp_drop(int x, int y, uint16_t amount);
void spr_skill_indicator(int sx, int sy, float progress);
