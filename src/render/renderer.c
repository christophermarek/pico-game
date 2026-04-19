#include "renderer.h"
#include "sprites.h"
#include "font.h"
#include "hal.h"
#include "colors.h"
#include "config.h"
#include "../ui/hud.h"
#include "../ui/battle_ui.h"
#include "../ui/menu.h"
#include "../ui/char_create.h"
#include "../game/td_cam.h"
#include <stdint.h>
#include <stdlib.h>

#define TD_FEET_OFF 7.0f
#define TD_PET_FEET 5.0f
#define TD_CULL     72

/* ------------------------------------------------------------------ */
/* Overworld: world pixel → screen (player-centred iso + bearing)      */
/* ------------------------------------------------------------------ */
static void td_world_to_screen(const GameState *s, const TdCamBasis *cam, float wx, float wy,
                               int *sx, int *sy)
{
    td_basis_world_pixel_to_screen(cam, s->td.x, s->td.y, wx, wy, sx, sy);
}

typedef struct {
    int64_t key;
    uint16_t tx, ty;
} TdCellSort;

static int cmp_td_cell(const void *a, const void *b)
{
    int64_t ka = ((const TdCellSort *)a)->key;
    int64_t kb = ((const TdCellSort *)b)->key;
    if (ka < kb)
        return -1;
    if (ka > kb)
        return 1;
    return 0;
}

typedef struct {
    int64_t key;
    uint8_t is_player;
    uint8_t party_i;
} ActorSort;

static int cmp_actor(const void *a, const void *b)
{
    int64_t ka = ((const ActorSort *)a)->key;
    int64_t kb = ((const ActorSort *)b)->key;
    if (ka < kb)
        return -1;
    if (ka > kb)
        return 1;
    return 0;
}

static bool td_cell_on_screen(const GameState *s, const TdCamBasis *cam, int tx, int ty)
{
    float tcx = (float)(tx * TILE + TILE / 2);
    float tcy = (float)(ty * TILE + TILE / 2);
    int   sx, sy;
    td_world_to_screen(s, cam, tcx, tcy, &sx, &sy);
    return sx >= -TD_CULL && sx < DISPLAY_W + TD_CULL && sy >= -TD_CULL && sy < DISPLAY_H + TD_CULL;
}

