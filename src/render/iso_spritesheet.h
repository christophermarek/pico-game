#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "../game/state.h"

void iso_draw_player_foot(int foot_sx, int foot_sy, uint8_t dir, uint8_t skin_idx,
                          uint8_t hair_idx, uint8_t outfit_idx);
void iso_draw_monster_foot(int foot_sx, int foot_sy, uint8_t evo_stage, uint8_t frame);

/* Top-down iso terrain — assets/assets_iso_tiles.png (see assets_iso_sheets.md). */
bool iso_draw_tile_full(uint8_t tile_id, int cx, int cy, uint32_t tick);
bool iso_tile_png_covers_onlay(uint8_t tile_id);

bool iso_draw_mansion(int anchor_sx, int anchor_sy, uint8_t bearing);

/* Side-view and top-down character/monster sprites — assets/assets_chars.png */
bool iso_draw_sv_player(int sx, int sy, uint8_t dir, uint8_t walk_frame);
bool iso_draw_sv_tile(int sx, int sy, uint8_t tile_id);
bool iso_draw_td_player_char(int sx, int sy, uint8_t dir, uint8_t walk_frame);
bool iso_draw_monster_sm(int sx, int sy, uint8_t evo_stage, uint8_t walk_frame);
bool iso_draw_monster_lg(int sx, int sy, uint8_t evo_stage);

/* Furniture — assets/assets_base.png */
bool iso_draw_furniture(int sx, int sy, uint8_t furn_type);

/* Iso depleted-node overlay — col 12 of assets/assets_iso_tiles.png */
bool iso_draw_depleted_mark(int cx, int cy);
