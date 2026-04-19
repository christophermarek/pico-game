#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "state.h"

void pet_init(Pet *p, uint8_t species_id, uint8_t level);
void pet_check_evo(GameState *s, uint8_t party_idx);
void pet_learn_moves(Pet *p, uint8_t evo_stage);
void pet_add_xp(GameState *s, uint8_t party_idx, uint32_t xp);
void pet_equip(Pet *p, uint8_t equip_slot);
void pets_check_recruit(GameState *s);
