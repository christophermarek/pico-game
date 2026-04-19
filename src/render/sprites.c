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
#include <stdlib.h>

/* ------------------------------------------------------------------ */
/* Shorthand fill                                                       */
/* ------------------------------------------------------------------ */
static inline void R(int x, int y, int w, int h, uint16_t c) {
    hal_fill_rect(x, y, w, h, c);
}

/* ------------------------------------------------------------------ */
/* Scanline polygon helpers (screen-space, hal_fill_rect clips for us) */
/* ------------------------------------------------------------------ */

/*
 * fill_tri — solid triangle, scan-converted by walking Y from min to max
 * and filling between the long edge (v1→v3) and the two short edges
 * (v1→v2 above the middle vertex, v2→v3 below).
 */
static void fill_tri(int x1, int y1, int x2, int y2, int x3, int y3, uint16_t col)
{
    if (y1 > y2) { int tx = x1; x1 = x2; x2 = tx; int ty = y1; y1 = y2; y2 = ty; }
    if (y2 > y3) { int tx = x2; x2 = x3; x3 = tx; int ty = y2; y2 = y3; y3 = ty; }
    if (y1 > y2) { int tx = x1; x1 = x2; x2 = tx; int ty = y1; y1 = y2; y2 = ty; }

    if (y3 == y1) {
        int mn = x1, mx = x1;
        if (x2 < mn) mn = x2; else if (x2 > mx) mx = x2;
        if (x3 < mn) mn = x3; else if (x3 > mx) mx = x3;
        R(mn, y1, mx - mn + 1, 1, col);
        return;
    }

    /* Upper half: y1 .. y2 */
    int dy_long = y3 - y1;
    int dx_long = x3 - x1;
    int dy_top  = y2 - y1;
    int dx_top  = x2 - x1;
    for (int y = y1; y <= y2; y++) {
        int xa = (dy_top == 0) ? x1 : x1 + dx_top  * (y - y1) / dy_top;
        int xb = x1 + dx_long * (y - y1) / dy_long;
        if (xa > xb) { int t = xa; xa = xb; xb = t; }
        R(xa, y, xb - xa + 1, 1, col);
    }
    /* Lower half: y2+1 .. y3 */
    int dy_bot = y3 - y2;
    int dx_bot = x3 - x2;
    for (int y = y2 + 1; y <= y3; y++) {
        int xa = (dy_bot == 0) ? x2 : x2 + dx_bot  * (y - y2) / dy_bot;
        int xb = x1 + dx_long * (y - y1) / dy_long;
        if (xa > xb) { int t = xa; xa = xb; xb = t; }
        R(xa, y, xb - xa + 1, 1, col);
    }
}

/*
 * fill_wall — a vertical "wall" parallelogram in iso space.
 * Base edge: (ax, ay) → (bx, by). Top edge is the same line shifted up by H px.
 * Both "side" edges are strictly vertical, which keeps walls aligned even after
 * camera rotation (the iso projection always tilts +x and +y, never the verticals).
 */
static void fill_wall(int ax, int ay, int bx, int by, int H, uint16_t col)
{
    if (H < 1) return;
    if (ax == bx) {
        int y0 = ay < by ? ay : by;
        int y1 = ay > by ? ay : by;
        R(ax, y0 - H, 1, (y1 - y0) + H + 1, col);
        return;
    }
    if (ax > bx) { int t = ax; ax = bx; bx = t; t = ay; ay = by; by = t; }
    int dx = bx - ax;
    for (int x = ax; x <= bx; x++) {
        int y_bot = ay + ((by - ay) * (x - ax) + (dx >> 1)) / dx;
        R(x, y_bot - H, 1, H + 1, col);
    }
}

/* fill_iso_quad — convex 4-corner polygon (any iso-projected rectangle). */
static void fill_iso_quad(int ax, int ay, int bx, int by,
                          int cx_, int cy_, int dx_, int dy_, uint16_t col)
{
    fill_tri(ax, ay, bx, by, cx_, cy_, col);
    fill_tri(ax, ay, cx_, cy_, dx_, dy_, col);
}

