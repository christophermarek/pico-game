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
#include "ui/dialog.h"
#include "ui/shop.h"
#ifndef PICO_BUILD
#include <stdio.h>
#include <stdlib.h>
#endif

static GameState state;
static World     world;

int main(void) {
    hal_init();

    if (!world_init(&world)) {
#ifndef PICO_BUILD
        fprintf(stderr, "fatal: map file not found (assets/maps/map.bin)\n");
        hal_deinit();
        return 1;
#else
        while (1) {}  /* halt — no map on device */
#endif
    }
    state_init(&state);
    save_read(&state);

    Input    inp;
    uint32_t last_game_tick = hal_ticks_ms();
    uint32_t save_tick_count = 0;

    while (1) {
        hal_input_poll(&inp);
        state.frame_count++;

        if (state.mode == MODE_TOPDOWN) {
            if (inp.cam_l_press || inp.cam_r_press) {
                uint8_t delta = inp.cam_l_press ? (TD_CAM_STEPS - 1u) : 1u;
                state.td_cam_bearing =
                    (uint8_t)((state.td_cam_bearing + delta) % TD_CAM_STEPS);
                player_camera_rotated(&state);
            }
        }

        switch (state.mode) {
            case MODE_TOPDOWN: player_update_td(&state, &inp, &world); break;
            case MODE_MENU:    menu_update(&state, &world, &inp);      break;
            case MODE_DIALOG:  dialog_update(&state, &inp);             break;
            case MODE_SHOP:    shop_update(&state, &inp);               break;
        }

        if (inp.start_press && state.mode == MODE_TOPDOWN)
            menu_open(&state);

        uint32_t now = hal_ticks_ms();
        if (now - last_game_tick >= TICK_MS) {
            game_tick(&state, &world);
            last_game_tick = now;
            if (++save_tick_count >= SAVE_EVERY_TICKS) {
                save_write(&state);
                save_tick_count = 0;
            }
        }

        world_anim_tick(&world);
        state_anim_tick(&state);
        render_frame(&state, &world);
    }

    hal_deinit();
    return 0;
}
