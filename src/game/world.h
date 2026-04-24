#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "config.h"

#define MAP_CELLS (MAP_W * MAP_H)

#define TILE_HIT_FRAMES   6    /* white-flash duration after each hit */

typedef struct {
    uint8_t w, h;
    uint8_t td_map[MAP_CELLS];
    uint8_t node_respawn[MAP_CELLS];
    uint8_t node_hp[MAP_CELLS];        /* remaining hits; 0 = depleted   */
    uint8_t tile_hit_timer[MAP_CELLS]; /* counts down per-tile after hit  */
} World;

/* Max HP per tile type — 0 means not a node. */
uint8_t world_node_max_hp(uint8_t tile_id);

bool    world_init(World *w); /* returns false if no map file found — caller must abort */
uint8_t world_tile(const World *w, int x, int y);
bool    world_walkable(const World *w, int x, int y);

/* Returns true if the node is now fully depleted (hp reached 0). */
bool    world_hit_node(World *w, int x, int y);

/* Called each frame — advances per-tile animation timers. */
void    world_anim_tick(World *w);

/* Called from skills.c after a successful hit lands. */
void    world_anim_on_hit(World *w, int tx, int ty);

void    world_tick(World *w);
