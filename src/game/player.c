#include "player.h"
#include "skills.h"
#include "items.h"
#include "actions.h"
#include "td_cam.h"
#include "config.h"
#include <math.h>
#include <stdlib.h>

#define OBSTACLE_REACH_MAX ((OBSTACLE_R > OBSTACLE_HALF) ? OBSTACLE_R : OBSTACLE_HALF)

static inline bool tile_is_circle(uint8_t tile_id) {
    return tile_id == T_TREE;
}

/*
 * Player is a circle of radius PL_RADIUS. Obstacle is either a circle
 * (trees) or an axis-aligned square (rocks/ore/water). Circle-vs-square
 * uses the classic clamp-to-square trick, then distance² < PL_RADIUS².
 */
static bool hit_obstacle(uint8_t tile_id, float px, float py,
                         float ocx, float ocy) {
    float ddx = px - ocx, ddy = py - ocy;
    if (tile_is_circle(tile_id)) {
        float r = (float)(PL_RADIUS + OBSTACLE_R);
        return ddx * ddx + ddy * ddy < r * r;
    }
    float cx = ddx, cy = ddy;
    float h  = (float)OBSTACLE_HALF;
    if (cx >  h) cx =  h;
    if (cx < -h) cx = -h;
    if (cy >  h) cy =  h;
    if (cy < -h) cy = -h;
    float dx = ddx - cx, dy = ddy - cy;
    float pr = (float)PL_RADIUS;
    return dx * dx + dy * dy < pr * pr;
}

/*
 * Shared broad-phase: iterate candidate tiles near the FOOT (td.y+TD_FEET_OFF,
 * not td.y — otherwise blocking triggers ~7 world px before the sprite
 * touches). On the first hit, optionally write out the blocker's identity.
 */
static bool collide_at(const World *w, float x, float y,
                       int *out_tx, int *out_ty, char *out_kind) {
    y += (float)TD_FEET_OFF;
    float r = (float)(PL_RADIUS + OBSTACLE_REACH_MAX);
    int tx0 = (int)floorf((x - r) / (float)TILE);
    int tx1 = (int)floorf((x + r) / (float)TILE);
    int ty0 = (int)floorf((y - r) / (float)TILE);
    int ty1 = (int)floorf((y + r) / (float)TILE);
    for (int ty = ty0; ty <= ty1; ty++) {
        for (int tx = tx0; tx <= tx1; tx++) {
            if (tx < 0 || tx >= MAP_W || ty < 0 || ty >= MAP_H) continue;
            if (world_walkable(w, tx, ty)) continue;
            uint8_t tile = world_tile(w, tx, ty);
            float ocx = (float)(tx * TILE + TILE / 2);
            float ocy = (float)(ty * TILE + TILE / 2);
            if (hit_obstacle(tile, x, y, ocx, ocy)) {
                if (out_tx)   *out_tx   = tx;
                if (out_ty)   *out_ty   = ty;
                if (out_kind)
                    *out_kind = (tile < 10) ? (char)('0' + tile)
                                            : (char)('A' + tile - 10);
                return true;
            }
        }
    }
    return false;
}

static inline bool td_collides(const World *w, float x, float y) {
    return collide_at(w, x, y, NULL, NULL, NULL);
}


/*
 * Tangent-slide: against a circle obstacle, a player sitting tangent and
 * pushing *into* it has both axis-only moves land further inside — axis-
 * separated collision gets stuck. Project the desired velocity onto the
 * blocking circle's tangent and try that.
 */
