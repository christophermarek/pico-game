#include "render_debug.h"
#include "render_internal.h"
#include "font.h"
#include "hal.h"
#include "colors.h"
#include "config.h"
#include "../game/player.h"
#include <math.h>

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

/*
 * Polygon-approximate a world-space circle through the iso camera so the
 * result is the correct ellipse on screen for the current bearing. What you
 * see is exactly the collision footprint.
 */
static void draw_world_circle(const GameState *s, const TdCamBasis *cam,
                              float wcx, float wcy, float r, uint16_t color)
{
    enum { SEG = 24 };
    int px = 0, py = 0;
    for (int i = 0; i <= SEG; i++) {
        float t  = (float)i * (6.2831853f / (float)SEG);
        int   sx, sy;
        world_to_screen(s, cam, wcx + r * cosf(t), wcy + r * sinf(t), &sx, &sy);
        if (i > 0) draw_line(px, py, sx, sy, color);
        px = sx; py = sy;
    }
}

/* World-space AABB corners projected through the camera (→ parallelogram). */
static void draw_world_rect(const GameState *s, const TdCamBasis *cam,
                            float wx0, float wy0, float wx1, float wy1,
                            uint16_t color)
{
    int sx[4], sy[4];
    world_to_screen(s, cam, wx0, wy0, &sx[0], &sy[0]);
    world_to_screen(s, cam, wx1, wy0, &sx[1], &sy[1]);
    world_to_screen(s, cam, wx1, wy1, &sx[2], &sy[2]);
    world_to_screen(s, cam, wx0, wy1, &sx[3], &sy[3]);
    for (int i = 0; i < 4; i++) {
        int j = (i + 1) & 3;
        draw_line(sx[i], sy[i], sx[j], sy[j], color);
    }
}

static inline bool tile_is_circle(uint8_t tile_id) {
    return tile_id == T_TREE;
}

static void draw_obstacle_shape(const GameState *s, const TdCamBasis *cam,
                                uint8_t tile, float tcx, float tcy, uint16_t col)
{
    if (tile_is_circle(tile)) {
        draw_world_circle(s, cam, tcx, tcy, (float)OBSTACLE_R, col);
    } else {
        float h = (float)OBSTACLE_HALF;
        draw_world_rect(s, cam, tcx - h, tcy - h, tcx + h, tcy + h, col);
    }
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

static void draw_hitbox(const GameState *s, const TdCamBasis *cam)
{
    uint16_t red = HEX(0xef4444);
    float    cy  = s->td.y + TD_FEET_OFF;
    draw_world_circle(s, cam, s->td.x, cy, (float)PL_RADIUS, red);
    int psx, psy;
    world_to_screen(s, cam, s->td.x, cy, &psx, &psy);
    hal_pixel(psx, psy, red);
}

static void draw_tile_hitboxes(const GameState *s, const World *w,
                               const TdCamBasis *cam)
{
    uint16_t col = HEX(0xff8c00);
    for (int ty = 0; ty < MAP_H; ty++) {
        for (int tx = 0; tx < MAP_W; tx++) {
            if (world_walkable(w, tx, ty)) continue;
            if (!cell_on_screen(s, cam, tx, ty)) continue;
            float tcx = (float)(tx * TILE + TILE / 2);
            float tcy = (float)(ty * TILE + TILE / 2);
            draw_obstacle_shape(s, cam, world_tile(w, tx, ty), tcx, tcy, col);
            int tsx, tsy;
            world_to_screen(s, cam, tcx, tcy, &tsx, &tsy);
            draw_circle(tsx, tsy, 1, col);
        }
    }
}

static void draw_facing_arrow(const GameState *s, const TdCamBasis *cam)
{
    int psx, psy;
    world_to_screen(s, cam, s->td.x, s->td.y + TD_FEET_OFF, &psx, &psy);

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
    world_to_screen(s, cam, ftx, fty, &fsx, &fsy);
    draw_line(psx, psy, fsx, fsy, HEX(0xfbbf24));
}

static void draw_collision_panel(const GameState *s, const World *w,
                                 const TdCamBasis *cam)
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
    world_to_screen(s, cam, s->td.x, s->td.y + TD_FEET_OFF, &fsx, &fsy);
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
        world_to_screen(s, cam, s->td.x, s->td.y, &psx, &psy);
        world_to_screen(s, cam, best_sx, best_sy, &ssx, &ssy);
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
        draw_obstacle_shape(s, cam, tile, scx, scy, hi);
    }

    /* Velocity arrow per axis: green = applied, red = blocked. */
    int psx, psy;
    world_to_screen(s, cam, s->td.x, s->td.y, &psx, &psy);
    float scale = 12.0f;
    if (s->dbg_dx != 0.0f) {
        int ex, ey;
        world_to_screen(s, cam, s->td.x + s->dbg_dx * scale, s->td.y, &ex, &ey);
        draw_line(psx, psy, ex, ey, s->dbg_blocked_x ? red : green);
    }
    if (s->dbg_dy != 0.0f) {
        int ex, ey;
        world_to_screen(s, cam, s->td.x, s->td.y + s->dbg_dy * scale, &ex, &ey);
        draw_line(psx, psy, ex, ey, s->dbg_blocked_y ? red : green);
    }
}

/* Yellow diamond around the tile player_do_action would pick right now.
 * Gives an unambiguous visual answer to "what does A target?". */
static void draw_action_target(const GameState *s, const World *w,
                               const TdCamBasis *cam)
{
    int tx, ty;
    if (!player_peek_action_target(s, w, &tx, &ty)) return;
    float cx = (float)(tx * TILE + TILE / 2);
    float cy = (float)(ty * TILE + TILE / 2);
    float h  = (float)(TILE / 2);
    draw_world_rect(s, cam, cx - h, cy - h, cx + h, cy + h, HEX(0xfbbf24));
}

void render_debug_overworld(const GameState *s, const World *w,
                            const TdCamBasis *cam)
{
    draw_tile_hitboxes(s, w, cam);
    draw_hitbox(s, cam);
    draw_facing_arrow(s, cam);
    draw_action_target(s, w, cam);
    draw_collision_panel(s, w, cam);
}

void render_debug_fps(void)
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
