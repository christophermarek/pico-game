#include "game_loop.h"
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

#define SAVE_EVERY_TICKS 6u

static GameState state;
static World     world;
static uint32_t  last_game_tick;
static uint32_t  save_tick_count;

bool game_boot(void)
{
    hal_init();
    if (!world_init(&world)) return false;
    state_init(&state);
    save_read(&state);
    last_game_tick  = hal_ticks_ms();
    save_tick_count = 0;
    return true;
}

void game_step(void)
{
    Input inp;
    hal_input_poll(&inp);
    state.frame_count++;

    if (state.mode == MODE_TOPDOWN) {
        if (inp.cam_l_press)
            state.td_cam_bearing =
                (uint8_t)((state.td_cam_bearing + TD_CAM_STEPS - 1u) % TD_CAM_STEPS);
        if (inp.cam_r_press)
            state.td_cam_bearing =
                (uint8_t)((state.td_cam_bearing + 1u) % TD_CAM_STEPS);
    }

    switch (state.mode) {
        case MODE_TOPDOWN: player_update_td(&state, &inp, &world); break;
        case MODE_MENU:    menu_update(&state, &world, &inp);      break;
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