/* ------------------------------------------------------------------ */
/* Tile sprites                                                         */
/* ------------------------------------------------------------------ */
void spr_tile(uint8_t tile_id, int sx, int sy, uint32_t tick) {
    if (iso_draw_sv_tile(sx, sy, tile_id))
        return;

    int T = TILE;

    switch (tile_id) {

    case T_GRASS:
        R(sx, sy, T, T, C_GRASS_DARK);
        /* Grass blades — simple marks */
        for (int i = 0; i < 4; i++) {
            int bx = sx + 2 + i * 4;
            int by = sy + 3 + (i & 1) * 4;
            R(bx, by, 1, 3, C_GRASS_LIGHT);
        }
        break;

    case T_PATH:
        R(sx, sy, T, T, C_PATH_DARK);
        /* Light pebble spots */
        R(sx+2,  sy+3,  3, 2, C_PATH_LIGHT);
        R(sx+9,  sy+10, 2, 2, C_PATH_LIGHT);
        R(sx+12, sy+5,  2, 2, C_PATH_LIGHT);
        break;

    case T_WATER: {
        R(sx, sy, T, T, C_WATER_DARK);
        /* Animated wave lines */
        int phase = (int)(tick / 4) % 4;
        for (int row = 0; row < 3; row++) {
            int wy = sy + 3 + row * 5;
            int off = (phase + row * 2) % 8;
            for (int wx = sx + off; wx < sx + T; wx += 8)
                R(wx, wy, 4, 1, C_WATER_MID);
        }
        break;
    }

    case T_SAND:
        R(sx, sy, T, T, C_SAND);
        R(sx+1, sy+1, T-2, T-2, C_SAND_DARK);
        R(sx+4, sy+4, 4, 2, C_SAND);
        R(sx+10, sy+9, 3, 2, C_SAND);
        break;

    case T_TREE:
        /* Full cell first — avoids uncleared framebuffer showing through gaps
         * when the camera scrolls (looked like trees sliding with the player). */
        R(sx, sy, T, T, C_GRASS_DARK);
        /* Trunk */
        R(sx+6, sy+10, 4, 6, C_TREE_TRUNK);
        /* Dark canopy */
        R(sx+2, sy+2, 12, 10, C_TREE_DARK);
        /* Mid layer */
        R(sx+3, sy+3, 10, 8, C_TREE_MID);
        /* Light highlights */
        R(sx+5, sy+4, 3, 2, C_TREE_LIGHT);
        R(sx+9, sy+6, 2, 2, C_TREE_LIGHT);
        break;

    case T_ROCK:
        R(sx, sy, T, T, C_ROCK_DARK);
        /* Base mass */
        R(sx+1, sy+4, 14, 10, C_ROCK_DARK);
        R(sx+3, sy+2, 10,  4, C_ROCK_DARK);
        /* Mid shading */
        R(sx+2, sy+4, 12,  8, C_ROCK_MID);
        /* Light top face */
        R(sx+4, sy+3,  8,  3, C_ROCK_LIGHT);
        R(sx+5, sy+2,  6,  2, C_ROCK_LIGHT);
        break;

    case T_ORE:
        R(sx, sy, T, T, C_ROCK_DARK);
        /* Dark grey base */
        R(sx+1, sy+4, 14, 10, C_ROCK_DARK);
        R(sx+3, sy+2, 10,  4, C_ROCK_DARK);
        R(sx+2, sy+4, 12,  8, C_ROCK_MID);
        /* Gold vein marks */
        R(sx+4, sy+5,  2,  2, C_ORE_VEIN);
        R(sx+8, sy+8,  3,  2, C_ORE_VEIN);
        R(sx+11,sy+5,  2,  3, C_ORE_VEIN);
        R(sx+6, sy+10, 2,  1, C_ORE_SHINE);
        break;

    case T_FLOWER:
        /* Grass base */
        R(sx, sy, T, T, C_GRASS_DARK);
        /* Flower stem */
        R(sx+7, sy+8, 2, 5, C_GRASS_LIGHT);
        /* Petals */
        R(sx+5, sy+4, 6, 2, C_FLOWER_PINK);
        R(sx+7, sy+2, 2, 6, C_FLOWER_PINK);
        R(sx+7, sy+4, 2, 2, C_GOLD);
        break;

    case T_TGRASS:
        R(sx, sy, T, T, C_GRASS_DARK);
        /* Vertical stripe pattern */
        for (int i = 0; i < 4; i++) {
            int tx2 = sx + 1 + i * 4;
            R(tx2, sy,   1, T, C_TALL_GRASS);
            R(tx2+1, sy+2, 1, T-4, C_GRASS_LIGHT);
        }
        break;

    case T_HOME_SOLID:
    case T_HOME:
        /* Facade drawn by spr_mansion_td() */
        break;

    default:
        R(sx, sy, T, T, C_GRASS_DARK);
        break;
    }
}

/* ------------------------------------------------------------------ */
/* Isometric cell footprint (2:1 diamond, centre = map cell centre)     */
/* ------------------------------------------------------------------ */
static void iso_diamond(int cx, int cy, int half_w, int half_h, uint16_t c)
{
    if (half_h <= 0)
        half_h = 1;
    for (int r = -half_h; r <= half_h; r++) {
        int aw = (half_w * (half_h - abs(r)) + half_h - 1) / half_h;
        if (aw < 0)
            aw = 0;
        int x0 = cx - aw - 1;
        int w  = aw * 2 + 3;
        R(x0, cy + r, w, 1, c);
    }
}

void spr_iso_depleted_mark(int cx, int cy)
{
    if (iso_draw_depleted_mark(cx, cy))
        return;
    int hw = TILE - 4;
    int hh = TILE / 2 - 2;
    if (hh < 1)
        hh = 1;
    iso_diamond(cx, cy, hw, hh, C_ROCK_DARK);
    iso_diamond(cx, cy + 1, hw - 3, hh - 1, HEX(0x1f2937));
}

static void iso_diamond_lit(int cx, int cy, int hw, int hh, uint16_t base, uint16_t rim_hi,
                            uint16_t rim_lo)
{
    iso_diamond(cx, cy, hw, hh, base);
    for (int r = -hh; r <= hh; r++) {
        int aw = (hw * (hh - abs(r)) + hh - 1) / hh;
        if (aw <= 0)
            continue;
        R(cx - aw - 1, cy + r, 1, 1, rim_hi);
        R(cx + aw + 1, cy + r, 1, 1, rim_lo);
    }
}

void spr_tile_iso_floor(uint8_t tile_id, int cx, int cy, uint32_t tick)
{
    if (iso_draw_tile_full(tile_id, cx, cy, tick))
        return;

    const int hw = TILE;
    const int hh = TILE / 2;

    switch (tile_id) {
    case T_GRASS:
        iso_diamond_lit(cx, cy, hw, hh, C_GRASS_DARK, C_GRASS_LIGHT, HEX(0x1e3d1e));
        break;
    case T_PATH:
        iso_diamond_lit(cx, cy, hw, hh, C_PATH_DARK, C_PATH_LIGHT, HEX(0x5c4a32));
        break;
    case T_WATER:
        iso_diamond_lit(cx, cy, hw, hh, C_WATER_DARK, C_WATER_MID, HEX(0x1a3a5a));
        break;
    case T_SAND:
        iso_diamond_lit(cx, cy, hw, hh, C_SAND, C_SAND_DARK, HEX(0xa08050));
        break;
    case T_TREE:
        iso_diamond_lit(cx, cy, hw, hh, C_GRASS_DARK, C_GRASS_LIGHT, HEX(0x1e4020));
        break;
    case T_ROCK:
    case T_ORE:
        iso_diamond(cx, cy, hw - 4, hh - 1, HEX(0x2a323c));
        break;
    case T_FLOWER:
        iso_diamond_lit(cx, cy, hw, hh, C_GRASS_DARK, C_GRASS_LIGHT, HEX(0x1e4020));
        break;
    case T_TGRASS:
        iso_diamond_lit(cx, cy, hw, hh, HEX(0x234d23), C_TALL_GRASS, HEX(0x143814));
        break;
    case T_HOME:
    case T_HOME_SOLID:
        /* Grass under the mansion; path spine is separate tiles (see world.c). */
        iso_diamond_lit(cx, cy, hw, hh, C_GRASS_DARK, C_GRASS_LIGHT, HEX(0x1e3d1e));
        break;
    default:
        iso_diamond_lit(cx, cy, hw, hh, C_GRASS_DARK, C_GRASS_LIGHT, HEX(0x1e3d1e));
        break;
    }
}

