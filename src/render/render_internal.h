#pragma once
#include <stdbool.h>
#include "config.h"
#include "../game/state.h"
#include "../game/td_cam.h"

/* Internal helpers shared by renderer.c and render_debug.c. */

static inline void world_to_screen(const GameState *s, const TdCamBasis *cam,
                                   float wx, float wy, int *sx, int *sy)
{
    td_basis_world_pixel_to_screen(cam, s->td.x, s->td.y, wx, wy, sx, sy);
}

static inline bool cell_on_screen(const GameState *s, const TdCamBasis *cam,
                                  int tx, int ty)
{
    float tcx = (float)(tx * TILE + TILE / 2);
    float tcy = (float)(ty * TILE + TILE / 2);
    int   sx, sy;
    world_to_screen(s, cam, tcx, tcy, &sx, &sy);
    return sx >= -TD_ISO_CULL && sx < DISPLAY_W + TD_ISO_CULL &&
           sy >= -TD_ISO_CULL && sy < DISPLAY_H + TD_ISO_CULL;
}
