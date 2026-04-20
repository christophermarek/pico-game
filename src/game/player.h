#pragma once
#include "state.h"
#include "world.h"
#include "hal.h"

void player_update_td(GameState *s, const Input *inp, World *w);
void player_do_action(GameState *s, World *w);
void player_stop_action(GameState *s);