void spr_tile_iso_onlay(uint8_t tile_id, int cx, int cy, uint32_t tick)
{
    if (iso_tile_png_covers_onlay(tile_id))
        return;

    const int hw = TILE;
    const int hh = TILE / 2;

    switch (tile_id) {
    case T_PATH:
        R(cx - 5, cy, 2, 2, C_PATH_LIGHT);
        R(cx + 3, cy - 2, 2, 2, C_PATH_LIGHT);
        R(cx - 1, cy + 3, 3, 1, C_PATH_LIGHT);
        break;
    case T_WATER: {
        int phase = (int)(tick / 4) % 4;
        for (int row = -2; row <= 2; row++) {
            int aw = (hw * (hh - abs(row)) + hh - 1) / hh;
            int off = (phase + row * 2) % 5;
            for (int x = cx - aw + off; x < cx + aw; x += 5)
                R(x, cy + row, 2, 1, C_WATER_LIGHT);
        }
        break;
    }
    case T_SAND:
        R(cx - 2, cy + 1, 2, 1, C_SAND_DARK);
        R(cx + 2, cy - 1, 3, 1, C_SAND);
        break;
    case T_TREE:
        R(cx - 1, cy - 4, 2, 8, C_TREE_TRUNK);
        iso_diamond(cx, cy - 6, hw - 4, hh - 2, C_TREE_DARK);
        iso_diamond(cx, cy - 9, hw - 8, hh - 4, C_TREE_MID);
        R(cx - 2, cy - 11, 4, 2, C_TREE_LIGHT);
        break;
    case T_ROCK:
        iso_diamond(cx, cy - 2, hw - 4, hh - 1, C_ROCK_DARK);
        iso_diamond(cx, cy - 4, hw - 8, hh - 3, C_ROCK_MID);
        R(cx - 3, cy - 6, 6, 3, C_ROCK_LIGHT);
        break;
    case T_ORE:
        iso_diamond(cx, cy - 2, hw - 4, hh - 1, C_ROCK_DARK);
        iso_diamond(cx, cy - 4, hw - 8, hh - 3, C_ROCK_MID);
        R(cx - 2, cy - 4, 2, 2, C_ORE_VEIN);
        R(cx + 2, cy - 2, 3, 2, C_ORE_VEIN);
        R(cx, cy, 2, 1, C_ORE_SHINE);
        break;
    case T_FLOWER:
        R(cx - 1, cy + 1, 2, 3, C_GRASS_LIGHT);
        R(cx - 3, cy - 2, 6, 2, C_FLOWER_PINK);
        R(cx - 1, cy - 4, 2, 4, C_FLOWER_PINK);
        R(cx - 1, cy - 2, 2, 2, C_GOLD);
        break;
    case T_TGRASS:
        for (int i = 0; i < 4; i++) {
            int rr = -hh + 2 + i * 3;
            if (rr > hh - 2)
                break;
            int aw = (hw * (hh - abs(rr)) + hh - 1) / hh;
            int lx = cx - aw + 1 + (i * 5) % (aw * 2 - 2);
            R(lx, cy + rr, 1, 5, C_TALL_GRASS);
        }
        break;
    default:
        break;
    }
}

void spr_tile_iso(uint8_t tile_id, int cx, int cy, uint32_t tick)
{
    spr_tile_iso_floor(tile_id, cx, cy, tick);
    spr_tile_iso_onlay(tile_id, cx, cy, tick);
}

/*
 * spr_mansion_td — render the home base mansion as a single coherent iso
 * building. Replaces the per-tile "field of diamond blocks" approach with:
 *
 *   1. porch tiles (still per-tile so the player can see path tiles under foot)
 *   2. ground shadow (a flat diamond under the footprint)
 *   3. stone plinth (2 visible side faces + top surface + rim highlight)
 *   4. two main wall quads (the two wall edges that include the "front" corner,
 *      picked each frame from the current iso camera bearing)
 *   5. cornice & sill trim (thin highlight bands across the top/bottom of walls)
 *   6. vertical seam at the front corner (helps the eye parse depth)
 *   7. windows distributed along each visible wall, scaled by wall length
 *   8. door centered on the world-south wall whenever that wall is front-facing
 *      (always visible at bearings 0 and 1, hidden by the building at 2 and 3 —
 *      physically correct for an orbital iso camera)
 *   9. pyramidal roof — 4 triangle panels meeting at a single screen-space apex;
 *      drawn back-to-front so visible panels cleanly overdraw hidden ones
 *  10. chimney on the roof for silhouette recognition
 *
 * Everything is parameterised on the actual footprint corners after the active
 * camera rotation, so rotating the view produces a plausible 3D read instead
 * of the grid of independent diamonds the per-tile version produced.
 */
