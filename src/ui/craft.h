#pragma once
#include "../game/state.h"
#include "../game/recipes.h"
#include "hal.h"

/* Open the craft menu. `station` is the crafting station filter; pass
 * STATION_COUNT (or any out-of-range value) to show every recipe
 * regardless of station — the dev-mode path we use before workbench
 * placement exists. */
void craft_open(GameState *s, CraftStation station);

void craft_update(GameState *s, const Input *inp);
void craft_render(const GameState *s);