static void tangent_slide(GameState *s, const World *w, float dx, float dy,
                          float nx, float ny) {
    int  wtx = 0, wty = 0;
    char wk  = '-';
    bool found = player_collide_who(w, nx, ny,         &wtx, &wty, &wk)
              || player_collide_who(w, nx, s->td.y,    &wtx, &wty, &wk)
              || player_collide_who(w, s->td.x, ny,    &wtx, &wty, &wk);
    if (!found) return;

    float ocx = (float)(wtx * TILE + TILE / 2);
    float ocy = (float)(wty * TILE + TILE / 2);
    float ex  = s->td.x - ocx;
    float ey  = (s->td.y + TD_FEET_OFF) - ocy;
    float el  = hypotf(ex, ey);
    if (el <= 1e-4f) return;

    ex /= el;
    ey /= el;
    /* Two tangents — pick the one pointing with the input velocity. */
    float t1x = -ey, t1y = ex;
    float dot = t1x * dx + t1y * dy;
    float tx  = (dot >= 0.0f) ? t1x : -t1x;
    float ty  = (dot >= 0.0f) ? t1y : -t1y;
    float spd = hypotf(dx, dy);
    float sdx = tx * spd;
    float sdy = ty * spd;

    float snx = s->td.x + sdx;
    float sny = s->td.y + sdy;
    if (!td_collides(w, snx, s->td.y)) {
        s->td.x = snx;
        s->dbg_blocked_x = false;
    }
    if (!td_collides(w, s->td.x, sny)) {
        s->td.y = sny;
        s->dbg_blocked_y = false;
    }
}

void player_update_td(GameState *s, const Input *inp, World *w) {
    if (s->skilling) {
        if (inp->b_press) {
            player_stop_action(s);
            return;
        }
        if (s->action_ticks_left > 0) {
            s->action_ticks_left--;
            if (s->action_ticks_left == 0)
                skill_complete_action(s, w);
        }
        return;
    }

    float vx = 0.0f, vy = 0.0f;
    if (inp->left)  vx -= 1.0f;
    if (inp->right) vx += 1.0f;
    if (inp->up)    vy -= 1.0f;
    if (inp->down)  vy += 1.0f;

    TdCamBasis cam = td_cam_basis(s->td_cam_bearing);
    float      dwx, dwy;
    td_basis_screen_to_world_vel(&cam, vx, vy, &dwx, &dwy);

    /* Derive the two facing cardinals straight from the raw velocities. No
     * (screen_dir × bearing) lookup table — screen_dir is the closest
     * cardinal to the SCREEN input, td.dir is the closest cardinal to the
     * resulting WORLD movement. Iso cardinals make |dwx|==|dwy|, so at
     * ties we bias toward the vertical axis (matches the bearing=0
     * convention: DOWN-press → world south). */
    if (vx != 0.0f || vy != 0.0f) {
        float ax = fabsf(vx), ay = fabsf(vy);
        if (ax > ay) s->td.screen_dir = (vx < 0.0f) ? DIR_LEFT : DIR_RIGHT;
        else         s->td.screen_dir = (vy < 0.0f) ? DIR_UP   : DIR_DOWN;

        float adx = fabsf(dwx), ady = fabsf(dwy);
        if (adx > ady) s->td.dir = (dwx < 0.0f) ? DIR_LEFT : DIR_RIGHT;
        else           s->td.dir = (dwy < 0.0f) ? DIR_UP   : DIR_DOWN;
    }

    /*
     * Constant SCREEN speed: scale the world-space velocity so the projected
     * screen motion is TD_SPEED px/frame in every direction. On a 2:1 iso
     * this makes left/right feel the same speed as up/down (world speed
     * silently varies — perceived motion is what the player judges).
     */
    float len = hypotf(dwx, dwy);
    if (len > 1e-5f) { dwx /= len; dwy /= len; }
    else             { dwx = 0.0f; dwy = 0.0f; }

    float srx, sry;
    td_basis_world_delta(&cam, dwx, dwy, &srx, &sry);
    float smag = hypotf(srx, sry);
    float wmul = (smag > 1e-5f) ? (TD_SPEED / smag) : 0.0f;

    bool moving  = (dwx != 0.0f || dwy != 0.0f);
    bool running = inp->b && moving && s->energy > 0 && !s->running_locked;
    if (running) {
        wmul *= TD_RUN_SPEED_MULT;
        if ((s->total_steps % TD_RUN_DRAIN_STEPS) == 0u && s->energy > 0)
            s->energy--;
        if (s->energy == 0) s->running_locked = true;
    } else if (s->running_locked && s->energy >= TD_RUN_ENERGY_RESUME) {
        s->running_locked = false;
    }

    float dx = dwx * wmul;
    float dy = dwy * wmul;
    float nx = s->td.x + dx;
    float ny = s->td.y + dy;

    s->dbg_dx        = dx;
    s->dbg_dy        = dy;
    s->dbg_blocked_x = false;
    s->dbg_blocked_y = false;

    if (dx != 0.0f) {
        if (!td_collides(w, nx, s->td.y)) s->td.x = nx;
        else                              s->dbg_blocked_x = true;
    }
    if (dy != 0.0f) {
        if (!td_collides(w, s->td.x, ny)) s->td.y = ny;
        else                              s->dbg_blocked_y = true;
    }

    if (s->dbg_blocked_x && s->dbg_blocked_y && (dx != 0.0f || dy != 0.0f))
        tangent_slide(s, w, dx, dy, nx, ny);

    /* Clamp the FOOT (td.y + TD_FEET_OFF) inside the world AABB. */
    if (s->td.x < PL_HALF_W)
        s->td.x = (float)PL_HALF_W;
    if (s->td.x > w->w * TILE - PL_HALF_W)
        s->td.x = (float)(w->w * TILE - PL_HALF_W);
    if (s->td.y + TD_FEET_OFF < PL_HALF_H)
        s->td.y = (float)PL_HALF_H - TD_FEET_OFF;
    if (s->td.y + TD_FEET_OFF > w->h * TILE - PL_HALF_H)
        s->td.y = (float)(w->h * TILE - PL_HALF_H) - TD_FEET_OFF;

    if (dx != 0.0f || dy != 0.0f) {
        s->td.walk_frame += TD_WALK_ANIM_STEP;
        if (s->td.walk_frame >= WALK_FRAME_WRAP) s->td.walk_frame = 0.0f;
        s->total_steps++;
    } else {
        s->td.walk_frame = 0.0f;
    }

    /* tile_x/y reflect the FOOT cell — used by facing_tile and skill actions. */
    s->td.tile_x = (int16_t)(s->td.x / TILE);
    s->td.tile_y = (int16_t)((s->td.y + TD_FEET_OFF) / TILE);

    if (inp->a_press)
        player_do_action(s, w);
}

