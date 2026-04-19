/*
 * GrumbleQuest integration-style tests (single translation unit, headless HAL).
 * Order is intentional: early suites do not depend on later ones.
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
#include "game/pets.h"

#include "render/renderer.h"
#include "render/font.h"

#include "ui/menu.h"
#include "ui/char_create.h"

#include "data/species.h"
#include "data/moves.h"
#include "data/equipment.h"
#include "data/berries.h"

static int g_failures;

static void T_(int ok, const char *file, int line, const char *expr)
{
    if (ok) return;
    fprintf(stderr, "%s:%d: assertion failed: %s\n", file, line, expr);
    g_failures++;
}

#define T(expr) T_((expr) ? 1 : 0, __FILE__, __LINE__, #expr)

/* ------------------------------------------------------------------ */
/* 1. config.h — layout and macro sanity                               */
/* ------------------------------------------------------------------ */
static void test_config(void)
{
    T(DISPLAY_W == 240 && DISPLAY_H == 240);
    T(MAP_W > 0 && MAP_H > 0);
    T(MAP_CELLS == MAP_W * MAP_H);
    T(SV_CELLS == SV_W * SV_H);
    T(SKILL_COUNT == 10);
    T(PARTY_MAX >= 1 && BOX_MAX >= 1);
    T(INV_SLOTS > 0 && MOVE_SLOTS == 4);
    T(TICK_MS > 0 && ACTION_TICKS > 0);
    T(FRAME_MS > 0);
    T(MODE_CHAR_CREATE < MODE_BASE);
}

/* ------------------------------------------------------------------ */
/* 2. colors.h — RGB565 / palette                                      */
/* ------------------------------------------------------------------ */
static void test_colors(void)
{
    T(C_BLACK == 0x0000u);
    T(C_WHITE == 0xFFFFu);
    T(RGB565(0, 0, 0) == C_BLACK);
    T(RGB565(255, 255, 255) == C_WHITE);
    T(HEX(0xFF0000) != HEX(0x0000FF));
    T(sizeof(C_SKIN_TONES) / sizeof(C_SKIN_TONES[0]) == 4);
    T(sizeof(C_HAIR_COLS) / sizeof(C_HAIR_COLS[0]) == 6);
    T(sizeof(C_OUTFIT_COLS) / sizeof(C_OUTFIT_COLS[0]) == 6);
}

/* ------------------------------------------------------------------ */
/* 3. data headers — tables and helpers                                */
/* ------------------------------------------------------------------ */
static void test_data_tables(void)
{
    T(SPECIES_COUNT >= 1);
    const Species *sp0 = get_species(0);
    T(sp0->id == 0);
    T(get_species(255)->id == 0); /* clamp */

    int16_t hp;
    int8_t  atk, def, spd;
    calc_stats(0, 5, &hp, &atk, &def, &spd);
    T(hp > 0 && atk > 0 && def > 0 && spd > 0);

    const Move *m1 = get_move(1);
    T(m1->power > 0 || m1->category == CAT_STATUS);

    const Equipment *eq1 = get_equipment(1);
    T(eq1 != NULL && eq1->name[0] != '\0');
    T(get_equipment(0) == NULL);

    T(BERRY_COUNT >= 1);
    T(BERRY_TABLE[0].name[0] != '\0');
}

/* ------------------------------------------------------------------ */
/* 4. state.c — init, XP, log, inventory                               */
/* ------------------------------------------------------------------ */
static void test_state(void)
{
    GameState s;
    state_init(&s);
    T(s.mode == MODE_CHAR_CREATE);
    T(s.hp > 0 && s.max_hp >= s.hp);
    T(s.party_count >= 1);
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

    state_add_xp_drop(&s, 10, 20, 42);
    T(s.xp_drop_count == 1);
    T(s.xp_drops[0].amount == 42);
}

/* ------------------------------------------------------------------ */
/* 5. world.c                                                          */
/* ------------------------------------------------------------------ */
static void test_world(void)
{
    World w;
    world_init(&w);
    T(world_tile(&w, MANSION_DOOR_TX, MANSION_DOOR_TY) == T_HOME);
    T(world_tile(&w, MANSION_DOOR_TX, MANSION_DOOR_TY - 1) == T_PATH); /* path spine to door */
    T(world_walkable(&w, MANSION_DOOR_TX, MANSION_DOOR_TY - 1));
    T(world_walkable(&w, 14, 9)); /* generic grass/path sanity */
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

    T(sv_is_solid(&w, 0, 8));
    T(sv_is_platform(&w, 5, 5));
}

