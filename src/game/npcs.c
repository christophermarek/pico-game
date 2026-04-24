#include "npcs.h"
#include <stddef.h>

/* The homestead NPC roster. Tile coordinates are on the 30×20 map.
 * When zones land, each zone gets its own array and npc_at() filters by
 * active zone. */
static const Npc NPCS[] = {
    {
        .kind     = NPC_SHOPKEEPER,
        .tile_x   = 15,
        .tile_y   = 9,
        .name     = "Shopkeeper",
        .greeting = "Welcome, newcomer. Care to buy something?",
    },
};

#define NPCS_N ((int)(sizeof(NPCS) / sizeof(NPCS[0])))

const Npc *npc_at(int tx, int ty) {
    for (int i = 0; i < NPCS_N; i++)
        if ((int)NPCS[i].tile_x == tx && (int)NPCS[i].tile_y == ty)
            return &NPCS[i];
    return NULL;
}

const Npc *npc_adjacent(int px, int py) {
    for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++)
            if (dx || dy) {
                const Npc *n = npc_at(px + dx, py + dy);
                if (n) return n;
            }
    return NULL;
}

int        npc_count(void)       { return NPCS_N; }
const Npc *npc_get(int i)        {
    return (i >= 0 && i < NPCS_N) ? &NPCS[i] : NULL;
}
