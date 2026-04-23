#pragma once
#include <stdbool.h>
#include <stdint.h>

bool iso_draw_tile_full(uint8_t tile_id, int cx, int cy, uint32_t tick);
bool iso_draw_tile_onlay(uint8_t tile_id, int cx, int cy);
bool iso_draw_depleted_mark(int cx, int cy);
bool iso_draw_td_player_char(int sx, int sy, uint8_t dir, uint8_t walk_frame);
