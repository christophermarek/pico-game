#pragma once
#include "../game/state.h"
#include "hal.h"

void menu_update(GameState *s, const Input *inp);
void menu_render(GameState *s);
void menu_open(GameState *s);
void menu_close(GameState *s);
