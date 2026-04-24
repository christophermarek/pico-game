#pragma once
#include "../game/state.h"
#include "../game/world.h"
#include "../game/recipes.h"
#include "hal.h"

/* Filter the craft list to one station. STATION_COUNT shows everything;
 * pass that for the general menu tab. When a placed station opens the
 * craft list, pass its station id so the player only sees what that
 * station can make. */
void craft_set_station(CraftStation station);

/* Per-frame input handler. Used by menu.c when the CRAFT tab is active.
 * Drives both list navigation and the in-progress craft timer. Returns
 * true if it consumed B-press (i.e. cancelled an in-flight craft).
 * Caller should treat false as "B may close the menu".
 *
 * World is needed to check tile walkability when crafting a recipe
 * that places a structure. */
bool craft_tab_update(GameState *s, const World *w, const Input *inp);

/* Render the craft list inside a vertical band: from y0 to y0+h-1. */
void craft_tab_render(const GameState *s, int y0, int h);

/* True while a craft is in progress — caller (menu.c) can use this to
 * gate tab-switching so swapping tabs doesn't leave a craft running
 * invisibly. */
bool craft_in_flight(void);
