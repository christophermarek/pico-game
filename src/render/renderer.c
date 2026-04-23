#include "renderer.h"
#include "sprites.h"
#include "font.h"
#include "hal.h"
#include "colors.h"
#include "config.h"
#include "../ui/hud.h"
#include "../ui/menu.h"
#include "../game/td_cam.h"
#include "../game/player.h"
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

static void td_world_to_screen(const GameState *s, const TdCamBasis *cam, float wx, float wy,
                               int *sx, int *sy)
{
    td_basis_world_pixel_to_screen(cam, s->td.x, s->td.y, wx, wy, sx, sy);
}

static bool td_cell_on_screen(const GameState *s, const TdCamBasis *cam, int tx, int ty);
static void draw_line(int x0, int y0, int x1, int y1, uint16_t color);

/* Draw a world-space circle: polygon-approximate it and project every vertex
 * through the iso camera, so the result is the correct diagonal ellipse on
 * screen for the current bearing. Use this for debug visualisations where
 * the shape must match the actual world-space collision zone. */
static void draw_world_circle(const GameState *s, const TdCamBasis *cam,
                              float wcx, float wcy, float r, uint16_t color)
{
    enum { SEG = 24 };
    int px = 0, py = 0;
    for (int i = 0; i <= SEG; i++) {
        float t  = (float)i * (6.2831853f / (float)SEG);
        float wx = wcx + r * cosf(t);
        float wy = wcy + r * sinf(t);
        int   sx, sy;
        td_world_to_screen(s, cam, wx, wy, &sx, &sy);
        if (i > 0) draw_line(px, py, sx, sy, color);
        px = sx; py = sy;
    }
}

/* Midpoint circle (Bresenham) — screen-space circle outline */
static void draw_circle(int cx, int cy, int r, uint16_t color)
{
    int x = r, y = 0, err = 0;
    while (x >= y) {
        hal_pixel(cx + x, cy + y, color);
        hal_pixel(cx + y, cy + x, color);
        hal_pixel(cx - y, cy + x, color);
        hal_pixel(cx - x, cy + y, color);
        hal_pixel(cx - x, cy - y, color);
        hal_pixel(cx - y, cy - x, color);
        hal_pixel(cx + y, cy - x, color);
        hal_pixel(cx + x, cy - y, color);
        y++;
        if (err <= 0) { err += 2 * y + 1; }
        else          { x--; err += 2 * (y - x) + 1; }
    }
}

