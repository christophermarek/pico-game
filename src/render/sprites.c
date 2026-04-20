#include "sprites.h"
#include "font.h"
#include "iso_spritesheet.h"
#include "hal.h"
#include "colors.h"
#include "config.h"
#include <stdbool.h>
#include <stdint.h>

static inline void R(int x, int y, int w, int h, uint16_t c) {
    hal_fill_rect(x, y, w, h, c);
}

void spr_iso_depleted_mark(int cx, int cy)
{
    iso_draw_depleted_mark(cx, cy);
}

void spr_tile_iso_floor(uint8_t tile_id, int cx, int cy, uint32_t tick)
{
    iso_draw_tile_full(tile_id, cx, cy, tick);
}

void spr_tile_iso_onlay(uint8_t tile_id, int cx, int cy, uint32_t tick)
{
    (void)tick;
    iso_draw_tile_onlay(tile_id, cx, cy);
}

void spr_tile_iso(uint8_t tile_id, int cx, int cy, uint32_t tick)
{
    spr_tile_iso_floor(tile_id, cx, cy, tick);
    spr_tile_iso_onlay(tile_id, cx, cy, tick);
}

void spr_player_td_foot(int foot_sx, int foot_sy, uint8_t dir, float walk_frame)
{
    (void)walk_frame;
    iso_draw_player_foot(foot_sx, foot_sy, dir);
}

void spr_player_td(int sx, int sy, uint8_t dir, float walk_frame)
{
    iso_draw_td_player_char(sx, sy, dir, (uint8_t)walk_frame);
}

void spr_skill_indicator(int sx, int sy, float progress)
{
    int bar_w = 20;
    int bar_h = 4;
    R(sx - bar_w / 2, sy - 12, bar_w, bar_h, C_BG);
    int fill = (int)(progress * bar_w);
    if (fill > bar_w) fill = bar_w;
    R(sx - bar_w / 2, sy - 12, fill, bar_h, C_XP_PURPLE);
}
