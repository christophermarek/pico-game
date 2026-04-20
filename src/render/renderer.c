#include "renderer.h"
#include "sprites.h"
#include "font.h"
#include "hal.h"
#include "colors.h"
#include "config.h"
#include "../ui/hud.h"
#include "../ui/menu.h"
#include "../game/td_cam.h"
#include <stdint.h>
#include <stdlib.h>

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
