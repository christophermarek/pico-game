#include "player.h"
#include "skills.h"
#include "items.h"
#include "actions.h"
#include "td_cam.h"
#include "config.h"
#include <math.h>

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

    /* screen_dir = the cardinal closest to the screen input (what the
     * sprite visually shows). td.dir = the cardinal closest to the
     * resulting world motion (informational; targeting does NOT use it —
     * it casts a ray from the foot in the screen-input direction, which
     * is naturally camera-correct). */
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
 * ─── Action targeting ─────────────────────────────────────────────────
 *
 * The engine picks a target via three stages, highest-priority first:
 *
 *   1. Collision probe. Nudge the player's collision circle by 1 pixel
 *      along each of the four world cardinals. If the nudge overlaps an
 *      actionable tile's collision shape, that tile is "pressed against"
 *      the player and gets priority — the tree you're physically up
 *      against gets chopped, regardless of where the sprite is facing.
 *
 *   2. Raycast. Convert the sprite's screen-facing direction into a
 *      world-space unit vector (via the inverse iso transform — this is
 *      naturally camera-correct, no lookup tables). Step along the ray
 *      from the foot and return the first actionable tile found, up to
 *      TARGET_REACH_PX. This is what you hit when you're near something
 *      but not touching it, and the same primitive generalises to
 *      combat (weapon reach) and interact-at-a-distance.
 *
 *   3. Own tile. If the player is standing on a walkable-but-actionable
 *      tile (tall grass), shear that tile on A-press.
 *
 * For the swing-and-collide combat model described in TODO.md, this
 * function grows into "pick target set for the current swing frame" and
 * returns a list of hits from the tool's bounding region — the raycast
 * here is the degenerate single-ray case.
 */
#define TARGET_REACH_PX  (TILE * 3 / 2)   /* ≤ 1.5 tiles forward reach  */
#define TARGET_RAY_STEP  2                /* ray sample stride (pixels) */

typedef enum {
    TARGET_NONE,
    TARGET_LIVE,
    TARGET_DEPLETED,
} TargetKind;

static TargetKind tile_kind(const World *w, int tx, int ty) {
    if (tx < 0 || tx >= w->w || ty < 0 || ty >= w->h) return TARGET_NONE;
    if (action_for_tile(world_tile(w, tx, ty)) == NULL) return TARGET_NONE;
    return (w->node_respawn[ty * w->w + tx] > 0) ? TARGET_DEPLETED
                                                 : TARGET_LIVE;
}

/* True if moving the player's foot by (ox, oy) pixels collides with an
 * obstacle; writes the blocker tile out if it does. */
static bool probe_blocker(const World *w, const GameState *s,
                          float ox, float oy, int *btx, int *bty)
{
    return player_collide_who(w, s->td.x + ox, s->td.y + oy, btx, bty, NULL);
}

static TargetKind find_action_target(const GameState *s, const World *w,
                                     int *out_tx, int *out_ty)
{
    /* (1) Collision probe — pressed-against tile wins. Prefer a live
     * target over a depleted one when multiple cardinals hit. */
    static const int PROBE_DIRS[4][2] = {{0,-1},{0,1},{-1,0},{1,0}};
    TargetKind blk_kind = TARGET_NONE;
    int blk_tx = -1, blk_ty = -1;
    for (int i = 0; i < 4; i++) {
        int btx, bty;
        float probe = (float)(PL_RADIUS + 1);
        if (!probe_blocker(w, s,
                           (float)PROBE_DIRS[i][0] * probe,
                           (float)PROBE_DIRS[i][1] * probe,
                           &btx, &bty)) continue;
        TargetKind k = tile_kind(w, btx, bty);
        if (k == TARGET_NONE) continue;
        if (blk_kind == TARGET_NONE ||
            (k == TARGET_LIVE && blk_kind == TARGET_DEPLETED)) {
            blk_kind = k; blk_tx = btx; blk_ty = bty;
        }
    }
    if (blk_kind != TARGET_NONE) {
        *out_tx = blk_tx; *out_ty = blk_ty;
        return blk_kind;
    }

    /* (2) Raycast from foot along the screen-facing direction, reinterpreted
     * as a world-space unit vector via the inverse iso transform. */
    float svx = 0.0f, svy = 0.0f;
    switch (s->td.screen_dir) {
        case DIR_DOWN:  svy =  1.0f; break;
        case DIR_UP:    svy = -1.0f; break;
        case DIR_LEFT:  svx = -1.0f; break;
        case DIR_RIGHT: svx =  1.0f; break;
    }
    TdCamBasis cam = td_cam_basis(s->td_cam_bearing);
    float dwx, dwy;
    td_basis_screen_to_world_vel(&cam, svx, svy, &dwx, &dwy);
    float len = hypotf(dwx, dwy);
    if (len >= 1e-4f) {
        dwx /= len; dwy /= len;
        float fx = s->td.x;
        float fy = s->td.y + (float)TD_FEET_OFF;
        int   prev_tx = -1, prev_ty = -1;
        for (int d = 0; d <= TARGET_REACH_PX; d += TARGET_RAY_STEP) {
            int tx = (int)((fx + dwx * (float)d) / (float)TILE);
            int ty = (int)((fy + dwy * (float)d) / (float)TILE);
            if (tx == prev_tx && ty == prev_ty) continue;
            prev_tx = tx; prev_ty = ty;
            if (tx == s->td.tile_x && ty == s->td.tile_y) continue;
            TargetKind k = tile_kind(w, tx, ty);
            if (k != TARGET_NONE) {
                *out_tx = tx; *out_ty = ty;
                return k;
            }
        }
    }

    /* (3) Own tile — walkable-actionable (tall grass underfoot). */
    TargetKind self = tile_kind(w, s->td.tile_x, s->td.tile_y);
    if (self != TARGET_NONE) {
        *out_tx = s->td.tile_x; *out_ty = s->td.tile_y;
        return self;
    }

    return TARGET_NONE;
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