/* ------------------------------------------------------------------ */
/* 6. pets / skills                                                    */
/* ------------------------------------------------------------------ */
static void test_pets_and_skills(void)
{
    GameState s;
    state_init(&s);
    Pet p;
    pet_init(&p, SP_GLUB, 3);
    T(p.level == 3);
    T(p.max_hp > 0 && p.hp == p.max_hp);
    pet_learn_moves(&p, 0);
    T(p.moves[0] > 0);

    uint32_t xp_before = s.party[0].xp;
    skill_add_xp(&s, SK_MINING, 1000);
    T(SKILL_INFO[SK_MINING].name[0] != '\0');
    T(s.skills[SK_MINING].xp > 0 || s.skills[SK_MINING].level > 1);
    T(s.party[0].xp >= xp_before);

    World w;
    world_init(&w);
    s.skilling          = true;
    s.active_skill      = SK_WOODCUT;
    s.action_node_x     = 3;
    s.action_node_y     = 12; /* inside tree forest region */
    T(world_tile(&w, s.action_node_x, s.action_node_y) == T_TREE);
    int inv_logs = s.log_count;
    skill_complete_action(&s, &w);
    T(s.action_ticks_left == ACTION_TICKS);
    T(s.log_count >= inv_logs);
}

/* ------------------------------------------------------------------ */
/* 7. game_tick                                                         */
/* ------------------------------------------------------------------ */
static void test_game_tick(void)
{
    GameState s;
    World     w;
    state_init(&s);
    world_init(&w);
    uint8_t hunger_before = s.hunger;
    game_tick(&s, &w);
    T(s.tick_count == 1u);
    T(s.hunger <= hunger_before);
}

/* ------------------------------------------------------------------ */
/* 8. save / HAL persistence                                           */
/* ------------------------------------------------------------------ */
static void test_save_roundtrip(void)
{
    hal_stub_reset();
    hal_display_init();
    hal_input_init();

    GameState a, b;
    state_init(&a);
    a.mode = MODE_TOPDOWN;
    strncpy(a.custom.name, "TestSave", sizeof(a.custom.name) - 1);
    a.custom.name[sizeof(a.custom.name) - 1] = '\0';

    T(save_write(&a));

    memset(&b, 0, sizeof(b));
    T(save_read(&b));
    T(b.mode == MODE_TOPDOWN);
    T(strcmp(b.custom.name, "TestSave") == 0);

    hal_stub_reset();
    hal_display_init();
    memset(&b, 0, sizeof(b));
    T(!save_read(&b));
}

/* ------------------------------------------------------------------ */
/* 10. player movement (no crash)                                      */
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
    s.mode = MODE_SIDE;
    player_update_sv(&s, &inp, &w);
    player_stop_action(&s);
}

/* ------------------------------------------------------------------ */
/* 11. render + UI + font (exercise full draw path)                   */
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

    s.mode = MODE_CHAR_CREATE;
    render_frame(&s, &w);
    T(hal_stub_fb_get(DISPLAY_W / 2, DISPLAY_H / 2) != 0);

    s.mode = MODE_TOPDOWN;
    render_frame(&s, &w);
    for (int b = 0; b < 4; b++) {
        s.td_cam_bearing = (uint8_t)b;
        render_frame(&s, &w);
    }

    s.mode = MODE_SIDE;
    render_frame(&s, &w);

    s.mode = MODE_TOPDOWN;
    menu_open(&s);
    render_frame(&s, &w);
    Input z;
    memset(&z, 0, sizeof(z));
    menu_update(&s, &z);
    menu_close(&s);

    s.mode = MODE_BASE;
    render_frame(&s, &w);

    T(font_str_width("ABC", 1) > 0);
    font_draw_str("ok", 0, 0, C_TEXT_WHITE, 1);

    char_create_render(&s);
}

/* ------------------------------------------------------------------ */
/* main                                                                 */
/* ------------------------------------------------------------------ */
int main(void)
{
    hal_init();

    test_config();
    test_colors();
    test_data_tables();
    test_state();
    test_world();
    test_pets_and_skills();
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
