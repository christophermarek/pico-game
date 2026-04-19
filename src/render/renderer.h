#pragma once
#include "../game/state.h"
#include "../game/world.h"

void render_topdown(GameState *s, const World *w);
void render_sideview(GameState *s, const World *w);
void render_frame(GameState *s, const World *w);
