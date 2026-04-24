#pragma once
/*
 * Frame-level driver — lets each platform supply its own main().
 *
 * Desktop / Pico (src/main.c) drive this with a plain while-loop.
 * Emscripten  (src/main_web.c) drives it via emscripten_set_main_loop,
 * which requires returning control to the browser every frame.
 *
 * game_boot() returns false if world init fails; the caller decides how
 * to report that.
 */

#include <stdbool.h>

bool game_boot(void);
void game_step(void);
