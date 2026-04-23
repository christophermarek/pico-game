#include "player.h"
#include "skills.h"
#include "td_cam.h"
#include "config.h"
#include <math.h>

/*
 * Unified obstacle collision.
 *
 * Shape per tile:
 *   T_TREE                 → circle radius OBSTACLE_R at tile centre
 *   T_ROCK/T_ORE/T_WATER   → AABB half-extent OBSTACLE_HALF at tile centre
 * The player is always a circle (PL_RADIUS) anchored at the FOOT
 * (td.y + TD_FEET_OFF, not td.y — otherwise we'd block ~7 world px before
 * the sprite touches the obstacle).
 *
 * Player-circle vs obstacle-square uses the classic closest-point trick:
 * clamp the player centre to the square, then check distance-squared
 * against PL_RADIUS². The broad-phase reach is MAX(OBSTACLE_R, OBSTACLE_HALF)
 * so we pick up candidates of either shape in one pass.
 */
#define OBSTACLE_REACH_MAX ((OBSTACLE_R > OBSTACLE_HALF) ? OBSTACLE_R : OBSTACLE_HALF)

static inline bool tile_is_circle_obstacle(uint8_t tile_id) {
    return tile_id == T_TREE;
}

static bool hit_obstacle(uint8_t tile_id, float px, float py,
                         float ocx, float ocy) {
    float ddx = px - ocx, ddy = py - ocy;
    if (tile_is_circle_obstacle(tile_id)) {
        float r = (float)(PL_RADIUS + OBSTACLE_R);
        return ddx * ddx + ddy * ddy < r * r;
    }
    /* Circle-vs-AABB: clamp the player centre to the square, test distance. */
    float cx = ddx;
    float cy = ddy;
    float h  = (float)OBSTACLE_HALF;
    if (cx >  h) cx =  h;
    if (cx < -h) cx = -h;
    if (cy >  h) cy =  h;
    if (cy < -h) cy = -h;
    float dx = ddx - cx, dy = ddy - cy;
    float pr = (float)PL_RADIUS;
    return dx * dx + dy * dy < pr * pr;
}

