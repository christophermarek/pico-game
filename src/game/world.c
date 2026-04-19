#include "world.h"
#include "config.h"
#include <string.h>

static inline int td_idx(int x, int y) { return y * MAP_W + x; }
static inline int sv_idx(int x, int y) { return y * SV_W + x; }

static inline void td_set(World *w, int x, int y, uint8_t t) {
    if (x >= 0 && x < MAP_W && y >= 0 && y < MAP_H)
        w->td_map[td_idx(x, y)] = t;
}
static inline void sv_set(World *w, int x, int y, uint8_t t) {
    if (x >= 0 && x < SV_W && y >= 0 && y < SV_H)
        w->sv_map[sv_idx(x, y)] = t;
}

/* ------------------------------------------------------------------ */
/* Top-down map — iso-first layout (orthogonal grid, readable routes) */
/* ------------------------------------------------------------------ */
static void build_td(World *w)
{
    memset(w->td_map, T_GRASS, MAP_CELLS);

    for (int x = 4; x <= 26; x++)
        td_set(w, x, 12, T_PATH);

    for (int y = 0; y < MAP_H; y++)
        td_set(w, 15, y, T_PATH);

    for (int y = 7; y <= 15; y++)
        td_set(w, 23, y, T_PATH);

    for (int y = 1; y <= 6; y++) {
        for (int x = 22; x <= 28; x++) {
            float dx = ((float)x - 25.0f) / 3.3f;
            float dy = ((float)y - 3.5f) / 2.9f;
            if (dx * dx + dy * dy <= 1.0f)
                td_set(w, x, y, T_WATER);
        }
    }

    for (int y = 0; y <= 7; y++) {
        for (int x = 19; x <= 29; x++) {
            if (world_tile(w, x, y) != T_GRASS)
                continue;
            bool adj = false;
            if (x > 0 && world_tile(w, x - 1, y) == T_WATER)
                adj = true;
            if (x < MAP_W - 1 && world_tile(w, x + 1, y) == T_WATER)
                adj = true;
            if (y > 0 && world_tile(w, x, y - 1) == T_WATER)
                adj = true;
            if (y < MAP_H - 1 && world_tile(w, x, y + 1) == T_WATER)
                adj = true;
            if (adj)
                td_set(w, x, y, T_SAND);
        }
    }

    for (int y = 14; y <= 18; y++) {
        for (int x = 2; x <= 9; x++) {
            if (!((x == 5 && y >= 14) || (y == 16 && x >= 2 && x <= 9)))
                td_set(w, x, y, T_TREE);
        }
    }

    for (int y = 13; y <= 18; y++) {
        for (int x = 20; x <= 27; x++) {
            if (!((x == 24 && y >= 13) || (y == 15 && x >= 20 && x <= 27)))
                td_set(w, x, y, T_ROCK);
        }
    }

    td_set(w, 4, 13, T_ORE);
    td_set(w, 8, 17, T_ORE);
    td_set(w, 24, 14, T_ORE);
    td_set(w, 26, 16, T_ORE);

    td_set(w, 2, 12, T_TREE);
    td_set(w, 3, 12, T_TREE);

    for (int y = 6; y <= 9; y++)
        for (int x = 6; x <= 10; x++)
            if (world_tile(w, x, y) == T_GRASS)
                td_set(w, x, y, T_TGRASS);

    for (int y = 14; y <= 17; y++)
        for (int x = 11; x <= 14; x++)
            if (world_tile(w, x, y) == T_GRASS)
                td_set(w, x, y, T_TGRASS);

    for (int y = 2; y <= 5; y++)
        for (int x = 2; x <= 5; x++)
            if (world_tile(w, x, y) == T_GRASS)
                td_set(w, x, y, T_TGRASS);

    static const int fx[] = { 1, 7, 12, 18, 20, 3, 17, 27, 10, 25 };
    static const int fy[] = { 2, 3, 6, 4, 8, 9, 2, 9, 11, 7 };
    for (int i = 0; i < 10; i++)
        if (world_tile(w, fx[i], fy[i]) == T_GRASS)
            td_set(w, fx[i], fy[i], T_FLOWER);

    for (int ty = MANSION_TY0; ty < MANSION_TY0 + MANSION_TH; ty++) {
        for (int tx = MANSION_TX0; tx < MANSION_TX0 + MANSION_TW; tx++) {
            if (tx == MANSION_DOOR_TX && ty == MANSION_DOOR_TY)
                td_set(w, tx, ty, T_HOME);
            else
                td_set(w, tx, ty, T_HOME_SOLID);
        }
    }
    for (int tx = MANSION_TX0; tx < MANSION_TX0 + MANSION_TW; tx++)
        td_set(w, tx, MANSION_TY0 + MANSION_TH, T_PATH);

    /* Path spine through the mansion lot up to the door (grass elsewhere on the lot). */
    for (int y = MANSION_TY0; y <= MANSION_TY0 + MANSION_TH; y++) {
        if (y == MANSION_DOOR_TY)
            td_set(w, MANSION_DOOR_TX, y, T_HOME);
        else
            td_set(w, MANSION_DOOR_TX, y, T_PATH);
    }
}