/* ------------------------------------------------------------------ */
/* Top-down rendering                                                   */
/* ------------------------------------------------------------------ */
void render_topdown(GameState *s, const World *w)
{
    TdCamBasis cam = td_cam_basis(s->td_cam_bearing);
    TdCellSort cells[MAP_W * MAP_H];
    int        n = 0;

    hal_fill(C_SKY_BOT);

    for (int ty = 0; ty < MAP_H; ty++) {
        for (int tx = 0; tx < MAP_W; tx++) {
            /* Home tiles render as a cobblestone courtyard; spr_mansion_td()
             * paints the building on top afterwards. */
            if (!td_cell_on_screen(s, &cam, tx, ty))
                continue;
            float wx = (float)((tx + 1) * TILE);
            float wy = (float)((ty + 1) * TILE);
            int   sx, sy;
            td_world_to_screen(s, &cam, wx, wy, &sx, &sy);
            cells[n].key = ((int64_t)sy << 24) | ((int64_t)tx << 12) | (int64_t)ty;
            cells[n].tx  = (uint16_t)tx;
            cells[n].ty  = (uint16_t)ty;
            n++;
        }
    }
    qsort(cells, (size_t)n, sizeof(cells[0]), cmp_td_cell);

    for (int i = 0; i < n; i++) {
        int       tx = cells[i].tx;
        int       ty = cells[i].ty;
        float     tcx = (float)(tx * TILE + TILE / 2);
        float     tcy = (float)(ty * TILE + TILE / 2);
        int       sx, sy;
        td_world_to_screen(s, &cam, tcx, tcy, &sx, &sy);
        uint8_t   tile = world_tile(w, tx, ty);
        spr_tile_iso_floor(tile, sx, sy, s->tick_count);
    }
    for (int i = 0; i < n; i++) {
        int       tx = cells[i].tx;
        int       ty = cells[i].ty;
        float     tcx = (float)(tx * TILE + TILE / 2);
        float     tcy = (float)(ty * TILE + TILE / 2);
        int       sx, sy;
        td_world_to_screen(s, &cam, tcx, tcy, &sx, &sy);
        uint8_t   tile = world_tile(w, tx, ty);
        spr_tile_iso_onlay(tile, sx, sy, s->tick_count);
        if (w->node_respawn[ty * MAP_W + tx] > 0)
            spr_iso_depleted_mark(sx, sy);
    }

    spr_mansion_td(s, w);

    {
        ActorSort act[PARTY_MAX + 1];
        int       na = 0;
        for (int i = 0; i < s->party_count; i++) {
            const Pet *p = &s->party[i];
            int        fsx, fsy;
            td_world_to_screen(s, &cam, p->trail_x, p->trail_y + TD_PET_FEET, &fsx, &fsy);
            act[na].key       = ((int64_t)fsy << 16) | (int64_t)i;
            act[na].is_player = 0;
            act[na].party_i   = (uint8_t)i;
            na++;
        }
        {
            int fsx, fsy;
            td_world_to_screen(s, &cam, s->td.x, s->td.y + TD_FEET_OFF, &fsx, &fsy);
            act[na].key       = ((int64_t)fsy << 16) | 4096;
            act[na].is_player = 1;
            act[na].party_i   = 0;
            na++;
        }
        qsort(act, (size_t)na, sizeof(act[0]), cmp_actor);

        int player_foot_sx = 0, player_foot_sy = 0;
        for (int i = 0; i < na; i++) {
            if (!act[i].is_player) {
                const Pet *p = &s->party[act[i].party_i];
                int        fsx, fsy;
                td_world_to_screen(s, &cam, p->trail_x, p->trail_y + TD_PET_FEET, &fsx, &fsy);
                spr_monster_foot(fsx, fsy, p->evo_stage, s->td.walk_frame);
            } else {
                td_world_to_screen(s, &cam, s->td.x, s->td.y + TD_FEET_OFF, &player_foot_sx,
                                   &player_foot_sy);
                spr_player_td_foot(player_foot_sx, player_foot_sy, s->td.screen_dir,
                                   s->td.walk_frame, s->custom.skin_idx, s->custom.hair_idx,
                                   s->custom.outfit_idx);
            }
        }

        if (s->skilling) {
            float progress = 1.0f - (float)s->action_ticks_left / (float)ACTION_TICKS;
            spr_skill_indicator(player_foot_sx, player_foot_sy - 18, progress);
        }
    }

    for (int i = 0; i < s->xp_drop_count; i++) {
        const XpDrop *d = &s->xp_drops[i];
        if (d->timer < 5)
            continue;
        int dx, dy;
        td_world_to_screen(s, &cam, (float)d->x, (float)d->y, &dx, &dy);
        dy -= (40 - d->timer);
        if (dx >= 0 && dx < DISPLAY_W && dy >= 0 && dy < DISPLAY_H)
            spr_xp_drop(dx, dy, d->amount);
    }
}