void spr_mansion_td(const GameState *s, const World *w)
{
    (void)w;
    TdCamBasis cam = td_cam_basis(s->td_cam_bearing);

    const float MV_HW = 48.0f;
    const float MV_HH = 32.0f;
    const float MV_CX =
        (float)(MANSION_TX0 * TILE) + (float)(MANSION_TW * TILE) * 0.5f;
    const float MV_CY =
        (float)(MANSION_TY0 * TILE) +
        (float)((MANSION_TH - MANSION_PORCH_ROWS) * TILE) * 0.5f;

    /* Try PNG first — assets/assets_iso_mansion.png (4 bearing frames). */
    {
        int anchor_sx, anchor_sy;
        td_basis_world_pixel_to_screen(&cam, s->td.x, s->td.y,
                                       MV_CX, MV_CY, &anchor_sx, &anchor_sy);
        if (iso_draw_mansion(anchor_sx, anchor_sy, s->td_cam_bearing))
            return;
    }

    /* Procedural fallback (used when PNG is not available, e.g. Pico build). */

    /* ---------- 1. Project 4 footprint corners (NW, NE, SE, SW) ---------- */
    const float wcx[4] = {
        MV_CX - MV_HW, MV_CX + MV_HW, MV_CX + MV_HW, MV_CX - MV_HW,
    };
    const float wcy[4] = {
        MV_CY - MV_HH, MV_CY - MV_HH, MV_CY + MV_HH, MV_CY + MV_HH,
    };
    int cx_[4], cy_[4];
    for (int i = 0; i < 4; i++)
        td_basis_world_pixel_to_screen(&cam, s->td.x, s->td.y, wcx[i], wcy[i],
                                       &cx_[i], &cy_[i]);

    /* Footprint centre (for roof apex, chimney, shadow) */
    float wfcx = (wcx[0] + wcx[2]) * 0.5f;
    float wfcy = (wcy[0] + wcy[2]) * 0.5f;
    int   fcx, fcy;
    td_basis_world_pixel_to_screen(&cam, s->td.x, s->td.y, wfcx, wfcy, &fcx, &fcy);

    const int H_PLINTH = 4;
    const int H_WALL   = 28;
    const int H_ROOF   = 22;

    /* Ground shadow: the cobblestone courtyard under the footprint already
     * grounds the building; a separate shadow diamond here just poked out
     * below the plinth as a visible black ring. Skipping it.               */

    /* ---------- 4. Front corner & edge visibility ------------------------ */
    /* Edge e (in CCW order around the footprint) connects corner e → (e+1)%4:
     *   e=0 NW→NE (north wall)
     *   e=1 NE→SE (east wall)
     *   e=2 SE→SW (south wall — door lives here in world space)
     *   e=3 SW→NW (west wall)
     * Whichever corner projects to the greatest screen Y is "front"; the two
     * edges that touch it are visible, the other two are behind.           */
    int front = 0;
    for (int i = 1; i < 4; i++) if (cy_[i] > cy_[front]) front = i;
    bool edge_vis[4];
    for (int e = 0; e < 4; e++)
        edge_vis[e] = (e == front) || (((e + 1) & 3) == front);

    /* ---------- 5. Plinth (stone base) ----------------------------------- */
    for (int e = 0; e < 4; e++) {
        if (!edge_vis[e]) continue;
        int a = e, b = (e + 1) & 3;
        fill_wall(cx_[a], cy_[a], cx_[b], cy_[b], H_PLINTH, C_ROCK_DARK);
    }
    fill_iso_quad(cx_[0], cy_[0] - H_PLINTH, cx_[1], cy_[1] - H_PLINTH,
                  cx_[2], cy_[2] - H_PLINTH, cx_[3], cy_[3] - H_PLINTH, C_ROCK_MID);
    /* 1px rim highlight along visible plinth top edges */
    for (int e = 0; e < 4; e++) {
        if (!edge_vis[e]) continue;
        int a = e, b = (e + 1) & 3;
        int ax = cx_[a], ay = cy_[a] - H_PLINTH;
        int bx = cx_[b], by = cy_[b] - H_PLINTH;
        if (ax == bx) continue;
        if (ax > bx) { int t = ax; ax = bx; bx = t; t = ay; ay = by; by = t; }
        int dx = bx - ax;
        for (int x = ax; x <= bx; x++) {
            int y = ay + ((by - ay) * (x - ax) + (dx >> 1)) / dx;
            R(x, y, 1, 1, C_ROCK_LIGHT);
        }
    }

    /* ---------- 6. Main walls -------------------------------------------- */
    /* Give the two visible walls slightly different tones so the corner reads
     * as a seam rather than flat colour. The "front" wall is the visible wall
     * whose midpoint is lower on screen (more of its face toward the camera). */
    int front_edge_a = (front + 3) & 3; /* ...→front  */
    int front_edge_b = front;           /* front→...  */
    int mid_a_y = (cy_[front_edge_a] + cy_[(front_edge_a + 1) & 3]) / 2;
    int mid_b_y = (cy_[front_edge_b] + cy_[(front_edge_b + 1) & 3]) / 2;
    int front_edge = (mid_a_y >= mid_b_y) ? front_edge_a : front_edge_b;
    int side_edge  = (front_edge == front_edge_a) ? front_edge_b : front_edge_a;

    const uint16_t C_WALL_FRONT = C_PANEL;            /* lit wall */
    const uint16_t C_WALL_SIDE  = HEX(0x110825);      /* shaded wall */
    const uint16_t C_WALL_TRIM  = C_BORDER;           /* cornice & trim */
    const uint16_t C_WALL_SHADE = HEX(0x070312);      /* deep shadow at base */

    int wall_base = H_PLINTH; /* base of wall in screen-Y offset terms */

    {
        int a = front_edge, b = (a + 1) & 3;
        fill_wall(cx_[a], cy_[a] - wall_base, cx_[b], cy_[b] - wall_base,
                  H_WALL, C_WALL_FRONT);
    }
    {
        int a = side_edge, b = (a + 1) & 3;
        fill_wall(cx_[a], cy_[a] - wall_base, cx_[b], cy_[b] - wall_base,
                  H_WALL, C_WALL_SIDE);
    }

    /* Cornice: 2-pixel trim strip running along the top of each visible wall */
    for (int e = 0; e < 4; e++) {
        if (!edge_vis[e]) continue;
        int a = e, b = (e + 1) & 3;
        fill_wall(cx_[a], cy_[a] - wall_base - H_WALL + 2,
                  cx_[b], cy_[b] - wall_base - H_WALL + 2, 2, C_WALL_TRIM);
    }
    /* Base shade: 2-pixel shadow strip where walls meet the plinth */
    for (int e = 0; e < 4; e++) {
        if (!edge_vis[e]) continue;
        int a = e, b = (e + 1) & 3;
        fill_wall(cx_[a], cy_[a] - wall_base, cx_[b], cy_[b] - wall_base,
                  2, C_WALL_SHADE);
    }

    /* Vertical seam at the front corner — helps the eye read the two walls
     * as belonging to one 3D block rather than floating polygons. */
    R(cx_[front], cy_[front] - wall_base - H_WALL,
      1, H_WALL + 1, C_WALL_SHADE);

    /* ---------- 7. Windows along each visible wall ----------------------- */
    /*
     * Windows are spaced by wall length, and their "center" is interpolated
     * along the slanted wall top. Small rectangles — 5 wide × 7 tall — with a
     * warm glow (C_GOLD_LIGHT) suggesting occupied rooms. We render two rows
     * (upper floor + lower floor) on the front-lit wall and a single row on
     * the darker side wall, which reads as the mansion having depth.
     */
    for (int pass = 0; pass < 2; pass++) {
        int e = (pass == 0) ? front_edge : side_edge;
        int a = e, b = (e + 1) & 3;
        int ax = cx_[a], ay = cy_[a] - wall_base;
        int bx = cx_[b], by = cy_[b] - wall_base;

        /* Sample window centers along the wall's base line, lifted to mid-wall */
        int count = 4;
        int wall_len = abs(bx - ax);
        if (wall_len < 36) count = 2;
        else if (wall_len < 64) count = 3;

        bool is_front = (pass == 0);

        /* Skip placing windows where the door will go, on the south wall */
        bool skip_center = (e == 2 && is_front);

        for (int i = 1; i <= count; i++) {
            float t = (float)i / (float)(count + 1);
            int wx = ax + (int)(t * (float)(bx - ax));
            int wy = ay + (int)(t * (float)(by - ay));

            if (skip_center && i == (count + 1) / 2) continue;
            /* If the wall is too narrow for center skip, offset window slightly */
            if (skip_center && count == 2) {
                /* For 2-window south wall, keep them at 0.25 and 0.75 already */
            }

            /* Upper-floor window */
            int up_top = wy - H_WALL + 6;
            R(wx - 2, up_top,     5, 7, C_WALL_SHADE);
            R(wx - 2, up_top + 1, 5, 5, C_SKY_MID);
            R(wx,     up_top + 1, 1, 5, C_WALL_SHADE);
            R(wx - 2, up_top + 3, 5, 1, C_WALL_SHADE);
            R(wx - 1, up_top + 1, 1, 1, C_GOLD_LIGHT);

            /* Lower-floor window (front wall only, and only if not under door) */
            if (is_front) {
                int lo_top = wy - 16;
                R(wx - 2, lo_top,     5, 7, C_WALL_SHADE);
                R(wx - 2, lo_top + 1, 5, 5, C_SKY_MID);
                R(wx,     lo_top + 1, 1, 5, C_WALL_SHADE);
                R(wx - 2, lo_top + 3, 5, 1, C_WALL_SHADE);
                R(wx - 1, lo_top + 1, 1, 1, C_GOLD_LIGHT);
            }
        }
    }

    /* ---------- 8. Door (world-south wall, when visible) ----------------
     *
     * The wall from cx_[2],cy_[2]-wall_base → cx_[3],cy_[3]-wall_base is
     * sloped in screen space. The door must share that slope so it reads as
     * embedded in the wall rather than pasted flat on top. We draw the door
     * as a wall-aligned parallelogram (fill_wall), then slope every detail
     * (frame, panels, knob) along the same base line.                      */
    if (edge_vis[2]) {
        int a = 2, b = 3;
        int ax = cx_[a], ay = cy_[a] - wall_base;
        int bx = cx_[b], by = cy_[b] - wall_base;

        /* Door base: a short run centred at t=0.5 along the wall base. */
        const int dw = 11; /* screen-x width of the door */
        const int dh = 16;
        int       mid_x = (ax + bx) / 2;
        int       dleft = mid_x - dw / 2;
        int       dright = mid_x + dw / 2;

        int denom = bx - ax;
        if (denom == 0) denom = 1;
        int dleft_y  = ay + ((by - ay) * (dleft  - ax) + (denom >> 1)) / denom;
        int dright_y = ay + ((by - ay) * (dright - ax) + (denom >> 1)) / denom;

        /* Slope helper: screen-y on the wall base at any screen-x */
        #define BASE_Y_AT(X) (ay + ((by - ay) * ((X) - ax) + (denom >> 1)) / denom)

        /* 1-px frame — draw body slightly larger with trim colour */
        fill_wall(dleft - 1, dleft_y,  dright + 1, dright_y, dh + 1, C_WALL_TRIM);
        /* Door body */
        fill_wall(dleft, dleft_y, dright, dright_y, dh, C_TREE_TRUNK);

        /* Four door panels — thin wall-aligned inserts */
        {
            int pw = 3;          /* panel width */
            int pad_top   = 2;   /* gap from door top */
            int pad_mid   = 2;   /* vertical gap between panel rows */
            int panel_h   = (dh - pad_top - pad_mid - 3) / 2;
            if (panel_h < 3) panel_h = 3;

            int col_xs[2] = { dleft + 1, dright - 1 - pw };
            for (int c = 0; c < 2; c++) {
                int px0 = col_xs[c];
                int px1 = px0 + pw;
                int py0 = BASE_Y_AT(px0);
                int py1 = BASE_Y_AT(px1);
                /* Upper-row panel */
                fill_wall(px0, py0 - dh + pad_top + panel_h,
                          px1, py1 - dh + pad_top + panel_h, panel_h, C_PATH_DARK);
                /* Lower-row panel */
                fill_wall(px0, py0 - pad_top,
                          px1, py1 - pad_top, panel_h, C_PATH_DARK);
            }
        }

        /* Slim arched lintel highlight just below the top of the door */
        fill_wall(dleft + 1, dleft_y  - dh + 2,
                  dright - 1, dright_y - dh + 2, 1, C_BORDER_ACT);

        /* Knob — 2px square on the right half of the door, at hand height */
        {
            int kx = dright - 3;
            int ky = BASE_Y_AT(kx) - dh / 2;
            R(kx, ky, 2, 2, C_GOLD);
        }

        /* Welcome mat: a small iso-diamond flagstone on the courtyard just
         * below the door base (follows camera rotation naturally).        */
        {
            int mat_cx = mid_x;
            int mat_cy = (ay + by) / 2 + 3; /* 3 px below wall base */
            iso_diamond(mat_cx, mat_cy, 7, 3, C_PATH_LIGHT);
            iso_diamond(mat_cx, mat_cy, 4, 2, C_GOLD_LIGHT);
        }

        #undef BASE_Y_AT
    }

    /* ---------- 9. Pyramidal roof --------------------------------------- */
    /* 4 top-of-wall corners and 1 apex (over footprint centre).           */
    int rtop_x[4], rtop_y[4];
    for (int i = 0; i < 4; i++) {
        rtop_x[i] = cx_[i];
        rtop_y[i] = cy_[i] - wall_base - H_WALL;
    }
    int apex_x = fcx;
    int apex_y = fcy - wall_base - H_WALL - H_ROOF;

    /* 4 roof panels. Sort by average screen Y (back-to-front) so visible
     * panels naturally overdraw hidden ones. */
    typedef struct { int a, b; int mid_y; } RPanel;
    RPanel panels[4];
    for (int e = 0; e < 4; e++) {
        panels[e].a = e;
        panels[e].b = (e + 1) & 3;
        panels[e].mid_y = (rtop_y[e] + rtop_y[(e + 1) & 3] + apex_y) / 3;
    }
    /* Simple bubble sort by mid_y ascending (smallest = farthest back) */
    for (int i = 0; i < 4; i++)
        for (int j = i + 1; j < 4; j++)
            if (panels[j].mid_y < panels[i].mid_y) {
                RPanel t = panels[i]; panels[i] = panels[j]; panels[j] = t;
            }

    const uint16_t ROOF_LIGHT = HEX(0x9a1d4a); /* terracotta highlight */
    const uint16_t ROOF_MID   = HEX(0x7a1238); /* main roof */
    const uint16_t ROOF_DARK  = HEX(0x4d0a22); /* shadowed */

    for (int k = 0; k < 4; k++) {
        int a = panels[k].a, b = panels[k].b;
        bool front_panel = (a == front) || (b == front);
        uint16_t col = front_panel ? ROOF_MID : ROOF_DARK;
        fill_tri(rtop_x[a], rtop_y[a],
                 rtop_x[b], rtop_y[b],
                 apex_x,    apex_y, col);
    }
    /* Ridge highlight along the two front ridge lines (apex to front corners
     * of the visible panels) */
    for (int k = 0; k < 4; k++) {
        int a = panels[k].a, b = panels[k].b;
        bool front_panel = (a == front) || (b == front);
        if (!front_panel) continue;
        /* Hairline on the corner-to-apex ridge that leads to the front corner */
        int ridge_from_x, ridge_from_y;
        if (a == front) {
            ridge_from_x = rtop_x[a]; ridge_from_y = rtop_y[a];
        } else {
            ridge_from_x = rtop_x[b]; ridge_from_y = rtop_y[b];
        }
        int steps = abs(apex_y - ridge_from_y) + abs(apex_x - ridge_from_x);
        if (steps < 1) steps = 1;
        int sx0 = ridge_from_x, sy0 = ridge_from_y;
        int ex0 = apex_x, ey0 = apex_y;
        int n = steps;
        for (int i = 0; i <= n; i++) {
            int xi = sx0 + (ex0 - sx0) * i / n;
            int yi = sy0 + (ey0 - sy0) * i / n;
            R(xi, yi, 1, 1, ROOF_LIGHT);
        }
    }
    /* Eave line — 1-pixel band around the roof base on visible panels */
    for (int e = 0; e < 4; e++) {
        if (!edge_vis[e]) continue;
        int a = e, b = (e + 1) & 3;
        fill_wall(rtop_x[a], rtop_y[a] + 1, rtop_x[b], rtop_y[b] + 1, 1, ROOF_DARK);
    }

    /* ---------- 10. Chimney --------------------------------------------- */
    {
        /* Chimney sits on the roof slope, biased toward the front so it's
         * visible. We put it at ~25% along the apex→front ridge. */
        int rfx = rtop_x[front];
        int rfy = rtop_y[front];
        int chx = apex_x + (rfx - apex_x) * 4 / 10;
        int chy = apex_y + (rfy - apex_y) * 4 / 10;
        int cw = 6, ch = 9;
        R(chx - cw / 2, chy - ch,     cw,     ch,     C_ROCK_DARK);
        R(chx - cw / 2, chy - ch,     cw,     2,      C_ROCK_LIGHT);
        R(chx - cw / 2 - 1, chy - ch - 2, cw + 2, 2,  C_ROCK_MID);
        /* Smoke puff */
        R(chx - 1, chy - ch - 4, 3, 2, HEX(0x6b7280));
        R(chx,     chy - ch - 6, 2, 2, HEX(0x9ca3af));
    }
}

