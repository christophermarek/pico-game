#include "sprites.h"
#include "font.h"
#include "iso_spritesheet.h"
#include "hal.h"
#include "colors.h"
#include "config.h"
#include "../game/td_cam.h"
#include "../game/world.h"
#include <stdbool.h>
#include <stdint.h>

static inline void R(int x, int y, int w, int h, uint16_t c) {
    hal_fill_rect(x, y, w, h, c);
}

/* ------------------------------------------------------------------ */
/* Tile sprites — side-view                                             */
/* ------------------------------------------------------------------ */
void spr_tile(uint8_t tile_id, int sx, int sy, uint32_t tick)
{
    (void)tick;
    iso_draw_sv_tile(sx, sy, tile_id);
}

/* ------------------------------------------------------------------ */
/* Tile sprites — isometric                                             */
/* ------------------------------------------------------------------ */
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

/* ------------------------------------------------------------------ */
/* Mansion                                                              */
/* ------------------------------------------------------------------ */
void spr_mansion_td(const GameState *s, const World *w)
{
    (void)w;
    TdCamBasis cam = td_cam_basis(s->td_cam_bearing);

    const float MV_CX =
        (float)(MANSION_TX0 * TILE) + (float)(MANSION_TW * TILE) * 0.5f;
    const float MV_CY =
        (float)(MANSION_TY0 * TILE) +
        (float)((MANSION_TH - MANSION_PORCH_ROWS) * TILE) * 0.5f;

    int anchor_sx, anchor_sy;
    td_basis_world_pixel_to_screen(&cam, s->td.x, s->td.y,
                                   MV_CX, MV_CY, &anchor_sx, &anchor_sy);
    iso_draw_mansion(anchor_sx, anchor_sy, s->td_cam_bearing);
}

/* ------------------------------------------------------------------ */
/* Player sprites                                                       */
/* ------------------------------------------------------------------ */
void spr_player_td_foot(int foot_sx, int foot_sy, uint8_t dir, float walk_frame,
                        uint8_t skin_idx, uint8_t hair_idx, uint8_t outfit_idx)
{
    (void)walk_frame;
    iso_draw_player_foot(foot_sx, foot_sy, dir, skin_idx, hair_idx, outfit_idx);
}

void spr_player_td(int sx, int sy, uint8_t dir, float walk_frame,
                   uint8_t skin_idx, uint8_t hair_idx, uint8_t outfit_idx)
{
    (void)skin_idx; (void)hair_idx; (void)outfit_idx;
    iso_draw_td_player_char(sx, sy, dir, (uint8_t)walk_frame);
}

void spr_player_sv(int sx, int sy, uint8_t dir, float walk_frame, bool jumping,
                   uint8_t skin_idx, uint8_t hair_idx, uint8_t outfit_idx)
{
    (void)skin_idx; (void)hair_idx; (void)outfit_idx;
    uint8_t eff_frame = jumping ? 1u : (uint8_t)walk_frame;
    iso_draw_sv_player(sx, sy, dir, eff_frame);
}

/* ------------------------------------------------------------------ */
/* Monster sprites                                                      */
/* ------------------------------------------------------------------ */
void spr_monster_foot(int foot_sx, int foot_sy, uint8_t evo_stage, float walk_frame)
{
    iso_draw_monster_foot(foot_sx, foot_sy, evo_stage, (uint8_t)walk_frame);
}

void spr_monster(int sx, int sy, uint8_t evo_stage, float walk_frame)
{
    iso_draw_monster_sm(sx, sy, evo_stage, (uint8_t)walk_frame);
}

/* ------------------------------------------------------------------ */
/* Furniture sprites                                                    */
/* ------------------------------------------------------------------ */
void spr_furniture(FurnitureType type, int sx, int sy)
{
    iso_draw_furniture(sx, sy, (uint8_t)type);
}

/* ------------------------------------------------------------------ */
/* XP drop floating text                                               */
/* ------------------------------------------------------------------ */
void spr_xp_drop(int x, int y, uint16_t amount)
{
    char buf[10];
    int n = 0;
    buf[n++] = '+';
    if (amount >= 1000) buf[n++] = (char)('0' + amount / 1000);
    if (amount >= 100)  buf[n++] = (char)('0' + (amount / 100) % 10);
    if (amount >= 10)   buf[n++] = (char)('0' + (amount /  10) % 10);
    buf[n++] = (char)('0' + amount % 10);
    buf[n++] = ' '; buf[n++] = 'X'; buf[n++] = 'P';
    buf[n] = '\0';
    font_draw_str(buf, x, y, C_XP_PURPLE, 1);
}

/* ------------------------------------------------------------------ */
/* Skill progress indicator                                             */
/* ------------------------------------------------------------------ */
void spr_skill_indicator(int sx, int sy, float progress)
{
    int bar_w = 20;
    int bar_h = 4;
    R(sx - bar_w / 2, sy - 12, bar_w, bar_h, C_BG);
    int fill = (int)(progress * bar_w);
    if (fill > bar_w) fill = bar_w;
    R(sx - bar_w / 2, sy - 12, fill, bar_h, C_XP_PURPLE);
}
