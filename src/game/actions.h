#pragma once
/*
 * Tile → action lookup. Every gathering/interaction node in the world maps
 * to one entry here. player.c, skills.c, and the renderer all read from
 * this table instead of each keeping its own switch — extending to a new
 * node type is a single row insertion.
 */
#include <stdint.h>
#include <stdbool.h>
#include "items.h"

typedef struct {
    uint8_t     tile;          /* T_TREE etc.                              */
    uint8_t     skill;         /* SK_WOODCUT / SK_MINING / SK_FISHING      */
    item_id_t   tool;          /* required tool                            */
    item_id_t   drop;          /* guaranteed drop; ITEM_NONE for chance    */
    uint8_t     drop_chance;   /* 1-in-N for chance drops; 0 = guaranteed  */
    item_id_t   chance_drop;   /* item dropped on chance hit               */
    const char *msg_start;     /* action-start log                         */
    const char *msg_got;       /* on successful drop ("Got X!" style)      */
    const char *msg_chance;    /* on chance drop                           */
} NodeAction;

/* Returns NULL if the tile has no action. */
const NodeAction *action_for_tile(uint8_t tile);
