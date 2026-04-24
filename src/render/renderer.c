#include "renderer.h"
#include "iso_spritesheet.h"
#include "render_debug.h"
#include "render_internal.h"
#include "hal.h"
#include "colors.h"
#include "config.h"
#include "../ui/hud.h"
#include "../ui/menu.h"
#include "../game/actions.h"
#include "../game/world.h"
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

/* Small + cross flash at node visual centre — signals a hit landed. */
static void draw_hit_flash(int sx, int sy, uint8_t tile_id)
{
    int cy = (tile_id == T_TREE) ? sy - 38 : sy - 22;
    hal_fill_rect(sx - 4, cy,     8, 1, C_WHITE);
    hal_fill_rect(sx,     cy - 3, 1, 6, C_WHITE);
}

/*
 * Burst-of-particles played once when a node is finished off. 8 dots on
 * a circle expand outward over TILE_DESTROY_FRAMES and fade through a
 * tile-type-coloured gradient. Drawn on bare ground (the onlay already
 * cleared on depletion).
 */
static void draw_destroy_burst(int sx, int sy, uint8_t tile_id,
                               uint8_t timer_left)
{
    /* Colour per node — picked to match the sprite's dominant tone. */
    uint16_t core, rim;
    switch (tile_id) {
        case T_TREE:   core = HEX(0x86efac); rim = HEX(0x3f6212); break; /* leaf */
        case T_ROCK:   core = HEX(0xe5e7eb); rim = HEX(0x6b7280); break; /* stone */
        case T_ORE:    core = HEX(0xfbbf24); rim = HEX(0x92400e); break; /* ore spark */
        case T_TGRASS: core = HEX(0xbef264); rim = HEX(0x3f6212); break; /* blade */
        case T_WATER:  core = HEX(0xbae6fd); rim = HEX(0x1e3a8a); break; /* splash */
        default:       core = C_WHITE;       rim = HEX(0x3a3a3a); break;
    }

    int phase = TILE_DESTROY_FRAMES - (int)timer_left;   /* 0..N-1 */
    int r     = phase + 2;                                /* expanding px */
    uint16_t c = (phase < TILE_DESTROY_FRAMES / 2) ? core : rim;

    /* 8 directions on a rough iso-flattened ring (y squashed 2:1). */
    static const int8_t DIRS[8][2] = {
        {  2,  0}, {  2,  1}, {  0,  1}, { -2,  1},
        { -2,  0}, { -2, -1}, {  0, -1}, {  2, -1},
    };
    for (int i = 0; i < 8; i++) {
        int px = sx + (DIRS[i][0] * r) / 2;
        int py = sy - 8 + (DIRS[i][1] * r) / 2;
        hal_fill_rect(px - 1, py - 1, 2, 2, c);
    }
}

/* Row of pips above the ground line showing remaining node HP. */
static void draw_hp_pips(int sx, int sy, uint8_t hp, uint8_t max_hp)
{
    if (max_hp == 0) return;
    const int pip_w = 4, pip_h = 3, gap = 1;
    int total_w = max_hp * pip_w + (max_hp - 1) * gap;
    int px = sx - total_w / 2;
    int py = sy - 2;
    hal_fill_rect(px - 1, py - 1, total_w + 2, pip_h + 2, C_BG_DARK);
    for (int i = 0; i < max_hp; i++) {
        uint16_t c = (i < hp) ? C_HP_GREEN : HEX(0x3a1010);
        hal_fill_rect(px + i * (pip_w + gap), py, pip_w, pip_h, c);
    }
}

static void draw_skill_indicator(int sx, int sy, float progress)
{
    int bar_w = 20, bar_h = 4;
    hal_fill_rect(sx - bar_w / 2, sy, bar_w, bar_h, C_BG);
    int fill = (int)(progress * bar_w);
    if (fill > bar_w) fill = bar_w;
    if (fill > 0) hal_fill_rect(sx - bar_w / 2, sy, fill, bar_h, C_XP_PURPLE);
}

