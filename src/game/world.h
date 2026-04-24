#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "config.h"

#define MAP_CELLS (MAP_W * MAP_H)

#define TILE_HIT_FRAMES     6   /* white-flash duration after each hit        */
#define TILE_DESTROY_FRAMES 18  /* burst-of-particles anim on final deplete   */

typedef struct {
    uint8_t w, h;
    uint8_t td_map[MAP_CELLS];
    uint8_t node_hp[MAP_CELLS];        /* remaining hits; 0 = depleted (permanent) */
    uint8_t tile_hit_timer[MAP_CELLS];     /* hit flash countdown               */
    uint8_t tile_destroy_timer[MAP_CELLS]; /* destruction burst countdown       */
} World;

/* Max HP per tile type — 0 means not a node. */
uint8_t world_node_max_hp(uint8_t tile_id);

bool    world_init(World *w); /* returns false if no map file found — caller must abort */
uint8_t world_tile(const World *w, int x, int y);
bool    world_walkable(const World *w, int x, int y);

/* A node is depleted when its hit-point counter hits zero. Depletion
 * is permanent; resources do not respawn. */
static inline bool world_node_depleted(const World *w, int x, int y) {
    if (x < 0 || x >= w->w || y < 0 || y >= w->h) return false;
    return w->node_hp[y * w->w + x] == 0;
}

/* Returns true if the node is now fully depleted (hp reached 0). */
bool    world_hit_node(World *w, int x, int y);

/* Called each frame — advances per-tile animation timers. */
void    world_anim_tick(World *w);

/* Called from skills.c after a successful hit lands. */
void    world_anim_on_hit(World *w, int tx, int ty);
