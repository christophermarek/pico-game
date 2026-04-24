#pragma once
/*
 * NPC directory. One fixed-position entity per row; no AI, no pathing —
 * NPCs stand still, render with a single facing, and talk when you
 * A-press on an adjacent tile.
 *
 * Per zone, not per save — npc_for_tile() scans a compile-time table.
 * When zone travel lands, each zone gets its own table (or a filter on
 * a shared one).
 */
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    NPC_NONE = 0,
    NPC_SHOPKEEPER,
    NPC_COUNT,
} NpcKind;

typedef struct {
    NpcKind kind;
    uint8_t tile_x, tile_y;
    const char *name;
    const char *greeting;   /* default line before the shop menu opens */
} Npc;

/* Returns NULL if no NPC stands on (tx, ty). */
const Npc *npc_at(int tx, int ty);

/* Returns NULL if the player's foot tile isn't adjacent to any NPC. */
const Npc *npc_adjacent(int player_tx, int player_ty);

/* Iterate. */
int        npc_count(void);
const Npc *npc_get(int i);