/* Bresenham line between two screen points */
static void draw_line(int x0, int y0, int x1, int y1, uint16_t color)
{
    int dx  =  (x1 > x0 ? x1 - x0 : x0 - x1);
    int sx  =   x0 < x1 ? 1 : -1;
    int dy  = -(y1 > y0 ? y1 - y0 : y0 - y1);
    int sy  =   y0 < y1 ? 1 : -1;
    int err =  dx + dy;
    for (;;) {
        hal_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

/* Player hitbox debug: screen-space circle at the foot anchor. */
static void draw_debug_hitbox(const GameState *s, const TdCamBasis *cam)
{
    int psx, psy;
    td_world_to_screen(s, cam, s->td.x, s->td.y + TD_FEET_OFF, &psx, &psy);
    draw_circle(psx, psy, 6, HEX(0xef4444)); /* red */
}

/* Orange outline showing the actual collision footprint for each blocked tile.
 * Trees use the trunk sub-rectangle (south-centre of the tile) matching
 * the tighter check in td_collides; other obstacles use the full tile. */
static void draw_debug_tile_hitboxes(const GameState *s, const World *w,
                                     const TdCamBasis *cam)
{
    uint16_t col = HEX(0xff8c00); /* orange */
    for (int ty = 0; ty < MAP_H; ty++) {
        for (int tx = 0; tx < MAP_W; tx++) {
            if (world_walkable(w, tx, ty)) continue;
            if (!td_cell_on_screen(s, cam, tx, ty)) continue;

            /* Unified debug: draw the world-space collision circle for every
             * non-walkable tile, centred on the tile centre. Inner dot marks
             * the tile centre so you can see the collision origin clearly. */
            float tcx = (float)(tx * TILE + TILE / 2);
            float tcy = (float)(ty * TILE + TILE / 2);
            int   tsx, tsy;
            td_world_to_screen(s, cam, tcx, tcy, &tsx, &tsy);
            draw_world_circle(s, cam, tcx, tcy,
                              (float)(PL_HALF_W + OBSTACLE_R), col);
            draw_circle(tsx, tsy, 1, col);
        }
    }
}

/* Gold arrow from sprite centre toward the tile the player is facing */
static void draw_debug_dir(const GameState *s, const TdCamBasis *cam)
{
    int psx, psy;
    td_world_to_screen(s, cam, s->td.x, s->td.y + TD_FEET_OFF, &psx, &psy);

    int ddx = 0, ddy = 0;
    switch (s->td.dir) {
        case 0 /* DIR_DOWN  */: ddy = +1; break;
        case 1 /* DIR_UP    */: ddy = -1; break;
        case 2 /* DIR_LEFT  */: ddx = -1; break;
        case 3 /* DIR_RIGHT */: ddx = +1; break;
    }
    float ftx = (float)((s->td.tile_x + ddx) * TILE + TILE / 2);
    float fty = (float)((s->td.tile_y + ddy) * TILE + TILE / 2);
    int fsx, fsy;
    td_world_to_screen(s, cam, ftx, fty, &fsx, &fsy);
    draw_line(psx, psy, fsx, fsy, HEX(0xfbbf24)); /* gold arrow */
}

/* Write decimal (optionally signed, 1-decimal fixed point) into buf. */
static int fmt_int(char *buf, int v)
{
    int n = 0;
    if (v < 0) { buf[n++] = '-'; v = -v; }
    char tmp[8]; int tn = 0;
    if (v == 0) tmp[tn++] = '0';
    while (v > 0) { tmp[tn++] = (char)('0' + v % 10); v /= 10; }
    while (tn > 0) buf[n++] = tmp[--tn];
    return n;
}

/* Collision debug panel:
 *  - Tests player_test_collide at (px±1, py) and (px, py±1): small coloured
 *    ticks on screen indicate blocked (red) / free (green) directions.
 *  - Finds the nearest T_TREE stump and draws a magenta line to it with
 *    the world distance.
 *  - Prints player world pos (px, py) and "BLK:UDLR" flags top-left. */
static void draw_debug_collision(const GameState *s, const World *w, const TdCamBasis *cam)
{
    int  who_tx[5]   = {-1,-1,-1,-1,-1};
    int  who_ty[5]   = {-1,-1,-1,-1,-1};
    char who_kind[5] = {'-','-','-','-','-'};
    bool blk_self  = player_collide_who(w, s->td.x,        s->td.y,
                                        &who_tx[0], &who_ty[0], &who_kind[0]);
    bool blk_left  = player_collide_who(w, s->td.x - 1.0f, s->td.y,
                                        &who_tx[1], &who_ty[1], &who_kind[1]);
    bool blk_right = player_collide_who(w, s->td.x + 1.0f, s->td.y,
                                        &who_tx[2], &who_ty[2], &who_kind[2]);
    bool blk_up    = player_collide_who(w, s->td.x,        s->td.y - 1.0f,
                                        &who_tx[3], &who_ty[3], &who_kind[3]);
    bool blk_down  = player_collide_who(w, s->td.x,        s->td.y + 1.0f,
                                        &who_tx[4], &who_ty[4], &who_kind[4]);

    /* Ticks around the foot anchor, one per world axis. */
    int fsx, fsy;
    td_world_to_screen(s, cam, s->td.x, s->td.y + TD_FEET_OFF, &fsx, &fsy);
    uint16_t red   = HEX(0xef4444);
    uint16_t green = HEX(0x22c55e);
    hal_fill_rect(fsx - 18, fsy - 1, 4, 2, blk_left  ? red : green); /* world -X */
    hal_fill_rect(fsx + 14, fsy - 1, 4, 2, blk_right ? red : green); /* world +X */
    hal_fill_rect(fsx - 1,  fsy - 14, 2, 4, blk_up    ? red : green); /* world -Y */
    hal_fill_rect(fsx - 1,  fsy + 10, 2, 4, blk_down  ? red : green); /* world +Y */

    /* Nearest non-walkable tile (obstacle), centred on tile centre. */
    float best_d  = 1e9f;
    float best_sx = 0, best_sy = 0;
    int   best_tx = -1, best_ty = -1;
    for (int ty = 0; ty < MAP_H; ty++) {
        for (int tx = 0; tx < MAP_W; tx++) {
            if (world_walkable(w, tx, ty)) continue;
            float scx = (float)(tx * TILE + TILE / 2);
            float scy = (float)(ty * TILE + TILE / 2);
            float d   = hypotf(s->td.x - scx, s->td.y - scy);
            if (d < best_d) { best_d = d; best_sx = scx; best_sy = scy;
                              best_tx = tx; best_ty = ty; }
        }
    }
    if (best_tx >= 0) {
        int psx, psy, ssx, ssy;
        td_world_to_screen(s, cam, s->td.x, s->td.y, &psx, &psy);
        td_world_to_screen(s, cam, best_sx, best_sy, &ssx, &ssy);
        uint16_t mag = HEX(0xd946ef); /* magenta */
        draw_line(psx, psy, ssx, ssy, mag);
        /* Stop-distance ring at the player's world position, drawn as a
         * proper world-space circle → matches the true iso ellipse shape.
         * Any obstacle centre dot touching this ring means collision. */
        draw_world_circle(s, cam, s->td.x, s->td.y,
                          (float)(PL_HALF_W + OBSTACLE_R), mag);
    }

    /* Text readout: "P=xxx,yyy  BLK:UDLR  DST=nn" top-left. */
    char line[40];
    int  n = 0;
    line[n++] = 'P'; line[n++] = '=';
    n += fmt_int(line + n, (int)s->td.x);
    line[n++] = ',';
    n += fmt_int(line + n, (int)s->td.y);
    line[n++] = ' ';
    line[n++] = 'B'; line[n++] = 'L'; line[n++] = 'K';
    line[n++] = blk_self ? '!' : ':';
    line[n++] = blk_up    ? 'U' : '-';
    line[n++] = blk_down  ? 'D' : '-';
    line[n++] = blk_left  ? 'L' : '-';
    line[n++] = blk_right ? 'R' : '-';
    line[n]   = '\0';
    font_draw_str(line, 2, 20, C_TEXT_WHITE, 1);

    if (best_tx >= 0) {
        char l2[40];
        int  m = 0;
        l2[m++] = 'T'; l2[m++] = '='; /* nearest tree tile */
        m += fmt_int(l2 + m, best_tx);
        l2[m++] = ',';
        m += fmt_int(l2 + m, best_ty);
        l2[m++] = ' '; l2[m++] = 'd'; l2[m++] = '=';
        m += fmt_int(l2 + m, (int)(best_d + 0.5f));
        l2[m]   = '\0';
        font_draw_str(l2, 2, 30, C_TEXT_WHITE, 1);
    }

    /* Third line: per-direction blocker identity (U/D/L/R). */
    {
        char l3[48];
        int  m = 0;
        const char *dirnames = "UDLR";
        for (int i = 0; i < 4; i++) {
            int idx = i + 1;  /* who_* index: 1..4 */
            if (who_kind[idx] == '-') continue;
            l3[m++] = dirnames[i];
            l3[m++] = ':';
            l3[m++] = who_kind[idx];
            m += fmt_int(l3 + m, who_tx[idx]);
            l3[m++] = ',';
            m += fmt_int(l3 + m, who_ty[idx]);
            l3[m++] = ' ';
        }
        l3[m] = '\0';
        if (m > 0) font_draw_str(l3, 2, 40, C_TEXT_WHITE, 1);
    }

    /* Red world-circle on top of each blocking obstacle to make it visually
     * obvious which tile is the actual blocker in each direction. */
    uint16_t hi = HEX(0xef4444);
    for (int i = 0; i < 5; i++) {
        if (who_kind[i] == '-') continue;
        float scx = (float)(who_tx[i] * TILE + TILE / 2);
        float scy = (float)(who_ty[i] * TILE + TILE / 2);
        draw_world_circle(s, cam, scx, scy,
                          (float)(PL_HALF_W + OBSTACLE_R), hi);
    }
}

typedef struct {
    int64_t key;
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

static bool td_cell_on_screen(const GameState *s, const TdCamBasis *cam, int tx, int ty)
{
    float tcx = (float)(tx * TILE + TILE / 2);
    float tcy = (float)(ty * TILE + TILE / 2);
    int   sx, sy;
    td_world_to_screen(s, cam, tcx, tcy, &sx, &sy);
    return sx >= -TD_ISO_CULL && sx < DISPLAY_W + TD_ISO_CULL &&
           sy >= -TD_ISO_CULL && sy < DISPLAY_H + TD_ISO_CULL;
}

void render_topdown(GameState *s, const World *w)
{
    TdCamBasis cam = td_cam_basis(s->td_cam_bearing);
    TdCellSort cells[MAP_W * MAP_H];
    int        n = 0;

    hal_fill(C_SKY_BOT);

    for (int ty = 0; ty < MAP_H; ty++) {
        for (int tx = 0; tx < MAP_W; tx++) {
            if (!td_cell_on_screen(s, &cam, tx, ty)) continue;
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
        int   tx  = cells[i].tx;
        int   ty  = cells[i].ty;
        float tcx = (float)(tx * TILE + TILE / 2);
        float tcy = (float)(ty * TILE + TILE / 2);
        int   sx, sy;
        td_world_to_screen(s, &cam, tcx, tcy, &sx, &sy);
        uint8_t tile = world_tile(w, tx, ty);
        spr_tile_iso_floor(tile, sx, sy, s->frame_count);
    }
    for (int i = 0; i < n; i++) {
        int   tx  = cells[i].tx;
        int   ty  = cells[i].ty;
        float tcx = (float)(tx * TILE + TILE / 2);
        float tcy = (float)(ty * TILE + TILE / 2);
        int   sx, sy;
        td_world_to_screen(s, &cam, tcx, tcy, &sx, &sy);
        uint8_t tile = world_tile(w, tx, ty);
        spr_tile_iso_onlay(tile, sx, sy, s->frame_count);
        if (w->node_respawn[ty * MAP_W + tx] > 0)
            spr_iso_depleted_mark(sx, sy);
    }

    {
        int player_foot_sx, player_foot_sy;
        td_world_to_screen(s, &cam, s->td.x, s->td.y + TD_FEET_OFF,
                           &player_foot_sx, &player_foot_sy);
        spr_player_td_foot(player_foot_sx, player_foot_sy, s->td.screen_dir,
                           s->td.walk_frame);

        if (s->skilling) {
            float progress = 1.0f - (float)s->action_ticks_left / (float)ACTION_TICKS;
            spr_skill_indicator(player_foot_sx, player_foot_sy - 18, progress);
        }

        /* Debug overlays — only when debug mode is on */
        if (s->debug_mode) {
            draw_debug_tile_hitboxes(s, w, &cam);
            draw_debug_hitbox(s, &cam);
            draw_debug_dir(s, &cam);
            draw_debug_collision(s, w, &cam);
        }
    }
}

static void draw_fps(void)
{
    static uint32_t bucket_start = 0;
    static uint32_t frame_count  = 0;
    static uint32_t cached_fps   = 0;

    uint32_t now = hal_ticks_ms();
    frame_count++;
    if (now - bucket_start >= 1000u) {
        cached_fps   = frame_count;
        frame_count  = 0;
        bucket_start = now;
    }

    char buf[8];
    int  n = 0;
    uint32_t v = cached_fps;
    if (v >= 100) buf[n++] = (char)('0' + v / 100);
    if (v >=  10) buf[n++] = (char)('0' + (v / 10) % 10);
    buf[n++] = (char)('0' + v % 10);
    buf[n++] = 'F'; buf[n++] = 'P'; buf[n++] = 'S';
    buf[n]   = '\0';
    font_draw_str(buf, DISPLAY_W - n * 6 - 2, 2, C_TEXT_DIM, 1);
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
    default:
        hal_fill(C_BG_DARK);
        break;
    }
    draw_fps();
    hal_show();
}
