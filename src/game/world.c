#include "world.h"
#include "config.h"
#include <string.h>
#ifndef PICO_BUILD
#include <stdio.h>
#endif

#ifndef PICO_BUILD
/* .map file format: magic "PMAP" + version 1 + width + height + width*height tile bytes. */
static bool load_map(World *w, const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    uint8_t hdr[7];
    if (fread(hdr, 1, 7, f) != 7) { fclose(f); return false; }
    if (hdr[0] != 'P' || hdr[1] != 'M' || hdr[2] != 'A' || hdr[3] != 'P' ||
        hdr[4] != 1 || hdr[5] == 0 || hdr[6] == 0 ||
        hdr[5] > MAP_W || hdr[6] > MAP_H) {
        fclose(f); return false;
    }
    memset(w, 0, sizeof(*w));
    w->w = hdr[5];
    w->h = hdr[6];
    size_t cells = (size_t)w->w * w->h;
    size_t n = fread(w->td_map, 1, cells, f);
    fclose(f);
    return n == cells;
}
#else
static bool load_map(World *w, const char *path) { (void)w; (void)path; return false; }
#endif

uint8_t world_node_max_hp(uint8_t tile_id)
{
    switch (tile_id) {
        case T_TREE:   return 3;
        case T_ROCK:   return 4;
        case T_ORE:    return 5;
        case T_WATER:  return 1;
        case T_TGRASS: return 2;
        default:       return 0;
    }
}

static void init_node_hp(World *w)
{
    int cells = w->w * w->h;
    for (int i = 0; i < cells; i++)
        w->node_hp[i] = world_node_max_hp(w->td_map[i]);
}

bool world_init(World *w)
{
    if (load_map(w, "assets/maps/map.bin"))    { init_node_hp(w); return true; }
    if (load_map(w, "../assets/maps/map.bin")) { init_node_hp(w); return true; }
    return false;
}

uint8_t world_tile(const World *w, int x, int y)
{
    if (x < 0 || x >= w->w || y < 0 || y >= w->h)
        return T_GRASS;
    return w->td_map[y * w->w + x];
}

bool world_walkable(const World *w, int x, int y)
{
    /* Ground tiles always pass. Obstacle tiles (trees, rocks, ore, water
     * of every tier) block while alive but become walkable once depleted —
     * once the node is gone the tile is just empty ground visually.
     * Unrecognised / "air" values stay non-walkable. */
    uint8_t t = world_tile(w, x, y);
    switch (t) {
    case T_GRASS: case T_PATH: case T_SAND: case T_FLOWER: case T_TGRASS:
        return true;
    case T_TREE: case T_ROCK: case T_ORE: case T_WATER:
    case T_ORE_COPPER: case T_ORE_TIN: case T_ORE_SILVER: case T_ORE_GOLD:
    case T_TREE_PINE:  case T_TREE_MAPLE: case T_TREE_YEW:
    case T_WATER_RIVER: case T_WATER_DEEP: case T_WATER_DARK:
        return world_node_depleted(w, x, y);
    default:
        return false;
    }
}

bool world_hit_node(World *w, int x, int y)
{
    if (x < 0 || x >= w->w || y < 0 || y >= w->h) return false;
    int idx = y * w->w + x;
    if (w->node_hp[idx] == 0) return false;
    w->node_hp[idx]--;
    if (w->node_hp[idx] == 0) {
        w->tile_destroy_timer[idx] = TILE_DESTROY_FRAMES;
        return true;
    }
    return false;
}

void world_anim_on_hit(World *w, int tx, int ty)
{
    if (tx < 0 || tx >= w->w || ty < 0 || ty >= w->h) return;
    w->tile_hit_timer[ty * w->w + tx] = TILE_HIT_FRAMES;
}

void world_anim_tick(World *w)
{
    int cells = w->w * w->h;
    for (int i = 0; i < cells; i++) {
        if (w->tile_hit_timer[i] > 0)     w->tile_hit_timer[i]--;
        if (w->tile_destroy_timer[i] > 0) w->tile_destroy_timer[i]--;
    }
}

