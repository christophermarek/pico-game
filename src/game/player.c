#include "player.h"
#include "skills.h"
#include "td_cam.h"
#include "config.h"
#include <math.h>

/* ------------------------------------------------------------------ */
/* AABB tile collision helpers                                          */
/* ------------------------------------------------------------------ */
static bool td_collides(const World *w, float x, float y) {
    int x0 = (int)(x - PL_HALF_W) / TILE;
    int x1 = (int)(x + PL_HALF_W - 1) / TILE;
    int y0 = (int)(y - PL_HALF_H) / TILE;
    int y1 = (int)(y + PL_HALF_H - 1) / TILE;
    for (int ty = y0; ty <= y1; ty++)
        for (int tx = x0; tx <= x1; tx++)
            if (!world_walkable(w, tx, ty)) return true;
    return false;
}

/* ------------------------------------------------------------------ */
/* Top-down update                                                      */
/* ------------------------------------------------------------------ */
void player_update_td(GameState *s, const Input *inp, World *w) {
    /* If skilling, tick action */
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
        /* Pick dominant axis so diagonals don't always collapse to L/R */
        float ax = fabsf(vx), ay = fabsf(vy);
        if (ax >= ay) s->td.screen_dir = (vx < 0.0f) ? DIR_LEFT : DIR_RIGHT;
        else          s->td.screen_dir = (vy < 0.0f) ? DIR_UP   : DIR_DOWN;
    }

    TdCamBasis cam = td_cam_basis(s->td_cam_bearing);
    float      dwx, dwy;
    td_basis_screen_to_world_vel(&cam, vx, vy, &dwx, &dwy);
    float len = hypotf(dwx, dwy);
    if (len > 1e-5f) {
        dwx /= len;
        dwy /= len;
    } else {
        dwx = 0.0f;
        dwy = 0.0f;
    }

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

    float dx   = dwx * wmul;
    float dy   = dwy * wmul;

    {
        float adx = fabsf(dwx), ady = fabsf(dwy);
        if (adx > 0.01f || ady > 0.01f) {
            if (adx >= ady) s->td.dir = (dwx < 0.0f) ? DIR_LEFT : DIR_RIGHT;
            else            s->td.dir = (dwy < 0.0f) ? DIR_UP   : DIR_DOWN;
        }
    }

    /* Move with collision */
    float nx = s->td.x + dx;
    float ny = s->td.y + dy;

    if (!td_collides(w, nx, s->td.y)) s->td.x = nx;
    if (!td_collides(w, s->td.x, ny)) s->td.y = ny;

    /* Clamp to map bounds */
    if (s->td.x < PL_HALF_W)             s->td.x = (float)PL_HALF_W;
    if (s->td.x > MAP_W * TILE - PL_HALF_W) s->td.x = (float)(MAP_W * TILE - PL_HALF_W);
    if (s->td.y < PL_HALF_H)             s->td.y = (float)PL_HALF_H;
    if (s->td.y > MAP_H * TILE - PL_HALF_H) s->td.y = (float)(MAP_H * TILE - PL_HALF_H);

    /* Walk frame */
    if (dx != 0.0f || dy != 0.0f) {
        s->td.walk_frame += TD_WALK_ANIM_STEP;
        if (s->td.walk_frame >= WALK_FRAME_WRAP) s->td.walk_frame = 0.0f;
        s->total_steps++;
    } else {
        s->td.walk_frame = 0.0f;
    }

    /* Update tile coords */
    s->td.tile_x = (int16_t)(s->td.x / TILE);
    s->td.tile_y = (int16_t)(s->td.y / TILE);

    /* Pet trailing */
    if (s->party_count > 0) {
        Pet *pet = &s->party[s->active_pet];
        float pdx = s->td.x - pet->trail_x;
        float pdy = s->td.y - pet->trail_y;
        float dist2 = pdx * pdx + pdy * pdy;
        float follow_dist = (float)TILE;
        if (dist2 > follow_dist * follow_dist) {
            float d = sqrtf(dist2);
            float spd = TD_SPEED * TD_PET_FOLLOW_MULT;
            pet->trail_x += (pdx / d) * spd;
            pet->trail_y += (pdy / d) * spd;
        }
    }

    /* Check HOME tile (on door) or porch directly south, facing the door */
    uint8_t cur_tile = world_tile(w, s->td.tile_x, s->td.tile_y);
    bool porch_enter = (inp->a_press && cur_tile == T_PATH &&
                          s->td.tile_x == MANSION_DOOR_TX &&
                          s->td.tile_y == MANSION_DOOR_TY + 1 && s->td.dir == DIR_UP);
    if (inp->a_press && (cur_tile == T_HOME || porch_enter)) {
        s->prev_mode = s->mode;
        s->mode = MODE_BASE;
        return;
    }

    /* A button action */
    if (inp->a_press) {
        player_do_action(s, w);
    }

}

/* ------------------------------------------------------------------ */
/* Find the tile the player is facing                                   */
/* ------------------------------------------------------------------ */
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

