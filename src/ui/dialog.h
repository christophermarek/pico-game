#pragma once
#include "../game/state.h"
#include "../game/npcs.h"
#include "hal.h"

/* Open a dialog box showing an NPC's line. After A-press it transitions
 * to the follow-up action (shopkeeper → shop menu; generic NPCs → close). */
void dialog_open(GameState *s, const Npc *npc);

void dialog_update(GameState *s, const Input *inp);
void dialog_render(const GameState *s);
