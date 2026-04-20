/*
 * GrumbleQuest integration-style tests (single translation unit, headless HAL).
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "hal_stub.h"
#include "config.h"
#include "colors.h"

#include "game/state.h"
#include "game/world.h"
#include "game/player.h"
#include "game/tick.h"
#include "game/save.h"
#include "game/skills.h"

#include "render/renderer.h"
#include "render/font.h"

#include "ui/menu.h"

static int g_failures;

static void T_(int ok, const char *file, int line, const char *expr)
{
    if (ok) return;
    fprintf(stderr, "%s:%d: assertion failed: %s\n", file, line, expr);
    g_failures++;
}

#define T(expr) T_((expr) ? 1 : 0, __FILE__, __LINE__, #expr)

/* ------------------------------------------------------------------ */
/* 1. config.h                                                         */
/* ------------------------------------------------------------------ */
static void test_config(void)
{
    T(DISPLAY_W == 240 && DISPLAY_H == 240);
    T(MAP_W > 0 && MAP_H > 0);
    T(MAP_CELLS == MAP_W * MAP_H);
    T(SKILL_COUNT == 4);
    T(INV_SLOTS > 0);
    T(TICK_MS > 0 && ACTION_TICKS > 0);
    T(FRAME_MS > 0);
    T(MODE_TOPDOWN < MODE_MENU);
}

/* ------------------------------------------------------------------ */
/* 2. colors.h                                                         */
/* ------------------------------------------------------------------ */
static void test_colors(void)
{
    T(C_BLACK == 0x0000u);
    T(C_WHITE == 0xFFFFu);
    T(RGB565(0, 0, 0) == C_BLACK);
    T(RGB565(255, 255, 255) == C_WHITE);
    T(HEX(0xFF0000) != HEX(0x0000FF));
}

/* ------------------------------------------------------------------ */
/* 3. state.c — init, XP, log, inventory                               */
/* ------------------------------------------------------------------ */
static void test_state(void)
{
    GameState s;
    state_init(&s);
    T(s.mode == MODE_TOPDOWN);
    T(s.hp > 0 && s.max_hp >= s.hp);
    T(s.skills[0].level >= 1);
    T(inv_count(&s, ITEM_BREAD) >= 1);

    T(xp_for_level(1) < xp_for_level(10));
    T(total_level(&s) >= (uint16_t)SKILL_COUNT);

    state_log(&s, "test log");
    T(s.log_count >= 1);
    T(strncmp(s.log[0], "test log", 8) == 0);

    int before = inv_count(&s, ITEM_COIN);
    T(inv_add(&s, ITEM_COIN, 5));
    T(inv_count(&s, ITEM_COIN) == before + 5);
    T(inv_remove(&s, ITEM_COIN, 5));
    T(inv_count(&s, ITEM_COIN) == before);
}

/* ------------------------------------------------------------------ */
/* 4. world.c                                                          */
/* ------------------------------------------------------------------ */
static void test_world(void)
{
    World w;
    world_init(&w);
    T(world_walkable(&w, 14, 9));
    T(!world_walkable(&w, 2, 12)); /* forest tree */
    T(world_is_resource(T_TREE));
    T(!world_is_resource(T_GRASS));

    int idx = 4 + 13 * MAP_W;
    T(world_tile(&w, 4, 13) == T_ORE);
    world_deplete_node(&w, 4, 13);
    T(w.node_respawn[idx] > 0);
    for (int i = 0; i < NODE_RESPAWN_TICKS; i++)
        world_tick(&w);
    T(w.node_respawn[idx] == 0);
}

/* ------------------------------------------------------------------ */
/* 5. skills                                                           */
/* ------------------------------------------------------------------ */
static void test_skills(void)
{
    GameState s;
    state_init(&s);

    skill_add_xp(&s, SK_MINING, 1000);
    T(SKILL_INFO[SK_MINING].name[0] != '\0');
    T(s.skills[SK_MINING].xp > 0 || s.skills[SK_MINING].level > 1);

    World w;
    world_init(&w);
    s.skilling      = true;
    s.active_skill  = SK_WOODCUT;
    s.action_node_x = 3;
    s.action_node_y = 12;
    T(world_tile(&w, s.action_node_x, s.action_node_y) == T_TREE);
    int inv_logs = s.log_count;
    skill_complete_action(&s, &w);
    T(s.action_ticks_left == ACTION_TICKS);
    T(s.log_count >= inv_logs);
}

/* ------------------------------------------------------------------ */
/* 6. game_tick                                                        */
/* ------------------------------------------------------------------ */
static void test_game_tick(void)
{
    GameState s;
    World     w;
    state_init(&s);
    world_init(&w);
    game_tick(&s, &w);
    T(s.tick_count == 1u);
}

/* ------------------------------------------------------------------ */
/* 7. save / HAL persistence                                           */
/* ------------------------------------------------------------------ */
static void test_save_roundtrip(void)
{
    hal_stub_reset();
    hal_display_init();
    hal_input_init();

    GameState a, b;
    state_init(&a);
    a.mode = MODE_TOPDOWN;

    T(save_write(&a));

    memset(&b, 0, sizeof(b));
    T(save_read(&b));
    T(b.mode == MODE_TOPDOWN);

    hal_stub_reset();
    hal_display_init();
    memset(&b, 0, sizeof(b));
    T(!save_read(&b));
}

/* ------------------------------------------------------------------ */
/* 8. player movement                                                  */
/* ------------------------------------------------------------------ */
static void test_player(void)
{
    GameState s;
    World     w;
    state_init(&s);
    world_init(&w);
    s.mode = MODE_TOPDOWN;
    Input inp;
    memset(&inp, 0, sizeof(inp));
    player_update_td(&s, &inp, &w);
    player_stop_action(&s);
}

/* ------------------------------------------------------------------ */
/* 9. render + UI + font                                               */
/* ------------------------------------------------------------------ */
static void test_render_and_ui(void)
{
    hal_stub_reset();
    hal_display_init();
    hal_input_init();

    GameState s;
    World     w;
    state_init(&s);
    world_init(&w);

    s.mode = MODE_TOPDOWN;
    render_frame(&s, &w);
    for (int b = 0; b < 4; b++) {
        s.td_cam_bearing = (uint8_t)b;
        render_frame(&s, &w);
    }

    s.mode = MODE_TOPDOWN;
    menu_open(&s);
    render_frame(&s, &w);
    Input z;
    memset(&z, 0, sizeof(z));
    menu_update(&s, &z);
    menu_close(&s);

    T(font_str_width("ABC", 1) > 0);
    font_draw_str("ok", 0, 0, C_TEXT_WHITE, 1);
}

/* ------------------------------------------------------------------ */
/* main                                                                 */
/* ------------------------------------------------------------------ */
int main(void)
{
    hal_init();

    test_config();
    test_colors();
    test_state();
    test_world();
    test_skills();
    test_game_tick();
    test_save_roundtrip();
    test_player();
    test_render_and_ui();

    hal_deinit();

    if (g_failures > 0) {
        fprintf(stderr, "\n%d test assertion(s) failed.\n", g_failures);
        return 1;
    }
    printf("grumblequest_tests: all assertions passed.\n");
    return 0;
}