/* ------------------------------------------------------------------ */
/* A-button action dispatch                                             */
/* ------------------------------------------------------------------ */
void player_do_action(GameState *s, World *w) {
    int tx, ty;
    facing_tile(s, &tx, &ty);

    if (tx < 0 || tx >= MAP_W || ty < 0 || ty >= MAP_H) return;

    uint8_t tile = world_tile(w, tx, ty);
    int     idx  = ty * MAP_W + tx;

    /* Node depleted? */
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

/* ------------------------------------------------------------------ */
/* Cancel skilling                                                      */
/* ------------------------------------------------------------------ */
void player_stop_action(GameState *s) {
    s->skilling          = false;
    s->action_ticks_left = 0;
}

/* ------------------------------------------------------------------ */
/* Side-view update                                                     */
/* ------------------------------------------------------------------ */
void player_update_sv(GameState *s, const Input *inp, World *w) {
    PlayerSV *p = &s->sv;

    float dx = 0.0f;
    if (inp->left)  { dx -= SV_SPEED; p->dir = DIR_LEFT;  }
    if (inp->right) { dx += SV_SPEED; p->dir = DIR_RIGHT; }

    /* Jump */
    if (inp->a_press && p->on_ground) {
        p->vy = SV_JUMP;
        p->on_ground = false;
    }

    /* Gravity */
    p->vy += SV_GRAVITY;
    if (p->vy > SV_MAX_FALL) p->vy = SV_MAX_FALL;

    /* Horizontal movement + collision */
    float nx = p->x + dx;
    int tx0  = (int)(nx / TILE);
    int tx1  = (int)((nx + TILE - 2) / TILE);
    int ty_m = (int)((p->y + TILE / 2) / TILE); /* mid-row */
    bool blocked = false;
    for (int tx = tx0; tx <= tx1 && !blocked; tx++)
        if (sv_is_solid(w, tx, ty_m)) blocked = true;
    if (!blocked) p->x = nx;

    /* Clamp x */
    if (p->x < 0)                    p->x = 0;
    if (p->x > (SV_W - 1) * TILE)   p->x = (float)((SV_W - 1) * TILE);

    /* Vertical movement + collision */
    float ny = p->y + p->vy;
    int   ftx0 = (int)(p->x / TILE);
    int   ftx1 = (int)((p->x + TILE - 2) / TILE);

    p->on_ground = false;

    if (p->vy >= 0.0f) {
        /* Falling — check ground/platform below */
        int bot_tile_y = (int)((ny + TILE) / TILE);
        bool hit = false;
        for (int ftx = ftx0; ftx <= ftx1 && !hit; ftx++) {
            if (sv_is_solid(w, ftx, bot_tile_y)) hit = true;
            /* One-way platform: only land if previously above */
            if (sv_is_platform(w, ftx, bot_tile_y)) {
                float prev_bot = p->y + TILE;
                float new_bot  = ny    + TILE;
                int   plat_top = bot_tile_y * TILE;
                if ((int)prev_bot <= plat_top && (int)new_bot >= plat_top) hit = true;
            }
        }
        if (hit) {
            ny = (float)(bot_tile_y * TILE - TILE);
            p->vy       = 0.0f;
            p->on_ground = true;
        }
    } else {
        /* Rising — check ceiling */
        int top_tile_y = (int)(ny / TILE);
        bool hit = false;
        for (int ftx = ftx0; ftx <= ftx1 && !hit; ftx++)
            if (sv_is_solid(w, ftx, top_tile_y)) hit = true;
        if (hit) { ny = (float)((top_tile_y + 1) * TILE); p->vy = 0.0f; }
    }
    p->y = ny;

    /* Walk frame */
    if (dx != 0.0f && p->on_ground) {
        p->walk_frame += SV_WALK_ANIM_STEP;
        if (p->walk_frame >= WALK_FRAME_WRAP) p->walk_frame = 0.0f;
    } else if (p->on_ground) {
        p->walk_frame = 0.0f;
    }

    /* Pet trailing in sv */
    if (s->party_count > 0) {
        Pet *pet = &s->party[s->active_pet];
        float pdx = p->x - pet->trail_x;
        float dist = pdx < 0 ? -pdx : pdx;
        if (dist > TILE) {
            float spd = SV_SPEED * SV_PET_FOLLOW_MULT;
            pet->trail_x += (pdx > 0 ? spd : -spd);
        }
        pet->trail_y = p->y;
    }

    /* A button in sv: interact with adjacent sv node */
    if (inp->a_press) {
        int ftx = (int)(p->x / TILE) + (p->dir == DIR_RIGHT ? 1 : -1);
        int fty = (int)((p->y + TILE / 2) / TILE);
        uint8_t t = world_sv_tile(w, ftx, fty);
        if (t == SVT_ORE) {
            s->active_skill = SK_MINING;
            skill_add_xp(s, SK_MINING, XP_SV_MINING);
            state_log(s, "Mined ore!");
        } else if (t == SVT_TREE) {
            s->active_skill = SK_WOODCUT;
            skill_add_xp(s, SK_WOODCUT, XP_SV_WOODCUT);
            state_log(s, "Cut wood!");
        }
    }
}
