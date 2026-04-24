/*
 * Emscripten entry point. The browser owns the frame clock, so we register
 * a callback with emscripten_set_main_loop and return — execution resumes
 * inside the event loop.
 *
 * Movement is still parameterised in pixels-per-frame (TD_SPEED etc.),
 * so we gate game_step to TARGET_FPS by checking elapsed time. Without
 * this a 120 Hz display would run the game at 4× speed.
 */

#include "game_loop.h"
#include "hal.h"
#include "config.h"

#include <emscripten.h>
#include <stdio.h>

static void web_frame(void)
{
    static uint32_t last_ms = 0;
    uint32_t now = hal_ticks_ms();
    if (last_ms != 0 && now - last_ms < (uint32_t)FRAME_MS) return;
    last_ms = now;
    game_step();
}

int main(void)
{
    if (!game_boot()) {
        fprintf(stderr, "fatal: map preload missing (assets/maps/map.bin)\n");
        return 1;
    }
    emscripten_set_main_loop(web_frame, 0, 1);
    return 0;
}
