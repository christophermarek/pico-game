#pragma once
#include "../game/state.h"
#include "../game/world.h"
#include "hal.h"

void menu_update(GameState *s, World *w, const Input *inp);
void menu_render(GameState *s);
void menu_open(GameState *s);
void menu_close(GameState *s);