static bool td_collides(const World *w, float x, float y) {
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
            if (hit_obstacle(tile, x, y, ocx, ocy)) return true;
        }
    }
    return false;
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

    if (vx != 0.0f && vy != 0.0f) {
        vx *= 0.7071f;
        vy *= 0.7071f;
    }

    if (vx != 0.0f || vy != 0.0f) {
        float ax = fabsf(vx), ay = fabsf(vy);
        if (ax >= ay) s->td.screen_dir = (vx < 0.0f) ? DIR_LEFT : DIR_RIGHT;
        else          s->td.screen_dir = (vy < 0.0f) ? DIR_UP   : DIR_DOWN;
    }

    TdCamBasis cam = td_cam_basis(s->td_cam_bearing);
    float      dwx, dwy;
    td_basis_screen_to_world_vel(&cam, vx, vy, &dwx, &dwy);

    /* Constant SCREEN speed: scale the world-space velocity so the projected
     * screen motion is TD_SPEED pixels/frame in every direction. On a 2:1 iso
     * this makes left/right feel the same speed as up/down and diagonals
     * (world-space speed silently varies — acceptable, since perceived motion
     * is what the player judges). */
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
        if ((s->total_steps % TD_RUN_DRAIN_STEPS) == 0u) {
            if (s->energy > 0) s->energy--;
        }
        if (s->energy == 0) s->running_locked = true;
    } else if (s->running_locked && s->energy >= TD_RUN_ENERGY_RESUME) {
        s->running_locked = false;
    }

    float dx = dwx * wmul;
    float dy = dwy * wmul;

    /*
     * Derive world-space facing from screen_dir + camera bearing.
     *
     * In iso projection, any screen-cardinal movement produces exactly equal
     * |dwx| == |dwy| in world space, so the naive adx/ady comparison always
     * ties and the tiebreaker picks LEFT or RIGHT exclusively.  Instead, map
     * the screen-relative direction through the camera rotation to get the
     * true world-axis the player is heading along.
     *
     * Bearing rotation table (clockwise 90° steps):
     *   bearing 0: screen UDLR → world UDLR
     *   bearing 1: screen UDLR → world RDUL   (world rotated 90° CW)
     *   bearing 2: screen UDLR → world DURL
     *   bearing 3: screen UDLR → world LRUD
     */
    if (vx != 0.0f || vy != 0.0f) {
        /* Map (screen_dir, bearing) → world dir via a 4×4 table.
         * Rows = DIR_DOWN/UP/LEFT/RIGHT (0-3), cols = bearing (0-3). */
        static const uint8_t DIR_MAP[4][4] = {
            /* screen\bearing    0           1           2           3    */
            /* DIR_DOWN  */  { DIR_DOWN,  DIR_LEFT,  DIR_UP,    DIR_RIGHT },
            /* DIR_UP    */  { DIR_UP,    DIR_RIGHT, DIR_DOWN,  DIR_LEFT  },
            /* DIR_LEFT  */  { DIR_LEFT,  DIR_DOWN,  DIR_RIGHT, DIR_UP    },
            /* DIR_RIGHT */  { DIR_RIGHT, DIR_UP,    DIR_LEFT,  DIR_DOWN  },
        };
        uint8_t sd = s->td.screen_dir & 3u;
        uint8_t b  = s->td_cam_bearing & 3u;
        s->td.dir  = DIR_MAP[sd][b];
    }

    float nx = s->td.x + dx;
    float ny = s->td.y + dy;

    s->dbg_dx = dx;
    s->dbg_dy = dy;
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

    /*
     * Tangent-slide fallback: with circle obstacles, a player sitting tangent
     * to a tree and pushing *into* it has both axis-only moves land further
     * inside the circle — neither axis is "free" to slide on. Axis-separated
     * collision gets stuck. Resolve it by projecting the desired velocity
     * onto the tangent of the blocking circle and moving along that instead,
     * so the player glides around the trunk.
     */
    if (s->dbg_blocked_x && s->dbg_blocked_y && (dx != 0.0f || dy != 0.0f)) {
        int  wtx = 0, wty = 0;
        char wk  = '-';
        bool found = player_collide_who(w, nx, ny, &wtx, &wty, &wk)
                  || player_collide_who(w, nx, s->td.y, &wtx, &wty, &wk)
                  || player_collide_who(w, s->td.x, ny, &wtx, &wty, &wk);
        if (found) {
            float ocx = (float)(wtx * TILE + TILE / 2);
            float ocy = (float)(wty * TILE + TILE / 2);
            float ex  = s->td.x - ocx;
            float ey  = (s->td.y + TD_FEET_OFF) - ocy;
            float el  = hypotf(ex, ey);
            if (el > 1e-4f) {
                ex /= el; ey /= el;
                /* Two tangents; pick the one pointing with the input vel. */
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
        }
    }

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

    /* tile_x / tile_y reflect the FOOT cell (used by facing_tile, skill_complete_action). */
    s->td.tile_x = (int16_t)(s->td.x / TILE);
    s->td.tile_y = (int16_t)((s->td.y + TD_FEET_OFF) / TILE);

    if (inp->a_press)
        player_do_action(s, w);
}

static void facing_tile(const GameState *s, int *out_tx, int *out_ty) {
    int tx = s->td.tile_x;
    int ty = s->td.tile_y;
    switch (s->td.dir) {
        case DIR_UP:    ty--; break;
        case DIR_DOWN:  ty++; break;
        case DIR_LEFT:  tx--; break;
        case DIR_RIGHT: tx++; break;
    }
    *out_tx = tx;
    *out_ty = ty;
}

void player_do_action(GameState *s, World *w) {
    int tx, ty;
    facing_tile(s, &tx, &ty);

    if (tx < 0 || tx >= w->w || ty < 0 || ty >= w->h) return;

    uint8_t tile = world_tile(w, tx, ty);
    int     idx  = ty * w->w + tx;

    if (w->node_respawn[idx] > 0) {
        state_log(s, "Node is depleted!");
        return;
    }

    uint8_t skill_id = 0xFF;
    switch (tile) {
        case T_TREE:  skill_id = SK_WOODCUT; break;
        case T_ROCK:  skill_id = SK_MINING;  break;
        case T_ORE:   skill_id = SK_MINING;  break;
        case T_WATER: skill_id = SK_FISHING; break;
        default: break;
    }

    if (skill_id != 0xFF) {
        s->skilling          = true;
        s->active_skill      = skill_id;
        s->action_node_x     = (int16_t)tx;
        s->action_node_y     = (int16_t)ty;
        s->action_ticks_left = ACTION_TICKS;
    }
}

void player_stop_action(GameState *s) {
    s->skilling          = false;
    s->action_ticks_left = 0;
}

bool player_test_collide(const World *w, float x, float y) {
    return td_collides(w, x, y);
}

bool player_collide_who(const World *w, float x, float y,
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
                if (out_kind) *out_kind = (char)tile + '0';
                return true;
            }
        }
    }
    return false;
}
