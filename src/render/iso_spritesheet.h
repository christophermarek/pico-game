#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "../game/items.h"

/*
 * Orientation of an icon when drawn. Source art is authored right-facing;
 * the other three orientations are derived by mirroring or rotating at
 * read time — no extra atlas columns needed.
 *
 *   ICON_ORIENT_R   right-facing (atlas default)
 *   ICON_ORIENT_L   mirror of R — blade points left
 *   ICON_ORIENT_D   R rotated 90° clockwise — blade points down
 *   ICON_ORIENT_U   R rotated 90° counter-clockwise — blade points up
 *
 * Only works cleanly for square frames (all our 16×16 items).
 */
typedef enum {
    ICON_ORIENT_R = 0,
    ICON_ORIENT_L,
    ICON_ORIENT_D,
    ICON_ORIENT_U,
} IconOrient;

bool iso_draw_tile_full(uint8_t tile_id, int cx, int cy, uint32_t tick);
bool iso_draw_tile_onlay(uint8_t tile_id, int cx, int cy);
bool iso_draw_td_player_char(int sx, int sy, uint8_t dir, uint8_t walk_frame);
bool iso_draw_npc(uint8_t npc_kind, int sx, int sy);
bool iso_draw_item_icon(item_id_t id, int sx, int sy);
bool iso_draw_item_icon_scaled(item_id_t id, int sx, int sy,
                               int sn, int sd, IconOrient orient);

/* Small HUD glyphs — 8x8, edit via assets/sprites/ui/ PNGs. */
typedef enum {
    UI_ICON_HEART = 0,
    UI_ICON_BOLT,
    UI_ICON_COUNT,
} UiIcon;

bool iso_draw_ui_icon(UiIcon icon, int sx, int sy);