/*
 * Camera-aware targeting: score every candidate tile by how "in front" it
 * is in SCREEN space.  This works identically at all four bearings because
 * tile screen positions come from the cam-aware projection — no lookup
 * table with per-bearing world-direction magic.
 *
 * Candidate set: the 3×3 block around the foot tile (including the foot's
 * own tile — collision can leave the foot straddled into a non-walkable
 * tile's AABB while staying outside its collision shape).
 *
 * Score = dot(tile_offset_on_screen, screen-facing unit vector). Positive
 * means "in front of" the player's on-screen facing. Tiles behind are
 * rejected outright; tied scores break on smallest perpendicular (most
 * centred on the facing axis).
 *
 * If no live node scores positive but a depleted one does, we log "Node
 * is depleted!" so the player knows they're pointing at something.
 */
typedef enum {
    TARGET_NONE,
    TARGET_LIVE,
    TARGET_DEPLETED,
} TargetKind;

static TargetKind find_action_target(const GameState *s, const World *w,
                                     int *out_tx, int *out_ty)
{
    TdCamBasis cam = td_cam_basis(s->td_cam_bearing);

    int psx, psy;
    td_basis_world_pixel_to_screen(&cam, s->td.x, s->td.y,
                                   s->td.x, s->td.y + (float)TD_FEET_OFF,
                                   &psx, &psy);

    int fdx = 0, fdy = 0;
    switch (s->td.screen_dir) {
        case DIR_DOWN:  fdy =  1; break;
        case DIR_UP:    fdy = -1; break;
        case DIR_LEFT:  fdx = -1; break;
        case DIR_RIGHT: fdx =  1; break;
    }

    int best_score = 0, best_perp = 0;
    int best_tx = -1, best_ty = -1;
    TargetKind best_kind = TARGET_NONE;

    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            int tx = s->td.tile_x + dx;
            int ty = s->td.tile_y + dy;
            if (tx < 0 || tx >= w->w || ty < 0 || ty >= w->h) continue;
            if (action_for_tile(world_tile(w, tx, ty)) == NULL) continue;

            int tsx, tsy;
            td_basis_world_pixel_to_screen(&cam, s->td.x, s->td.y,
                                           (float)(tx * TILE + TILE / 2),
                                           (float)(ty * TILE + TILE / 2),
                                           &tsx, &tsy);
            int dsx = tsx - psx, dsy = tsy - psy;
            int score = dsx * fdx + dsy * fdy;
            if (score <= 0) continue;           /* behind the player */
            int perp = abs(dsx * fdy - dsy * fdx);

            TargetKind kind = (w->node_respawn[ty * w->w + tx] > 0)
                              ? TARGET_DEPLETED : TARGET_LIVE;

            /* Live always wins over depleted. Within same kind, higher
             * score wins; ties break on lower perp. */
            bool better = false;
            if (best_kind == TARGET_NONE)                 better = true;
            else if (best_kind == TARGET_DEPLETED && kind == TARGET_LIVE)
                better = true;
            else if (best_kind == kind) {
                if (score > best_score) better = true;
                else if (score == best_score && perp < best_perp) better = true;
            }

            if (better) {
                best_score = score; best_perp = perp;
                best_tx = tx; best_ty = ty;
                best_kind = kind;
            }
        }
    }

    if (best_kind == TARGET_NONE) return TARGET_NONE;
    *out_tx = best_tx;
    *out_ty = best_ty;
    return best_kind;
}

