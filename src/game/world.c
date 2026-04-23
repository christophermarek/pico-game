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

void world_init(World *w)
{
    if (load_map(w, "assets/maps/map.bin")) return;
    if (load_map(w, "../assets/maps/map.bin")) return;
    memset(w, 0, sizeof(*w));
    w->w = MAP_W;
    w->h = MAP_H;
}

uint8_t world_tile(const World *w, int x, int y)
{
    if (x < 0 || x >= w->w || y < 0 || y >= w->h)
        return T_GRASS;
    return w->td_map[y * w->w + x];
}

bool world_walkable(const World *w, int x, int y)
{
    uint8_t t = world_tile(w, x, y);
    return !(t == T_TREE || t == T_ROCK || t == T_ORE || t == T_WATER);
}

void world_deplete_node(World *w, int x, int y)
{
    if (x < 0 || x >= w->w || y < 0 || y >= w->h) return;
    w->node_respawn[y * w->w + x] = NODE_RESPAWN_TICKS;
}

void world_tick(World *w)
{
    int cells = w->w * w->h;
    for (int i = 0; i < cells; i++) {
        if (w->node_respawn[i] > 0)
            w->node_respawn[i]--;
    }
}
