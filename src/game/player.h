#pragma once
#include "state.h"
#include "world.h"
#include "hal.h"

void player_update_td(GameState *s, const Input *inp, World *w);
void player_do_action(GameState *s, World *w);
void player_stop_action(GameState *s);

/* Debug: true if the player AABB at (x, y) would collide with any blocking tile. */
bool player_test_collide(const World *w, float x, float y);

/* Debug: if (x,y) collides, writes the blocking tile coords and the tile-id
 * digit (e.g. '4' for T_TREE, '5' for T_ROCK, '9' for T_ORE). Returns true
 * iff collision. */
bool player_collide_who(const World *w, float x, float y,
                        int *out_tx, int *out_ty, char *out_kind);