typedef struct {
    int64_t  key;
    int16_t  sx, sy;      /* screen coords of tile centre, cached for draw */
    uint16_t tx, ty;
} TdCellSort;

static int cmp_td_cell(const void *a, const void *b)
{
    int64_t ka = ((const TdCellSort *)a)->key;
    int64_t kb = ((const TdCellSort *)b)->key;
    if (ka < kb) return -1;
    if (ka > kb) return  1;
    return 0;
}

/*
 * Tool held while skilling. Camera-aware because every position it uses
 * comes from td_basis_world_pixel_to_screen — the target's screen pos
 * rotates naturally with the camera so the offset vector always points
 * from the player to the tile being hit.
 *
 * Player sprite spans y = sy-7..sy+9 (feet project at sy). Hip/hand sits
 * at sy+TOOL_HAND_Y_OFF. Horizontal flip applies when the character's
 * sprite is facing left so the axe-head points forward not into the body.
 */
#define TOOL_ICON_NUM   2
#define TOOL_ICON_DEN   3
#define TOOL_ICON_DRAWN ((16 * TOOL_ICON_NUM) / TOOL_ICON_DEN)   /* ≈ 10 px */
#define TOOL_REACH_PX   5
#define TOOL_HAND_Y_OFF 1

static void render_action_tool(const GameState *s, const World *w,
                               const TdCamBasis *cam,
                               int player_sx, int player_sy)
{
    if (!s->skilling) return;

    const NodeAction *a = action_for_tile(
        world_tile(w, s->action_node_x, s->action_node_y));
    if (!a) return;

    int node_sx, node_sy;
    td_basis_world_pixel_to_screen(cam, s->td.x, s->td.y,
                                   (float)(s->action_node_x * TILE + TILE / 2),
                                   (float)(s->action_node_y * TILE + TILE / 2),
                                   &node_sx, &node_sy);

    int hand_x = player_sx;
    int hand_y = player_sy + TOOL_HAND_Y_OFF;

    float dx  = (float)(node_sx - hand_x);
    float dy  = (float)(node_sy - hand_y);
    float len = hypotf(dx, dy);
    if (len < 1.0f) len = 1.0f;

    int cx = hand_x + (int)(dx / len * (float)TOOL_REACH_PX);
    int cy = hand_y + (int)(dy / len * (float)TOOL_REACH_PX);

    /* Chop lift — small bob for visible swing. */
    int phase = (s->frame_count / 2) & 3;
    int lift  = (phase == 0) ? -2 : (phase == 1) ? -1 : (phase == 2) ? 0 : -1;
    cy += lift;

    /* Pick an orientation so the blade/head always points away from the
     * body. Atlas art is right-facing. */
    IconOrient orient;
    switch (s->td.screen_dir) {
        case DIR_LEFT:  orient = ICON_ORIENT_L; break;
        case DIR_UP:    orient = ICON_ORIENT_U; break;
        case DIR_DOWN:  orient = ICON_ORIENT_D; break;
        case DIR_RIGHT:
        default:        orient = ICON_ORIENT_R; break;
    }

    iso_draw_item_icon_scaled(a->tool,
                              cx - TOOL_ICON_DRAWN / 2,
                              cy - TOOL_ICON_DRAWN / 2,
                              TOOL_ICON_NUM, TOOL_ICON_DEN, orient);
}

