#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "../game/items.h"

bool iso_draw_tile_full(uint8_t tile_id, int cx, int cy, uint32_t tick);
bool iso_draw_tile_onlay(uint8_t tile_id, int cx, int cy);
bool iso_draw_depleted_mark(int cx, int cy);
bool iso_draw_td_player_char(int sx, int sy, uint8_t dir, uint8_t walk_frame);
bool iso_draw_item_icon(item_id_t id, int sx, int sy);
bool iso_draw_item_icon_scaled(item_id_t id, int sx, int sy, int sn, int sd);