/* ------------------------------------------------------------------ */
/* Player top-down sprite                                               */
/* ------------------------------------------------------------------ */
void spr_player_td_foot(int foot_sx, int foot_sy, uint8_t dir, float walk_frame,
                        uint8_t skin_idx, uint8_t hair_idx, uint8_t outfit_idx)
{
    (void)walk_frame;
    iso_draw_player_foot(foot_sx, foot_sy, dir, skin_idx, hair_idx, outfit_idx);
}

void spr_player_td(int sx, int sy, uint8_t dir, float walk_frame,
                   uint8_t skin_idx, uint8_t hair_idx, uint8_t outfit_idx) {
    (void)skin_idx; (void)hair_idx; (void)outfit_idx;
    if (iso_draw_td_player_char(sx, sy, dir, (uint8_t)walk_frame))
        return;

    uint16_t skin   = C_SKIN_TONES[skin_idx < 4 ? skin_idx : 0];
    uint16_t hair   = C_HAIR_COLS[hair_idx < 6 ? hair_idx : 0];
    uint16_t outfit = C_OUTFIT_COLS[outfit_idx < 6 ? outfit_idx : 0];

    int frame = (int)walk_frame;

    /* Body */
    R(sx+3, sy+6, 10, 8, outfit);
    /* Legs — alternating walk */
    if (frame == 0) {
        R(sx+4, sy+12, 3, 4, C_PLAYER_LEGS);
        R(sx+9, sy+12, 3, 4, C_PLAYER_LEGS);
    } else {
        R(sx+4, sy+11, 3, 5, C_PLAYER_LEGS);
        R(sx+9, sy+13, 3, 3, C_PLAYER_LEGS);
    }
    /* Head */
    R(sx+3, sy+1, 10, 8, skin);
    /* Hair */
    R(sx+3, sy+1, 10, 3, hair);
    /* Eyes based on direction */
    switch (dir) {
        case DIR_DOWN:
            R(sx+5, sy+5, 2, 2, C_EYE);
            R(sx+9, sy+5, 2, 2, C_EYE);
            break;
        case DIR_UP:
            /* back of head */
            break;
        case DIR_LEFT:
            R(sx+4, sy+5, 2, 2, C_EYE);
            break;
        case DIR_RIGHT:
            R(sx+10, sy+5, 2, 2, C_EYE);
            break;
    }
}