/* Parabola-arc item sprites flying from a node to the hotbar. */
static void render_item_flies(const GameState *s, const TdCamBasis *cam)
{
    int total_w = HOTBAR_SLOTS * HUD_HOTBAR_SLOT_W + (HOTBAR_SLOTS - 1) * HUD_HOTBAR_SLOT_GAP;
    int hb_x0   = (DISPLAY_W - total_w) / 2;
    int hb_cy   = HUD_HOTBAR_Y + HUD_HOTBAR_SLOT_W / 2;

    for (int i = 0; i < MAX_ITEM_FLIES; i++) {
        const ItemFly *fly = &s->item_flies[i];
        if (!fly->active) continue;

        int src_sx, src_sy;
        td_basis_world_pixel_to_screen(cam, s->td.x, s->td.y,
                                       fly->wx, fly->wy, &src_sx, &src_sy);

        int dst_sx, dst_sy;
        int sl = (int)fly->slot;
        if (sl >= 0 && sl < HOTBAR_SLOTS) {
            dst_sx = hb_x0 + sl * (HUD_HOTBAR_SLOT_W + HUD_HOTBAR_SLOT_GAP)
                     + HUD_HOTBAR_SLOT_W / 2;
            dst_sy = hb_cy;
        } else {
            dst_sx = DISPLAY_W / 2;
            dst_sy = DISPLAY_H + 20;
        }

        float t    = 1.0f - (float)fly->timer / (float)ITEM_FLY_FRAMES;
        int   px   = (int)(src_sx + t * (float)(dst_sx - src_sx));
        float lift = sinf(t * (float)M_PI) * 30.0f;
        int   py   = (int)(src_sy + t * (float)(dst_sy - src_sy) - lift);

        /* Poof burst at spawn: 4 frames starting the frame after spawn.
         * state_anim_tick decrements before render, so the fly is first
         * rendered at timer=ITEM_FLY_FRAMES-1 — pf must start at 0 there. */
        int pf = ITEM_FLY_FRAMES - 1 - (int)fly->timer; /* 0,1,2,3 */
        if (pf >= 0 && pf < 4) {
            int r  = pf * 4 + 3;
            uint16_t pc = (pf == 0) ? C_WHITE : (pf == 1) ? C_GOLD : HEX(0x886622);
            int rd = r * 7 / 10;
            hal_fill_rect(src_sx - r - 1, src_sy - 1, 2, 2, pc);
            hal_fill_rect(src_sx + r - 1, src_sy - 1, 2, 2, pc);
            hal_fill_rect(src_sx - 1, src_sy - r - 1, 2, 2, pc);
            hal_fill_rect(src_sx - 1, src_sy + r - 1, 2, 2, pc);
            hal_fill_rect(src_sx - rd - 1, src_sy - rd - 1, 2, 2, pc);
            hal_fill_rect(src_sx + rd - 1, src_sy - rd - 1, 2, 2, pc);
            hal_fill_rect(src_sx - rd - 1, src_sy + rd - 1, 2, 2, pc);
            hal_fill_rect(src_sx + rd - 1, src_sy + rd - 1, 2, 2, pc);
        }

        if (px >= -8 && px < DISPLAY_W + 8 && py >= -8 && py < DISPLAY_H + 8) {
            if (!iso_draw_item_icon(fly->item, px - 8, py - 8))
                hal_fill_rect(px - 3, py - 3, 6, 6, C_WHITE);
        }
    }
}

