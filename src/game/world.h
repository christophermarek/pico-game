#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "config.h"

#define MAP_CELLS (MAP_W * MAP_H)
#define SV_CELLS  (SV_W  * SV_H)

typedef struct {
    uint8_t td_map[MAP_CELLS];
    uint8_t sv_map[SV_CELLS];
    uint8_t node_respawn[MAP_CELLS]; /* 0 = available */
} World;

void    world_init(World *w);
uint8_t world_tile(const World *w, int x, int y);
bool    world_walkable(const World *w, int x, int y);
bool    world_is_resource(uint8_t tile_id);
void    world_deplete_node(World *w, int x, int y);
void    world_tick(World *w);
uint8_t world_sv_tile(const World *w, int x, int y);
bool    sv_is_solid(const World *w, int x, int y);
bool    sv_is_platform(const World *w, int x, int y);
