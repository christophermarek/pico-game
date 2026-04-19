#pragma once
#include <stdbool.h>
#include "state.h"

bool save_write(const GameState *s);
bool save_read(GameState *s);
