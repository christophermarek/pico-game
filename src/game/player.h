#pragma once
#include "state.h"
#include "world.h"
#include "hal.h"

void player_update_td(GameState *s, const Input *inp, World *w);
void player_do_action(GameState *s, World *w);
void player_stop_action(GameState *s);

bool player_collide_who(const World *w, float x, float y,
                        int *out_tx, int *out_ty, char *out_kind);
