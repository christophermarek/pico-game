#pragma once
/*
 * Top-down overworld: 4-bearing isometric (2:1 diamond) camera.
 *
 * Pipeline order matters — rotate the world delta BEFORE applying the iso
 * skew, otherwise the tile grid no longer renders as axis-aligned diamonds
 * as the camera rotates:
 *
 *     world delta  →  90°-step world rotation  →  iso skew  →  screen
 *
 * Input goes through the inverse chain:
 *     screen velocity  →  inverse iso  →  inverse world rotation  →  world delta
 */
#include <math.h>
#include <stdint.h>
#include "config.h"

#define TD_CAM_STEPS 4

/*
 * 64×32 flat diamond on a 64×48 sprite:
 *   1 tile in world X → screen (+32, +16)  =>  A = 2.0, B = 1.0.
 */
#define TD_ISO_HW ((float)TILE * 2.0f)
#define TD_ISO_HH ((float)TILE * 1.0f)
#define TD_ISO_A  (TD_ISO_HW / (float)TILE)
#define TD_ISO_B  (TD_ISO_HH / (float)TILE)

typedef struct {
    uint8_t bearing;  /* 0..3, 90° clockwise steps */
} TdCamBasis;

static inline TdCamBasis td_cam_basis(uint8_t bearing) {
    TdCamBasis k;
    k.bearing = (uint8_t)(bearing & (TD_CAM_STEPS - 1));
    return k;
}

static inline void td_world_rotate(uint8_t bearing, float dwx, float dwy,
                                   float *rx, float *ry) {
    switch (bearing & 3u) {
    case 0: *rx =  dwx; *ry =  dwy; break;
    case 1: *rx = -dwy; *ry =  dwx; break;
    case 2: *rx = -dwx; *ry = -dwy; break;
    case 3: *rx =  dwy; *ry = -dwx; break;
    }
}

static inline void td_world_unrotate(uint8_t bearing, float rx, float ry,
                                     float *dwx, float *dwy) {
    switch (bearing & 3u) {
    case 0: *dwx =  rx; *dwy =  ry; break;
    case 1: *dwx =  ry; *dwy = -rx; break;
    case 2: *dwx = -rx; *dwy = -ry; break;
    case 3: *dwx = -ry; *dwy =  rx; break;
    }
}

static inline void td_world_delta_to_iso(float dwx, float dwy, float *ix, float *iy) {
    *ix = TD_ISO_A * (dwx - dwy);
    *iy = TD_ISO_B * (dwx + dwy);
}

static inline void td_iso_delta_to_world(float ix, float iy, float *dwx, float *dwy) {
    *dwx = 0.5f * (ix / TD_ISO_A + iy / TD_ISO_B);
    *dwy = 0.5f * (iy / TD_ISO_B - ix / TD_ISO_A);
}

static inline void td_basis_world_delta(const TdCamBasis *k, float ddx, float ddy,
                                        float *rx, float *ry) {
    float rwx, rwy;
    td_world_rotate(k->bearing, ddx, ddy, &rwx, &rwy);
    td_world_delta_to_iso(rwx, rwy, rx, ry);
}

/* World pixel → screen pixel (camera centred on the player at px,py). */
static inline void td_basis_world_pixel_to_screen(const TdCamBasis *k, float px,
                                                  float py, float wx, float wy,
                                                  int *sx, int *sy) {
    float rx, ry;
    td_basis_world_delta(k, wx - px, wy - py, &rx, &ry);
    *sx = (int)lrintf(rx) + DISPLAY_W / 2;
    *sy = (int)lrintf(ry) + DISPLAY_H / 2;
}

/* Screen-relative walk input → world walk. Caller normalises length. */
static inline void td_basis_screen_to_world_vel(const TdCamBasis *k, float svx,
                                                float svy, float *dwx, float *dwy) {
    float rwx, rwy;
    td_iso_delta_to_world(svx, svy, &rwx, &rwy);
    td_world_unrotate(k->bearing, rwx, rwy, dwx, dwy);
}