/* ------------------------------------------------------------------ */
/* Player side-view sprite                                              */
/* ------------------------------------------------------------------ */
void spr_player_sv(int sx, int sy, uint8_t dir, float walk_frame, bool jumping,
                   uint8_t skin_idx, uint8_t hair_idx, uint8_t outfit_idx) {
    (void)skin_idx; (void)hair_idx; (void)outfit_idx;
    uint8_t eff_frame = jumping ? 1 : (uint8_t)walk_frame;
    if (iso_draw_sv_player(sx, sy, dir, eff_frame))
        return;

    uint16_t skin   = C_SKIN_TONES[skin_idx < 4 ? skin_idx : 0];
    uint16_t hair   = C_HAIR_COLS[hair_idx < 6 ? hair_idx : 0];
    uint16_t outfit = C_OUTFIT_COLS[outfit_idx < 6 ? outfit_idx : 0];

    int frame = jumping ? 1 : (int)walk_frame;
    (void)dir;

    /* Body */
    R(sx+2, sy+5, 10, 7, outfit);
    /* Legs */
    if (frame == 0) {
        R(sx+3, sy+11, 3, 5, C_PLAYER_LEGS);
        R(sx+8, sy+11, 3, 5, C_PLAYER_LEGS);
    } else {
        R(sx+3, sy+10, 3, 6, C_PLAYER_LEGS);
        R(sx+8, sy+12, 3, 4, C_PLAYER_LEGS);
    }
    /* Head */
    R(sx+2, sy,   10, 7, skin);
    /* Hair */
    R(sx+2, sy,   10, 3, hair);
    /* Eye (always facing right for simplicity; flip if left) */
    if (dir == DIR_LEFT)
        R(sx+3, sy+3, 2, 2, C_EYE);
    else
        R(sx+7, sy+3, 2, 2, C_EYE);
}

