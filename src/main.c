#include "hal.h"
#include "config.h"
#include "game/state.h"
#include "game/td_cam.h"
#include "game/world.h"
#include "game/player.h"
#include "game/tick.h"
#include "game/save.h"
#include "render/renderer.h"
#include "ui/menu.h"

static GameState state;
static World     world;

int main(void) {
    hal_init();

    world_init(&world);
    state_init(&state);

    if (!save_read(&state)) {
        state.mode = MODE_TOPDOWN;
    }

    Input    inp;
    uint32_t last_game_tick = hal_ticks_ms();

    while (1) {
        hal_input_poll(&inp);

        if (state.mode == MODE_TOPDOWN) {
            if (inp.cam_l_press) {
                state.td_cam_bearing =
                    (uint8_t)((state.td_cam_bearing + TD_CAM_STEPS - 1u) % TD_CAM_STEPS);
            }
            if (inp.cam_r_press) {
                state.td_cam_bearing =
                    (uint8_t)((state.td_cam_bearing + 1u) % TD_CAM_STEPS);
            }
        }

        switch (state.mode) {
            case MODE_TOPDOWN: player_update_td(&state, &inp, &world); break;
            case MODE_MENU:    menu_update(&state, &inp); break;
        }

        if (inp.start_press && state.mode == MODE_TOPDOWN) {
            menu_open(&state);
        }

        uint32_t now = hal_ticks_ms();
        if (now - last_game_tick >= TICK_MS) {
            game_tick(&state, &world);
            last_game_tick = now;

            static uint32_t save_tick_count = 0;
            if (++save_tick_count >= 6u) {
                save_write(&state);
                save_tick_count = 0;
            }
        }

        render_frame(&state, &world);
    }

    hal_deinit();
    return 0;
}