/* ------------------------------------------------------------------ */
/* Side-view map generation                                             */
/* ------------------------------------------------------------------ */
static void build_sv(World *w)
{
    memset(w->sv_map, SVT_AIR, SV_CELLS);

    for (int x = 0; x < SV_W; x++)
        sv_set(w, x, 8, SVT_GROUND);

    for (int x = 4; x <= 7; x++)
        sv_set(w, x, 5, SVT_PLATFORM);
    for (int x = 11; x <= 14; x++)
        sv_set(w, x, 4, SVT_PLATFORM);
    for (int x = 18; x <= 21; x++)
        sv_set(w, x, 5, SVT_PLATFORM);
    for (int x = 24; x <= 27; x++)
        sv_set(w, x, 3, SVT_PLATFORM);

    sv_set(w, 6, 7, SVT_ORE);
    sv_set(w, 15, 6, SVT_ORE);
    sv_set(w, 22, 7, SVT_ORE);

    sv_set(w, 2, 6, SVT_TREE);
    sv_set(w, 9, 6, SVT_TREE);
    sv_set(w, 17, 6, SVT_TREE);
    sv_set(w, 27, 6, SVT_TREE);
}

/* ------------------------------------------------------------------ */
/* Public API                                                           */
/* ------------------------------------------------------------------ */
void world_init(World *w)
{
    memset(w, 0, sizeof(*w));
    build_td(w);
    build_sv(w);
}

uint8_t world_tile(const World *w, int x, int y)
{
    if (x < 0 || x >= MAP_W || y < 0 || y >= MAP_H)
        return T_GRASS;
    return w->td_map[td_idx(x, y)];
}

bool world_walkable(const World *w, int x, int y)
{
    uint8_t t = world_tile(w, x, y);
    if (t == T_TREE || t == T_ROCK || t == T_WATER)
        return false;
    if (t == T_HOME_SOLID)
        return false;
    return true;
}

bool world_is_resource(uint8_t tile_id)
{
    return (tile_id == T_TREE || tile_id == T_ORE || tile_id == T_WATER || tile_id == T_ROCK);
}

void world_deplete_node(World *w, int x, int y)
{
    if (x < 0 || x >= MAP_W || y < 0 || y >= MAP_H)
        return;
    w->node_respawn[td_idx(x, y)] = NODE_RESPAWN_TICKS;
}

void world_tick(World *w)
{
    for (int i = 0; i < MAP_CELLS; i++) {
        if (w->node_respawn[i] > 0)
            w->node_respawn[i]--;
    }
}

uint8_t world_sv_tile(const World *w, int x, int y)
{
    if (x < 0 || x >= SV_W || y < 0 || y >= SV_H)
        return SVT_AIR;
    return w->sv_map[sv_idx(x, y)];
}

bool sv_is_solid(const World *w, int x, int y)
{
    uint8_t t = world_sv_tile(w, x, y);
    return (t == SVT_GROUND);
}

bool sv_is_platform(const World *w, int x, int y)
{
    uint8_t t = world_sv_tile(w, x, y);
    return (t == SVT_PLATFORM);
}