/* ------------------------------------------------------------------ */
/* Monster sprites                                                      */
/* ------------------------------------------------------------------ */
void spr_monster_foot(int foot_sx, int foot_sy, uint8_t evo_stage, float walk_frame)
{
    iso_draw_monster_foot(foot_sx, foot_sy, evo_stage, (uint8_t)walk_frame);
}

void spr_monster(int sx, int sy, uint8_t evo_stage, float walk_frame) {
    if (iso_draw_monster_sm(sx, sy, evo_stage, (uint8_t)walk_frame))
        return;

    int frame = (int)walk_frame & 1;
    int bob   = frame;

    switch (evo_stage) {
    case 0:
        /* Stage 0: small purple blob */
        R(sx+4, sy+6+bob, 8,  6, C_MON_PURPLE_M);
        R(sx+3, sy+7+bob, 10, 4, C_MON_PURPLE_M);
        /* Eyes */
        R(sx+5, sy+8+bob, 2, 2, C_EYE);
        R(sx+9, sy+8+bob, 2, 2, C_EYE);
        /* Tiny bumps */
        R(sx+3, sy+5+bob, 3, 2, C_MON_PURPLE_D);
        R(sx+10,sy+5+bob, 3, 2, C_MON_PURPLE_D);
        break;

    case 1:
        /* Stage 1: medium with horns */
        R(sx+3, sy+4+bob, 10, 9, C_MON_PURPLE_M);
        R(sx+2, sy+6+bob, 12, 6, C_MON_PURPLE_M);
        /* Horns */
        R(sx+4, sy+1+bob, 2, 4, C_MON_PURPLE_D);
        R(sx+10,sy+1+bob, 2, 4, C_MON_PURPLE_D);
        /* Eyes */
        R(sx+5, sy+6+bob, 2, 2, C_EYE);
        R(sx+9, sy+6+bob, 2, 2, C_EYE);
        /* Legs */
        R(sx+4, sy+12,    3, 3, C_MON_DARK);
        R(sx+9, sy+12,    3, 3, C_MON_DARK);
        break;

    case 2:
        /* Stage 2: armoured beast */
        R(sx+2, sy+3+bob, 12, 10, C_MON_PURPLE_D);
        R(sx+1, sy+5+bob, 14,  7, C_MON_PURPLE_M);
        /* Armour plates */
        R(sx+3, sy+4+bob,  4,  3, C_MON_DARK);
        R(sx+9, sy+4+bob,  4,  3, C_MON_DARK);
        /* Eyes — glowing */
        R(sx+4, sy+6+bob, 3, 2, C_MON_RED);
        R(sx+9, sy+6+bob, 3, 2, C_MON_RED);
        /* Legs */
        R(sx+3, sy+12, 3, 4, C_MON_DARK);
        R(sx+8, sy+12, 3, 4, C_MON_DARK);
        /* Tail */
        R(sx+12, sy+8+bob, 3, 2, C_MON_PURPLE_B);
        break;

    case 3:
    default:
        /* Stage 3: imposing warlord */
        R(sx+1, sy+2+bob, 14, 12, C_MON_DARK);
        R(sx+2, sy+3+bob, 12, 10, C_MON_PURPLE_D);
        R(sx+3, sy+4+bob, 10,  8, C_MON_PURPLE_M);
        /* Crown */
        R(sx+4, sy+0+bob, 8,  3, C_MON_CROWN);
        R(sx+5, sy+0+bob, 2,  4, C_MON_CROWN);
        R(sx+9, sy+0+bob, 2,  4, C_MON_CROWN);
        /* Eyes — fierce */
        R(sx+4, sy+6+bob, 3, 2, C_MON_RED_L);
        R(sx+9, sy+6+bob, 3, 2, C_MON_RED_L);
        /* Shoulders */
        R(sx+0, sy+4+bob, 3, 4, C_MON_DARK);
        R(sx+13,sy+4+bob, 3, 4, C_MON_DARK);
        /* Legs */
        R(sx+3, sy+12, 4, 4, C_MON_DARK);
        R(sx+9, sy+12, 4, 4, C_MON_DARK);
        break;
    }
}

