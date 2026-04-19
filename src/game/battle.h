#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "state.h"
#include "hal.h"

void battle_start_wild(GameState *s, uint8_t species_id, uint8_t level);
void battle_update(GameState *s, const Input *inp);
void battle_end(GameState *s, bool won);

int  battle_calc_damage(int atk, int def, int power, float tm, int8_t atk_mod);
bool battle_try_catch(GameState *s);
void battle_add_log(BattleState *b, const char *msg);