/* Slot index where a given tool icon lives (AXE→4, PICKAXE→5, etc.). */
static uint8_t tool_slot(item_id_t tool) {
    return (uint8_t)(TOOL_SLOT_START + (tool - ITEM_AXE));
}

void player_do_action(GameState *s, World *w) {
    int tx, ty;
    TargetKind kind = find_action_target(s, w, &tx, &ty);
    if (kind == TARGET_NONE) return;
    if (kind == TARGET_DEPLETED) { state_log(s, "Node is depleted!"); return; }

    const NodeAction *a = action_for_tile(world_tile(w, tx, ty));
    if (!a) return; /* shouldn't happen — find_action_target filters */

    if (!inventory_has_tool(&s->inv, a->tool)) {
        char msg[36];
        const char *n = ITEM_DEFS[a->tool].name;
        int i = 0;
        msg[i++] = 'N'; msg[i++] = 'e'; msg[i++] = 'e'; msg[i++] = 'd'; msg[i++] = ' ';
        while (*n && i < 33) msg[i++] = *n++;
        msg[i++] = '!'; msg[i] = '\0';
        state_log(s, msg);
        return;
    }

    state_log(s, a->msg_start);

    s->active_slot       = tool_slot(a->tool);
    s->skilling          = true;
    s->active_skill      = a->skill;
    s->action_node_x     = (int16_t)tx;
    s->action_node_y     = (int16_t)ty;
    s->action_ticks_left = ACTION_TICKS;
}

bool player_peek_action_target(const GameState *s, const World *w,
                               int *out_tx, int *out_ty)
{
    return find_action_target(s, w, out_tx, out_ty) == TARGET_LIVE;
}

void player_stop_action(GameState *s) {
    s->skilling          = false;
    s->action_ticks_left = 0;
}

bool player_collide_who(const World *w, float x, float y,
                        int *out_tx, int *out_ty, char *out_kind) {
    return collide_at(w, x, y, out_tx, out_ty, out_kind);
}