/* ------------------------------------------------------------------ */
/* Large monster for battle screen (2× size)                           */
/* ------------------------------------------------------------------ */
void spr_monster_large(int sx, int sy, uint8_t evo_stage) {
    if (iso_draw_monster_lg(sx, sy, evo_stage))
        return;

#define R2(x,y,w,h,c) R(sx + (x)*2, sy + (y)*2, (w)*2, (h)*2, c)

    switch (evo_stage) {
    case 0:
        R2(4,6, 8, 6, C_MON_PURPLE_M);
        R2(3,7,10, 4, C_MON_PURPLE_M);
        R2(5,8, 2, 2, C_EYE);
        R2(9,8, 2, 2, C_EYE);
        R2(3,5, 3, 2, C_MON_PURPLE_D);
        R2(10,5,3, 2, C_MON_PURPLE_D);
        break;
    case 1:
        R2(3,4,10, 9, C_MON_PURPLE_M);
        R2(2,6,12, 6, C_MON_PURPLE_M);
        R2(4,1, 2, 4, C_MON_PURPLE_D);
        R2(10,1,2, 4, C_MON_PURPLE_D);
        R2(5,6, 2, 2, C_EYE);
        R2(9,6, 2, 2, C_EYE);
        R2(4,12,3, 3, C_MON_DARK);
        R2(9,12,3, 3, C_MON_DARK);
        break;
    case 2:
        R2(2,3,12,10, C_MON_PURPLE_D);
        R2(1,5,14, 7, C_MON_PURPLE_M);
        R2(3,4, 4, 3, C_MON_DARK);
        R2(9,4, 4, 3, C_MON_DARK);
        R2(4,6, 3, 2, C_MON_RED);
        R2(9,6, 3, 2, C_MON_RED);
        R2(3,12,3, 4, C_MON_DARK);
        R2(8,12,3, 4, C_MON_DARK);
        R2(12,8,3, 2, C_MON_PURPLE_B);
        break;
    case 3:
    default:
        R2(1,2,14,12, C_MON_DARK);
        R2(2,3,12,10, C_MON_PURPLE_D);
        R2(3,4,10, 8, C_MON_PURPLE_M);
        R2(4,0, 8, 3, C_MON_CROWN);
        R2(5,0, 2, 4, C_MON_CROWN);
        R2(9,0, 2, 4, C_MON_CROWN);
        R2(4,6, 3, 2, C_MON_RED_L);
        R2(9,6, 3, 2, C_MON_RED_L);
        R2(0,4, 3, 4, C_MON_DARK);
        R2(13,4,3, 4, C_MON_DARK);
        R2(3,12,4, 4, C_MON_DARK);
        R2(9,12,4, 4, C_MON_DARK);
        break;
    }
#undef R2
}

/* ------------------------------------------------------------------ */
/* Furniture sprites                                                    */
/* ------------------------------------------------------------------ */
void spr_furniture(FurnitureType type, int sx, int sy) {
    if (iso_draw_furniture(sx, sy, (uint8_t)type))
        return;

    switch (type) {
    case FURN_BED:
        R(sx,    sy+4, 16, 10, C_PATH_DARK);
        R(sx+1,  sy+5, 14,  8, C_PLAYER_BODY);
        R(sx,    sy+4,  5, 10, C_ROCK_LIGHT); /* pillow */
        break;
    case FURN_WORKBENCH:
        R(sx,    sy+6, 16,  6, C_TREE_TRUNK);
        R(sx,    sy+4, 16,  3, C_TREE_MID);
        R(sx+1,  sy+10, 3,  6, C_TREE_TRUNK);
        R(sx+12, sy+10, 3,  6, C_TREE_TRUNK);
        break;
    case FURN_FORGE:
        R(sx+2, sy+4, 12, 10, C_ROCK_DARK);
        R(sx+3, sy+2,  4,  4, C_ROCK_MID);
        R(sx+4, sy+3,  2,  3, C_MON_RED);    /* flame */
        R(sx+3, sy+10, 10, 2, C_ROCK_MID);
        break;
    case FURN_FARM:
        R(sx, sy+8, 16, 6, C_SAND_DARK); /* soil */
        R(sx, sy+7, 16, 2, C_SAND);
        /* Sprout */
        R(sx+7, sy+3, 2, 5, C_GRASS_LIGHT);
        R(sx+5, sy+4, 6, 2, C_TREE_LIGHT);
        break;
    case FURN_CHEST:
        R(sx+1, sy+4, 14, 10, C_TREE_TRUNK);
        R(sx+1, sy+4, 14,  3, C_TREE_MID);
        R(sx+7, sy+8,  2,  2, C_GOLD);
        break;
    case FURN_PET_BED:
        R(sx,    sy+8, 16,  6, C_MON_PURPLE_D);
        R(sx+1,  sy+9, 14,  4, C_MON_PURPLE_M);
        break;
    case FURN_PET_HOUSE:
        R(sx+2, sy+4, 12, 10, C_ROCK_MID);
        R(sx,   sy+2, 16,  4, C_ROCK_LIGHT); /* roof */
        R(sx+6, sy+8,  4,  6, C_BG_DARK);    /* door */
        break;
    default:
        break;
    }
}

/* ------------------------------------------------------------------ */
/* XP drop floating text                                               */
/* ------------------------------------------------------------------ */
void spr_xp_drop(int x, int y, uint16_t amount) {
    /* Build string "+NNN XP" */
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
void spr_skill_indicator(int sx, int sy, float progress) {
    int bar_w = 20;
    int bar_h = 4;
    /* Background */
    R(sx - bar_w/2, sy - 12, bar_w, bar_h, C_BG);
    /* Fill */
    int fill = (int)(progress * bar_w);
    if (fill > bar_w) fill = bar_w;
    R(sx - bar_w/2, sy - 12, fill, bar_h, C_XP_PURPLE);
}
