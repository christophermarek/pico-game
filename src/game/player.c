#include "player.h"
#include "skills.h"
#include "items.h"
#include "td_cam.h"
#include "config.h"
#include <math.h>

/*
 * Map (screen_dir, bearing) → world dir. Iso projection makes any screen-
 * cardinal movement produce |dwx| == |dwy|, so a naive adx/ady comparison
 * always ties. Route screen direction through the camera rotation to get
 * the true world axis instead.
 */
static const uint8_t DIR_MAP[4][4] = {
    /* screen\bearing  0           1           2           3    */
    /* DIR_DOWN  */ { DIR_DOWN,  DIR_LEFT,  DIR_UP,    DIR_RIGHT },
    /* DIR_UP    */ { DIR_UP,    DIR_RIGHT, DIR_DOWN,  DIR_LEFT  },
    /* DIR_LEFT  */ { DIR_LEFT,  DIR_DOWN,  DIR_RIGHT, DIR_UP    },
    /* DIR_RIGHT */ { DIR_RIGHT, DIR_UP,    DIR_LEFT,  DIR_DOWN  },
};

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

    /* Magnitude doesn't matter here — world velocity is normalised below. */
    if (vx != 0.0f || vy != 0.0f) {
        float ax = fabsf(vx), ay = fabsf(vy);
        if (ax >= ay) s->td.screen_dir = (vx < 0.0f) ? DIR_LEFT : DIR_RIGHT;
        else          s->td.screen_dir = (vy < 0.0f) ? DIR_UP   : DIR_DOWN;
        s->td.dir = DIR_MAP[s->td.screen_dir & 3u][s->td_cam_bearing & 3u];
    }

    TdCamBasis cam = td_cam_basis(s->td_cam_bearing);
    float      dwx, dwy;
    td_basis_screen_to_world_vel(&cam, vx, vy, &dwx, &dwy);

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

static bool is_actionable_tile(uint8_t t) {
    return t == T_TREE || t == T_ROCK || t == T_ORE ||
           t == T_WATER || t == T_TGRASS;
}

/*
 * Pick the nearest live actionable node in the 8 tiles surrounding the
 * player's feet. This matches visual intuition — the tree you're pressed
 * against gets chopped, regardless of what direction you last moved in.
 */
static bool tile_is_live_node(const World *w, int tx, int ty) {
    if (tx < 0 || tx >= w->w || ty < 0 || ty >= w->h) return false;
    if (!is_actionable_tile(world_tile(w, tx, ty)))   return false;
    if (w->node_respawn[ty * w->w + tx] > 0)          return false;
    return true;
}

/*
 * Strict front-of-player targeting. Two candidates, in order:
 *   1) the tile in the player's facing cardinal direction;
 *   2) the player's own tile_x/tile_y — collision slides the foot into a
 *      pressed-against obstacle's AABB while keeping it outside the
 *      obstacle's collision shape, so the obstacle ends up on the same
 *      tile as the foot.
 *
 * If neither is a live actionable node, we return false and do nothing.
 * This intentionally never reaches for tiles "behind" or "beside" the
 * player — the player has to face what they want to interact with.
 */
static bool find_action_target(const GameState *s, const World *w,
                               int *out_tx, int *out_ty)
{
    uint8_t world_dir = DIR_MAP[s->td.screen_dir & 3u][s->td_cam_bearing & 3u];
    int fdx = 0, fdy = 0;
    switch (world_dir) {
        case DIR_UP:    fdy = -1; break;
        case DIR_DOWN:  fdy =  1; break;
        case DIR_LEFT:  fdx = -1; break;
        case DIR_RIGHT: fdx =  1; break;
    }

    int fx = s->td.tile_x + fdx;
    int fy = s->td.tile_y + fdy;
    if (tile_is_live_node(w, fx, fy)) { *out_tx = fx; *out_ty = fy; return true; }

    int ox = s->td.tile_x;
    int oy = s->td.tile_y;
    if (tile_is_live_node(w, ox, oy)) { *out_tx = ox; *out_ty = oy; return true; }

    return false;
}

void player_do_action(GameState *s, World *w) {
    int tx, ty;
    if (!find_action_target(s, w, &tx, &ty)) {
        /* No live node adjacent — tell the player if they're next to a
         * depleted one via the old facing-tile check. */
        int ftx, fty;
        facing_tile(s, &ftx, &fty);
        if (ftx >= 0 && ftx < w->w && fty >= 0 && fty < w->h &&
            w->node_respawn[fty * w->w + ftx] > 0)
            state_log(s, "Node is depleted!");
        return;
    }

    uint8_t    skill_id;
    item_id_t  required_tool;
    switch (world_tile(w, tx, ty)) {
        case T_TREE:   skill_id = SK_WOODCUT; required_tool = ITEM_AXE;         break;
        case T_ROCK:   skill_id = SK_MINING;  required_tool = ITEM_PICKAXE;     break;
        case T_ORE:    skill_id = SK_MINING;  required_tool = ITEM_PICKAXE;     break;
        case T_WATER:  skill_id = SK_FISHING; required_tool = ITEM_FISHING_ROD; break;
        case T_TGRASS: skill_id = SK_WOODCUT; required_tool = ITEM_SHEARS;      break;
        default: return;
    }

    if (!inventory_has_tool(&s->inv, required_tool)) {
        char msg[36];
        const char *n = ITEM_DEFS[required_tool].name;
        int i = 0;
        msg[i++] = 'N'; msg[i++] = 'e'; msg[i++] = 'e'; msg[i++] = 'd'; msg[i++] = ' ';
        while (*n && i < 33) msg[i++] = *n++;
        msg[i++] = '!'; msg[i] = '\0';
        state_log(s, msg);
        return;
    }

    /* Log what node the action targets so the player knows before the wait. */
    switch (world_tile(w, tx, ty)) {
        case T_TREE:   state_log(s, "Chopping tree..."); break;
        case T_ROCK:   state_log(s, "Mining rock...");   break;
        case T_ORE:    state_log(s, "Mining ore...");    break;
        case T_WATER:  state_log(s, "Fishing...");       break;
        case T_TGRASS: state_log(s, "Shearing grass..."); break;
    }

    /* Tools live in slots TOOL_SLOT_START..+3 in AXE, PICKAXE, ROD, SHEARS
     * order — the same enum order items.h uses. */
    s->active_slot       = (uint8_t)(TOOL_SLOT_START + (required_tool - ITEM_AXE));
    s->skilling          = true;
    s->active_skill      = skill_id;
    s->action_node_x     = (int16_t)tx;
    s->action_node_y     = (int16_t)ty;
    s->action_ticks_left = ACTION_TICKS;
}

void player_stop_action(GameState *s) {
    s->skilling          = false;
    s->action_ticks_left = 0;
}

bool player_collide_who(const World *w, float x, float y,
                        int *out_tx, int *out_ty, char *out_kind) {
    return collide_at(w, x, y, out_tx, out_ty, out_kind);
}
