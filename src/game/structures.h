#pragma once
/*
 * Placed homestead structures. One slot per built thing — workbench,
 * shack, chest, campfire, forge, workshop. Position is a tile (x, y).
 * Persisted across saves.
 *
 * Structures block movement and are non-buildable on top of (one
 * structure per tile).
 */
#include <stdint.h>
#include <stdbool.h>
#include "recipes.h"   /* CraftStation */

typedef enum {
    STK_NONE = 0,           /* empty slot / "no structure" sentinel */
    STK_WORKBENCH,
    STK_SHACK,
    STK_CHEST,
    STK_CAMPFIRE,
    STK_FORGE,
    STK_WORKSHOP,
    STK_KIND_COUNT,
} StructureKind;

typedef struct {
    uint8_t kind;            /* StructureKind; STK_NONE = unused slot */
    uint8_t tile_x, tile_y;
} Structure;

#define MAX_STRUCTURES 32

/* GameState owns the structure list — these helpers operate on a
 * pointer to that array. Forward-declared to avoid the include cycle. */
struct GameState;

const Structure *structure_at(const struct GameState *s, int tx, int ty);
const Structure *structure_adjacent(const struct GameState *s, int px, int py);

/* Build a structure at (tx, ty). Returns false if the slot is full,
 * the tile already has a structure, or the tile isn't walkable
 * terrain. */
bool structure_place(struct GameState *s, StructureKind kind, int tx, int ty);

/* The crafting station a structure exposes when interacted with.
 * Returns STATION_COUNT for non-station structures (shack, chest). */
CraftStation structure_station(StructureKind kind);

/* Display name for log messages. */
const char *structure_name(StructureKind kind);
