#include "hal.h"
#include "config.h"
#include "game/state.h"
#include "game/td_cam.h"
#include "game/world.h"
#include "game/player.h"
#include "game/tick.h"
#include "game/battle.h"
#include "game/save.h"
#include "render/renderer.h"
#include "ui/menu.h"
#include "ui/char_create.h"

static GameState state;
static World     world;

int main(void) {
    hal_init();

    world_init(&world);
    state_init(&state);

    if (!save_read(&state)) {
        state.mode = MODE_CHAR_CREATE;
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
                state.td_cam_bearing = (uint8_t)((state.td_cam_bearing + 1u) % TD_CAM_STEPS);
            }
        }

        /* Mode dispatch */
        switch (state.mode) {
            case MODE_CHAR_CREATE: char_create_update(&state, &inp); break;
            case MODE_TOPDOWN:     player_update_td(&state, &inp, &world); break;
            case MODE_SIDE:        player_update_sv(&state, &inp, &world); break;
            case MODE_BATTLE:      battle_update(&state, &inp); break;
            case MODE_MENU:        menu_update(&state, &inp); break;
            case MODE_BASE:
                /* Basic: B button to leave base */
                if (inp.b_press) {
                    state.mode = state.prev_mode;
                    if (state.mode == MODE_BASE) state.mode = MODE_TOPDOWN;
                }
                break;
        }

        /* Handle view toggle (sel button) outside of battle/menu */
        if (inp.sel_press &&
            (state.mode == MODE_TOPDOWN || state.mode == MODE_SIDE)) {
            state.mode = (state.mode == MODE_TOPDOWN) ? MODE_SIDE : MODE_TOPDOWN;
        }

        /* Handle menu open (start button) outside of battle */
        if (inp.start_press &&
            (state.mode == MODE_TOPDOWN || state.mode == MODE_SIDE)) {
            menu_open(&state);
        }

        /* Game tick (every TICK_MS) */
        uint32_t now = hal_ticks_ms();
        if (now - last_game_tick >= TICK_MS) {
            game_tick(&state, &world);
            last_game_tick = now;

            /* Auto-save every tick */
            save_write(&state);
        }

        /* Render */
        render_frame(&state, &world);
        hal_show();
    }

    hal_deinit();
    return 0;
}
