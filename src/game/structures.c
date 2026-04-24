#include "structures.h"
#include "state.h"
#include "world.h"
#include <stddef.h>

const Structure *structure_at(const GameState *s, int tx, int ty) {
    for (int i = 0; i < MAX_STRUCTURES; i++) {
        const Structure *st = &s->structures[i];
        if (st->kind == STK_NONE) continue;
        if ((int)st->tile_x == tx && (int)st->tile_y == ty) return st;
    }
    return NULL;
}

const Structure *structure_adjacent(const GameState *s, int px, int py) {
    for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++)
            if (dx || dy) {
                const Structure *st = structure_at(s, px + dx, py + dy);
                if (st) return st;
            }
    return NULL;
}

bool structure_place(GameState *s, StructureKind kind, int tx, int ty) {
    if (kind == STK_NONE || kind >= STK_KIND_COUNT) return false;
    if (structure_at(s, tx, ty)) return false;

    /* Find an empty slot. */
    int slot = -1;
    for (int i = 0; i < MAX_STRUCTURES; i++)
        if (s->structures[i].kind == STK_NONE) { slot = i; break; }
    if (slot < 0) return false;

    s->structures[slot].kind   = (uint8_t)kind;
    s->structures[slot].tile_x = (uint8_t)tx;
    s->structures[slot].tile_y = (uint8_t)ty;
    return true;
}

CraftStation structure_station(StructureKind kind) {
    switch (kind) {
    case STK_WORKBENCH: return STATION_WORKBENCH;
    case STK_WORKSHOP:  return STATION_WORKSHOP;
    case STK_FORGE:     return STATION_FORGE;
    case STK_CAMPFIRE:  return STATION_CAMPFIRE;
    default:            return STATION_COUNT;   /* not a craft station */
    }
}

const char *structure_name(StructureKind kind) {
    switch (kind) {
    case STK_WORKBENCH: return "Workbench";
    case STK_SHACK:     return "Shack";
    case STK_CHEST:     return "Chest";
    case STK_CAMPFIRE:  return "Campfire";
    case STK_FORGE:     return "Forge";
    case STK_WORKSHOP:  return "Workshop";
    default:            return "?";
    }
}
