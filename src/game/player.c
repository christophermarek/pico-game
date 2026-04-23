#include "player.h"
#include "skills.h"
#include "td_cam.h"
#include "config.h"
#include <math.h>

/*
 * Unified obstacle collision: every non-walkable tile is a circle centred on
 * its tile centre, radius OBSTACLE_R. Collides when player-centre to tile-centre
 * distance < PL_HALF_W + OBSTACLE_R.
 */
static bool td_collides(const World *w, float x, float y) {
    int x0 = (int)(x - PL_HALF_W) / TILE;
    int x1 = (int)(x + PL_HALF_W - 1) / TILE;
    int y0 = (int)(y - PL_HALF_H) / TILE;
    int y1 = (int)(y + PL_HALF_H - 1) / TILE;
    float r = (float)(PL_HALF_W + OBSTACLE_R);
    for (int ty = y0; ty <= y1; ty++) {
        for (int tx = x0; tx <= x1; tx++) {
            if (tx < 0 || tx >= MAP_W || ty < 0 || ty >= MAP_H) continue;
            if (world_walkable(w, tx, ty)) continue;
            float tcx = (float)(tx * TILE + TILE / 2);
            float tcy = (float)(ty * TILE + TILE / 2);
            if (hypotf(x - tcx, y - tcy) < r) return true;
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

    if (!td_collides(w, nx, s->td.y)) s->td.x = nx;
    if (!td_collides(w, s->td.x, ny)) s->td.y = ny;

    if (s->td.x < PL_HALF_W)                s->td.x = (float)PL_HALF_W;
    if (s->td.x > MAP_W * TILE - PL_HALF_W) s->td.x = (float)(MAP_W * TILE - PL_HALF_W);
    if (s->td.y < PL_HALF_H)                s->td.y = (float)PL_HALF_H;
    if (s->td.y > MAP_H * TILE - PL_HALF_H) s->td.y = (float)(MAP_H * TILE - PL_HALF_H);

    if (dx != 0.0f || dy != 0.0f) {
        s->td.walk_frame += TD_WALK_ANIM_STEP;
        if (s->td.walk_frame >= WALK_FRAME_WRAP) s->td.walk_frame = 0.0f;
        s->total_steps++;
    } else {
        s->td.walk_frame = 0.0f;
    }

    s->td.tile_x = (int16_t)(s->td.x / TILE);
    s->td.tile_y = (int16_t)(s->td.y / TILE);

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

    if (tx < 0 || tx >= MAP_W || ty < 0 || ty >= MAP_H) return;

    uint8_t tile = world_tile(w, tx, ty);
    int     idx  = ty * MAP_W + tx;

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
    int x0 = (int)(x - PL_HALF_W) / TILE;
    int x1 = (int)(x + PL_HALF_W - 1) / TILE;
    int y0 = (int)(y - PL_HALF_H) / TILE;
    int y1 = (int)(y + PL_HALF_H - 1) / TILE;
    float r = (float)(PL_HALF_W + OBSTACLE_R);
    for (int ty = y0; ty <= y1; ty++) {
        for (int tx = x0; tx <= x1; tx++) {
            if (tx < 0 || tx >= MAP_W || ty < 0 || ty >= MAP_H) continue;
            if (world_walkable(w, tx, ty)) continue;
            float tcx = (float)(tx * TILE + TILE / 2);
            float tcy = (float)(ty * TILE + TILE / 2);
            if (hypotf(x - tcx, y - tcy) < r) {
                if (out_tx)   *out_tx   = tx;
                if (out_ty)   *out_ty   = ty;
                if (out_kind) *out_kind = (char)world_tile(w, tx, ty) + '0';
                return true;
            }
        }
    }
    return false;
}
