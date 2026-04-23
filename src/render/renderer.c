#include "renderer.h"
#include "iso_spritesheet.h"
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

static void draw_skill_indicator(int sx, int sy, float progress)
{
    int bar_w = 20, bar_h = 4;
    hal_fill_rect(sx - bar_w / 2, sy, bar_w, bar_h, C_BG);
    int fill = (int)(progress * bar_w);
    if (fill > bar_w) fill = bar_w;
    if (fill > 0) hal_fill_rect(sx - bar_w / 2, sy, fill, bar_h, C_XP_PURPLE);
}

/*
 * Polygon-approximate a world-space circle through the iso camera so the
 * result is the correct ellipse on screen for the current bearing. The
 * shape drawn IS the collision footprint.
 */
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

/* World-space axis-aligned square: project corners through the iso camera
 * (yields a parallelogram on screen) and connect them. */
static void draw_world_rect(const GameState *s, const TdCamBasis *cam,
                            float wx0, float wy0, float wx1, float wy1,
                            uint16_t color)
{
    int sx[4], sy[4];
    td_world_to_screen(s, cam, wx0, wy0, &sx[0], &sy[0]);
    td_world_to_screen(s, cam, wx1, wy0, &sx[1], &sy[1]);
    td_world_to_screen(s, cam, wx1, wy1, &sx[2], &sy[2]);
    td_world_to_screen(s, cam, wx0, wy1, &sx[3], &sy[3]);
    for (int i = 0; i < 4; i++) {
        int j = (i + 1) & 3;
        draw_line(sx[i], sy[i], sx[j], sy[j], color);
    }
}

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

/* Player hitbox — world-space circle at the foot anchor (where collision
 * actually tests). Foot dot marks the centre. */
static void draw_debug_hitbox(const GameState *s, const TdCamBasis *cam)
{
    uint16_t red = HEX(0xef4444);
    float    cy  = s->td.y + TD_FEET_OFF;
    draw_world_circle(s, cam, s->td.x, cy, (float)PL_RADIUS, red);
    int psx, psy;
    td_world_to_screen(s, cam, s->td.x, cy, &psx, &psy);
    hal_pixel(psx, psy, red);
}

static inline bool tile_is_circle(uint8_t tile_id) {
    return tile_id == T_TREE;
}

/* Draws the real collision shape per non-walkable tile (circle for trees,
 * square for rocks/ore/water) so the debug overlay matches what td_collides
 * actually tests. Inner dot marks the tile centre. */
static void draw_debug_tile_hitboxes(const GameState *s, const World *w,
                                     const TdCamBasis *cam)
{
    uint16_t col = HEX(0xff8c00);
    for (int ty = 0; ty < MAP_H; ty++) {
        for (int tx = 0; tx < MAP_W; tx++) {
            if (world_walkable(w, tx, ty)) continue;
            if (!td_cell_on_screen(s, cam, tx, ty)) continue;

            uint8_t tile = world_tile(w, tx, ty);
            float tcx = (float)(tx * TILE + TILE / 2);
            float tcy = (float)(ty * TILE + TILE / 2);
            if (tile_is_circle(tile)) {
                draw_world_circle(s, cam, tcx, tcy, (float)OBSTACLE_R, col);
            } else {
                float h = (float)OBSTACLE_HALF;
                draw_world_rect(s, cam, tcx - h, tcy - h, tcx + h, tcy + h, col);
            }
            int tsx, tsy;
            td_world_to_screen(s, cam, tcx, tcy, &tsx, &tsy);
            draw_circle(tsx, tsy, 1, col);
        }
    }
}

