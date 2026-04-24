#pragma once
/*
 * Tile → action lookup. Every gathering/interaction node in the world maps
 * to one entry here. player.c, skills.c, and the renderer all read from
 * this table instead of keeping their own switches — extending to a new
 * node type is a single row insertion.
 */
#include <stdint.h>
#include <stdbool.h>
#include "items.h"

typedef struct {
    uint8_t      tile;          /* T_TREE / T_ROCK / etc.                  */
    uint8_t      skill;         /* SK_WOODCUT / SK_MINING / SK_FISHING     */
    uint8_t      level_required;/* skill level gate (1..99)                */
    tool_kind_t  tool;          /* any item of this kind is accepted       */
    item_id_t    drop;          /* guaranteed drop; ITEM_NONE for chance   */
    uint8_t      drop_chance;   /* 1-in-N chance drops; 0 = guaranteed     */
    item_id_t    chance_drop;   /* item dropped on chance hit              */
    const char  *msg_start;     /* action-start log                        */
    const char  *msg_got;       /* on successful drop                      */
    const char  *msg_chance;    /* on chance drop                          */
} NodeAction;

/* Returns NULL if the tile has no action. */
const NodeAction *action_for_tile(uint8_t tile);

/* Action speed — divisor on ACTION_TICKS. Tool tier scales speed:
 * tier 1 = 1.0×, tier 2 = 1.5×, tier 3 = 2.0×, tier 4 = 3.0×. Higher
 * input tier → fewer ticks per action. Returns base ACTION_TICKS for
 * tier 0 (no tool / unknown). */
uint8_t action_ticks_for_tier(uint8_t tier);
