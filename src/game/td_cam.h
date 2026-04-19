#pragma once
/*
 * Top-down overworld: 4-bearing isometric (2:1 diamond) camera.
 *
 * Pipeline (correct order, keeps the tile grid consistent at every bearing):
 *     world delta  →  90°-step world rotation (bearing)  →  iso skew  →  screen
 *
 * Because rotation happens BEFORE the iso skew, tiles always render as
 * axis-aligned 2:1 horizontal diamonds on screen. Only the *content* of the
 * world rotates with the camera. This is what the sprite tiles, the mansion
 * quad, and the player's screen-relative input all assume.
 *
 * Input: screen velocity → inverse iso → inverse world rotation → world delta.
 */
#include <math.h>
#include <stdint.h>

#include "config.h"

#define TD_CAM_STEPS 4

/* One world-tile step maps to this iso spacing (2:1 diamond). */
#define TD_ISO_HW ((float)TILE)
#define TD_ISO_HH ((float)TILE * 0.5f)
#define TD_ISO_A  (TD_ISO_HW / (float)TILE)   /* = 1.0 */
#define TD_ISO_B  (TD_ISO_HH / (float)TILE)   /* = 0.5 */

typedef struct {
    uint8_t bearing; /* 0..3, clockwise 90° steps on the world grid */
} TdCamBasis;

static inline TdCamBasis td_cam_basis(uint8_t bearing)
{
    TdCamBasis k;
    k.bearing = (uint8_t)(bearing & (TD_CAM_STEPS - 1));
    return k;
}

/* Rotate a world delta by bearing*90° (clockwise on the world grid). */
static inline void td_world_rotate(uint8_t bearing, float dwx, float dwy,
                                   float *rx, float *ry)
{
    switch (bearing & 3u) {
    case 0: *rx =  dwx; *ry =  dwy; break;
    case 1: *rx = -dwy; *ry =  dwx; break;
    case 2: *rx = -dwx; *ry = -dwy; break;
    case 3: *rx =  dwy; *ry = -dwx; break;
    }
}

/* Inverse of td_world_rotate: camera-oriented delta → raw world delta. */
static inline void td_world_unrotate(uint8_t bearing, float rx, float ry,
                                     float *dwx, float *dwy)
{
    switch (bearing & 3u) {
    case 0: *dwx =  rx; *dwy =  ry; break;
    case 1: *dwx =  ry; *dwy = -rx; break;
    case 2: *dwx = -rx; *dwy = -ry; break;
    case 3: *dwx = -ry; *dwy =  rx; break;
    }
}

/* World-axis delta → iso (2:1 diamond) screen offset. */
static inline void td_world_delta_to_iso(float dwx, float dwy,
                                         float *ix, float *iy)
{
    *ix = TD_ISO_A * (dwx - dwy);
    *iy = TD_ISO_B * (dwx + dwy);
}

/* Inverse of td_world_delta_to_iso. */
static inline void td_iso_delta_to_world(float ix, float iy,
                                         float *dwx, float *dwy)
{
    *dwx = 0.5f * (ix / TD_ISO_A + iy / TD_ISO_B);
    *dwy = 0.5f * (iy / TD_ISO_B - ix / TD_ISO_A);
}

/* World offset (from player) → screen offset before centring. */
static inline void td_basis_world_delta(const TdCamBasis *k, float ddx, float ddy,
                                        float *rx, float *ry)
{
    float rwx, rwy;
    td_world_rotate(k->bearing, ddx, ddy, &rwx, &rwy);
    td_world_delta_to_iso(rwx, rwy, rx, ry);
}

/* World pixel → screen pixel (camera centred on the player). */
static inline void td_basis_world_pixel_to_screen(const TdCamBasis *k, float px,
                                                  float py, float wx, float wy,
                                                  int *sx, int *sy)
{
    float rx, ry;
    td_basis_world_delta(k, wx - px, wy - py, &rx, &ry);
    *sx = (int)lrintf(rx) + DISPLAY_W / 2;
    *sy = (int)lrintf(ry) + DISPLAY_H / 2;
}

/* Screen-relative walk (y+ = down) → world walk; normalize in caller. */
static inline void td_basis_screen_to_world_vel(const TdCamBasis *k, float svx,
                                                float svy, float *dwx, float *dwy)
{
    float rwx, rwy;
    td_iso_delta_to_world(svx, svy, &rwx, &rwy);
    td_world_unrotate(k->bearing, rwx, rwy, dwx, dwy);
}