static void draw_debug_dir(const GameState *s, const TdCamBasis *cam)
{
    int psx, psy;
    td_world_to_screen(s, cam, s->td.x, s->td.y + TD_FEET_OFF, &psx, &psy);

    int ddx = 0, ddy = 0;
    switch (s->td.dir) {
        case DIR_DOWN:  ddy = +1; break;
        case DIR_UP:    ddy = -1; break;
        case DIR_LEFT:  ddx = -1; break;
        case DIR_RIGHT: ddx = +1; break;
    }
    float ftx = (float)((s->td.tile_x + ddx) * TILE + TILE / 2);
    float fty = (float)((s->td.tile_y + ddy) * TILE + TILE / 2);
    int fsx, fsy;
    td_world_to_screen(s, cam, ftx, fty, &fsx, &fsy);
    draw_line(psx, psy, fsx, fsy, HEX(0xfbbf24));
}

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

    int fsx, fsy;
    td_world_to_screen(s, cam, s->td.x, s->td.y + TD_FEET_OFF, &fsx, &fsy);
    uint16_t red   = HEX(0xef4444);
    uint16_t green = HEX(0x22c55e);
    hal_fill_rect(fsx - 18, fsy - 1,  4, 2, blk_left  ? red : green);
    hal_fill_rect(fsx + 14, fsy - 1,  4, 2, blk_right ? red : green);
    hal_fill_rect(fsx - 1,  fsy - 14, 2, 4, blk_up    ? red : green);
    hal_fill_rect(fsx - 1,  fsy + 10, 2, 4, blk_down  ? red : green);

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
        draw_line(psx, psy, ssx, ssy, HEX(0xd946ef));
    }

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
        l2[m++] = 'T'; l2[m++] = '=';
        m += fmt_int(l2 + m, best_tx);
        l2[m++] = ',';
        m += fmt_int(l2 + m, best_ty);
        l2[m++] = ' '; l2[m++] = 'd'; l2[m++] = '=';
        m += fmt_int(l2 + m, (int)(best_d + 0.5f));
        l2[m]   = '\0';
        font_draw_str(l2, 2, 30, C_TEXT_WHITE, 1);
    }

    {
        char l3[48];
        int  m = 0;
        const char *dirnames = "UDLR";
        for (int i = 0; i < 4; i++) {
            int idx = i + 1;
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

    /* Highlight each blocking tile with its actual collision shape. */
    uint16_t hi = HEX(0xef4444);
    for (int i = 0; i < 5; i++) {
        if (who_kind[i] == '-') continue;
        uint8_t tile = world_tile(w, who_tx[i], who_ty[i]);
        float scx = (float)(who_tx[i] * TILE + TILE / 2);
        float scy = (float)(who_ty[i] * TILE + TILE / 2);
        if (tile_is_circle(tile)) {
            draw_world_circle(s, cam, scx, scy, (float)OBSTACLE_R, hi);
        } else {
            float h = (float)OBSTACLE_HALF;
            draw_world_rect(s, cam, scx - h, scy - h, scx + h, scy + h, hi);
        }
    }

    /* Velocity arrow per axis: green = applied, red = blocked. Tiny per-frame
     * vectors are amplified so the indicator is visible. */
    {
        int psx, psy;
        td_world_to_screen(s, cam, s->td.x, s->td.y, &psx, &psy);
        float scale = 12.0f;
        if (s->dbg_dx != 0.0f) {
            int ex, ey;
            td_world_to_screen(s, cam, s->td.x + s->dbg_dx * scale, s->td.y,
                               &ex, &ey);
            draw_line(psx, psy, ex, ey, s->dbg_blocked_x ? red : green);
        }
        if (s->dbg_dy != 0.0f) {
            int ex, ey;
            td_world_to_screen(s, cam, s->td.x, s->td.y + s->dbg_dy * scale,
                               &ex, &ey);
            draw_line(psx, psy, ex, ey, s->dbg_blocked_y ? red : green);
        }
    }
}

typedef struct {
    int64_t  key;
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

static void render_topdown(GameState *s, const World *w)
{
    /* Static: ~7 KB — keeps it off the stack so the Pico's 4 KB main-thread
     * stack doesn't overflow. Renderer is not reentrant. */
    static TdCellSort cells[MAP_CELLS];
    TdCamBasis cam = td_cam_basis(s->td_cam_bearing);
    int        n = 0;

    hal_fill(C_SKY_BOT);

    for (int ty = 0; ty < w->h; ty++) {
        for (int tx = 0; tx < w->w; tx++) {
            if (!td_cell_on_screen(s, &cam, tx, ty)) continue;
            float wx = (float)((tx + 1) * TILE);
            float wy = (float)((ty + 1) * TILE);
            int   sx, sy;
            td_world_to_screen(s, &cam, wx, wy, &sx, &sy);
            /* Bias sy above zero before shifting; left-shifting a negative
             * signed int is undefined behaviour. Tiles in the cull zone can
             * have sy < 0, so we add an offset that safely covers the
             * possible range (DISPLAY_H + 2*TD_ISO_CULL). */
            int64_t sy_biased = (int64_t)sy + (DISPLAY_H + 2 * TD_ISO_CULL);
            cells[n].key = (sy_biased << 24) | ((int64_t)tx << 12) | (int64_t)ty;
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
        iso_draw_tile_full(world_tile(w, tx, ty), sx, sy, s->frame_count);
    }
    for (int i = 0; i < n; i++) {
        int   tx  = cells[i].tx;
        int   ty  = cells[i].ty;
        float tcx = (float)(tx * TILE + TILE / 2);
        float tcy = (float)(ty * TILE + TILE / 2);
        int   sx, sy;
        td_world_to_screen(s, &cam, tcx, tcy, &sx, &sy);
        iso_draw_tile_onlay(world_tile(w, tx, ty), sx, sy);
        if (w->node_respawn[ty * w->w + tx] > 0)
            iso_draw_depleted_mark(sx, sy);
    }

    int player_sx, player_sy;
    td_world_to_screen(s, &cam, s->td.x, s->td.y + TD_FEET_OFF,
                       &player_sx, &player_sy);
    iso_draw_td_player_char(player_sx, player_sy, s->td.screen_dir,
                            (uint8_t)s->td.walk_frame);

    if (s->skilling) {
        float progress = 1.0f - (float)s->action_ticks_left / (float)ACTION_TICKS;
        draw_skill_indicator(player_sx, player_sy - 30, progress);
    }

    if (s->debug_mode) {
        draw_debug_tile_hitboxes(s, w, &cam);
        draw_debug_hitbox(s, &cam);
        draw_debug_dir(s, &cam);
        draw_debug_collision(s, w, &cam);
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