/* ------------------------------------------------------------------ */
/* Side-view rendering                                                  */
/* ------------------------------------------------------------------ */
void render_sideview(GameState *s, const World *w) {
    /* Camera follows player X */
    int cam_x = (int)s->sv.x - DISPLAY_W / 2;
    if (cam_x < 0)                        cam_x = 0;
    if (cam_x > SV_W * TILE - DISPLAY_W)  cam_x = SV_W * TILE - DISPLAY_W;

    /* Sky gradient: 3 horizontal bands */
    hal_fill_rect(0, 0,         DISPLAY_W, DISPLAY_H / 3, C_SKY_TOP);
    hal_fill_rect(0, DISPLAY_H / 3, DISPLAY_W, DISPLAY_H / 3, C_SKY_MID);
    hal_fill_rect(0, 2*DISPLAY_H/3, DISPLAY_W, DISPLAY_H / 3, C_SKY_BOT);

    /* Parallax mountains — two layers */
    int mx1 = -(cam_x * 3 / 10) % (DISPLAY_W + 60);
    int mx2 = -(cam_x * 5 / 10) % (DISPLAY_W + 40);

    /* Layer 1: far mountains */
    for (int i = -1; i < 3; i++) {
        int bx = mx1 + i * (DISPLAY_W / 2 + 30);
        hal_fill_rect(bx,        120, 50, 60, C_MOUNTAIN);
        hal_fill_rect(bx + 30,   105, 50, 75, C_MOUNTAIN);
        hal_fill_rect(bx + 60,   115, 40, 65, C_MOUNTAIN);
    }
    /* Layer 2: near mountains */
    for (int i = -1; i < 4; i++) {
        int bx = mx2 + i * (DISPLAY_W / 3 + 20);
        hal_fill_rect(bx,        140, 40, 80, C_SKY_TOP);
        hal_fill_rect(bx + 20,   130, 40, 90, C_SKY_TOP);
    }

    /* Draw tiles */
    int tile_x0 = cam_x / TILE;
    int tile_x1 = (cam_x + DISPLAY_W - 1) / TILE + 1;
    if (tile_x0 < 0)     tile_x0 = 0;
    if (tile_x1 > SV_W)  tile_x1 = SV_W;

    for (int ty = 0; ty < SV_H; ty++) {
        for (int tx = tile_x0; tx < tile_x1; tx++) {
            uint8_t t = world_sv_tile(w, tx, ty);
            if (t == SVT_AIR) continue;
            int sx = tx * TILE - cam_x;
            int sy = ty * TILE;
            switch (t) {
            case SVT_GROUND:
                hal_fill_rect(sx, sy, TILE, TILE, C_PATH_DARK);
                hal_fill_rect(sx, sy, TILE, 3, C_GRASS_DARK);
                break;
            case SVT_DIRT:
                hal_fill_rect(sx, sy, TILE, TILE, C_SAND_DARK);
                break;
            case SVT_PLATFORM:
                hal_fill_rect(sx, sy, TILE, 4, C_TREE_TRUNK);
                break;
            case SVT_ORE:
                spr_tile(T_ORE, sx, sy, s->tick_count);
                break;
            case SVT_TREE:
                spr_tile(T_TREE, sx, sy, s->tick_count);
                break;
            default:
                break;
            }
        }
    }

    /* Draw pet */
    for (int i = 0; i < s->party_count; i++) {
        const Pet *p = &s->party[i];
        int px = (int)p->trail_x - cam_x - TILE / 2;
        int py = (int)p->trail_y - TILE / 2;
        spr_monster(px, py, p->evo_stage, 0.0f);
    }

    /* Draw player */
    int player_sx = (int)s->sv.x - cam_x - TILE / 2;
    int player_sy = (int)s->sv.y - TILE / 2;
    spr_player_sv(player_sx, player_sy,
                  s->sv.dir, s->sv.walk_frame, !s->sv.on_ground,
                  s->custom.skin_idx, s->custom.hair_idx, s->custom.outfit_idx);
}

/* ------------------------------------------------------------------ */
/* Base mode: placeholder screen                                        */
/* ------------------------------------------------------------------ */
static void render_base(GameState *s) {
    hal_fill(C_BG);
    font_draw_str("HOME BASE", 70, 50, C_TEXT_MAIN, 2);
    font_draw_str("(Coming soon)", 55, 80, C_TEXT_DIM, 1);
    font_draw_str("B: Exit", 85, 200, C_BORDER, 1);

    /* Draw some furniture if placed */
    for (int i = 0; i < FURNITURE_SLOTS; i++) {
        if (s->furniture[i].placed && s->furniture[i].type != FURN_NONE) {
            int bx = 20 + (i % 4) * 50;
            int by = 100 + (i / 4) * 60;
            spr_furniture(s->furniture[i].type, bx, by);
        }
    }
}

/* ------------------------------------------------------------------ */
/* Frame dispatch                                                       */
/* ------------------------------------------------------------------ */
void render_frame(GameState *s, const World *w) {
    switch (s->mode) {
    case MODE_CHAR_CREATE:
        char_create_render(s);
        break;
    case MODE_TOPDOWN:
        render_topdown(s, w);
        hud_draw(s);
        break;
    case MODE_SIDE:
        render_sideview(s, w);
        hud_draw(s);
        break;
    case MODE_BATTLE:
        battle_ui_render(s);
        break;
    case MODE_MENU:
        menu_render(s);
        break;
    case MODE_BASE:
        render_base(s);
        hud_draw(s);
        break;
    default:
        hal_fill(C_BG_DARK);
        break;
    }
}
