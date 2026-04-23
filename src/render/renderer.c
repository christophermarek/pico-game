#include "renderer.h"
#include "iso_spritesheet.h"
#include "render_debug.h"
#include "hal.h"
#include "colors.h"
#include "config.h"
#include "../ui/hud.h"
#include "../ui/menu.h"
#include "../game/td_cam.h"
#include <stdint.h>
#include <stdlib.h>

static void world_to_screen(const GameState *s, const TdCamBasis *cam,
                            float wx, float wy, int *sx, int *sy)
{
    td_basis_world_pixel_to_screen(cam, s->td.x, s->td.y, wx, wy, sx, sy);
}

static bool cell_on_screen(const GameState *s, const TdCamBasis *cam, int tx, int ty)
{
    float tcx = (float)(tx * TILE + TILE / 2);
    float tcy = (float)(ty * TILE + TILE / 2);
    int   sx, sy;
    world_to_screen(s, cam, tcx, tcy, &sx, &sy);
    return sx >= -TD_ISO_CULL && sx < DISPLAY_W + TD_ISO_CULL &&
           sy >= -TD_ISO_CULL && sy < DISPLAY_H + TD_ISO_CULL;
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
    for (int i = 0; i < n; i++) {
        int     tx   = cells[i].tx;
        int     ty   = cells[i].ty;
        uint8_t tile = world_tile(w, tx, ty);
        iso_draw_tile_onlay(tile, cells[i].sx, cells[i].sy);
        if (w->node_respawn[ty * w->w + tx] > 0)
            iso_draw_depleted_mark(cells[i].sx, cells[i].sy);
    }

    int player_sx, player_sy;
    world_to_screen(s, &cam, s->td.x, s->td.y + TD_FEET_OFF,
                    &player_sx, &player_sy);
    iso_draw_td_player_char(player_sx, player_sy, s->td.screen_dir,
                            (uint8_t)s->td.walk_frame);

    if (s->skilling) {
        float progress = 1.0f - (float)s->action_ticks_left / (float)ACTION_TICKS;
        draw_skill_indicator(player_sx, player_sy - 30, progress);
    }

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