static void render_topdown(GameState *s, const World *w)
{
    /* Static: ~7 KB — off the stack so the Pico's 4 KB main-thread stack
     * doesn't overflow. Renderer is not reentrant. */
    static TdCellSort cells[MAP_CELLS];
    TdCamBasis cam = td_cam_basis(s->td_cam_bearing);
    int        n = 0;

    hal_fill(C_SKY_BOT);

    for (int ty = 0; ty < w->h; ty++) {
        for (int tx = 0; tx < w->w; tx++) {
            if (!cell_on_screen(s, &cam, tx, ty)) continue;
            float tcx = (float)(tx * TILE + TILE / 2);
            float tcy = (float)(ty * TILE + TILE / 2);
            int   sx, sy;
            world_to_screen(s, &cam, tcx, tcy, &sx, &sy);
            /* Bias sy above zero before shifting; left-shifting a negative
             * signed int is undefined behaviour. Tiles in the cull zone can
             * have sy < 0. */
            int64_t sy_biased = (int64_t)sy + (DISPLAY_H + 2 * TD_ISO_CULL);
            cells[n].key = (sy_biased << 24) | ((int64_t)tx << 12) | (int64_t)ty;
            cells[n].sx = (int16_t)sx;
            cells[n].sy = (int16_t)sy;
            cells[n].tx = (uint16_t)tx;
            cells[n].ty = (uint16_t)ty;
            n++;
        }
    }
    qsort(cells, (size_t)n, sizeof(cells[0]), cmp_td_cell);

    for (int i = 0; i < n; i++) {
        uint8_t tile = world_tile(w, cells[i].tx, cells[i].ty);
        iso_draw_tile_full(tile, cells[i].sx, cells[i].sy, s->frame_count);
    }
    int player_sx, player_sy;
    world_to_screen(s, &cam, s->td.x, s->td.y + TD_FEET_OFF,
                    &player_sx, &player_sy);

    /* Depth-sort the player into the onlay pass: the player draws in
     * between tiles whose screen y straddles theirs, so obstacles visually
     * in front of them (larger sy) overlap the player sprite. */
    bool player_drawn = false;

    for (int i = 0; i < n; i++) {
        int     tx   = cells[i].tx;
        int     ty   = cells[i].ty;
        uint8_t tile = world_tile(w, tx, ty);
        int     idx  = ty * w->w + tx;
        int     sx   = cells[i].sx;
        int     sy   = cells[i].sy;

        if (!player_drawn && player_sy <= sy) {
            iso_draw_td_player_char(player_sx, player_sy, s->td.screen_dir,
                                    (uint8_t)s->td.walk_frame);
            render_action_tool(s, w, &cam, player_sx, player_sy);
            player_drawn = true;
        }

        /* Depleted nodes clear entirely — skip the onlay, leaving bare
         * floor. Still play the destroy burst if the timer is running
         * (one-shot particle shower on the final hit). */
        if (world_node_depleted(w, tx, ty)) {
            if (w->tile_destroy_timer[idx] > 0)
                draw_destroy_burst(sx, sy, tile, w->tile_destroy_timer[idx]);
            continue;
        }

        int shake_ox = 0;
        if (s->skilling && s->action_node_x == tx && s->action_node_y == ty)
            shake_ox = ((s->frame_count / 3) & 1) ? 1 : -1;

        /* Tall-grass sway: 1 px wobble when the player is moving through
         * this tile. Cheap per-tile check, no new state. */
        if (tile == T_TGRASS && s->td.tile_x == tx && s->td.tile_y == ty &&
            s->td.walk_frame > 0.01f)
            shake_ox += ((s->frame_count / 3) & 1) ? 1 : -1;

        iso_draw_tile_onlay(tile, sx + shake_ox, sy);

        if (w->tile_hit_timer[idx] > 0)
            draw_hit_flash(sx, sy, tile);
        uint8_t max_hp = world_node_max_hp(tile);
        if (max_hp > 1 && w->node_hp[idx] < max_hp)
            draw_hp_pips(sx, sy, w->node_hp[idx], max_hp);
    }
    if (!player_drawn) {
        iso_draw_td_player_char(player_sx, player_sy, s->td.screen_dir,
                                (uint8_t)s->td.walk_frame);
        render_action_tool(s, w, &cam, player_sx, player_sy);
    }

    if (s->skilling) {
        float progress = 1.0f - (float)s->action_ticks_left / (float)ACTION_TICKS;
        draw_skill_indicator(player_sx, player_sy - 30, progress);
    }

    render_item_flies(s, &cam);

    if (s->debug_mode)
        render_debug_overworld(s, w, &cam);
}

void render_frame(GameState *s, const World *w) {
    switch (s->mode) {
    case MODE_TOPDOWN:
        render_topdown(s, w);
        hud_draw(s);
        break;
    case MODE_MENU:
        menu_render(s);
        break;
    }
    if (s->debug_mode) render_debug_fps();
    hal_show();
}
