/* GrumbleQuest integration tests — single TU, headless HAL. */

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
#include "game/items.h"

#include "render/renderer.h"
#include "render/font.h"

#include "ui/menu.h"

static int g_failures;

static void T_(int ok, const char *file, int line, const char *expr) {
    if (ok) return;
    fprintf(stderr, "%s:%d: assertion failed: %s\n", file, line, expr);
    g_failures++;
}

#define T(expr) T_((expr) ? 1 : 0, __FILE__, __LINE__, #expr)

static void test_config(void) {
    T(DISPLAY_W == 240 && DISPLAY_H == 240);
    T(MAP_W > 0 && MAP_H > 0);
    T(MAP_CELLS == MAP_W * MAP_H);
    T(SKILL_COUNT == 3);
    T(TICK_MS > 0 && ACTION_TICKS > 0);
    T(FRAME_MS > 0);
    T(MODE_TOPDOWN < MODE_MENU);
}

static void test_colors(void) {
    T(C_BLACK == 0x0000u);
    T(C_WHITE == 0xFFFFu);
    T(RGB565(0, 0, 0) == C_BLACK);
    T(RGB565(255, 255, 255) == C_WHITE);
    T(HEX(0xFF0000) != HEX(0x0000FF));
}

static void test_state(void) {
    GameState s;
    state_init(&s);
    T(s.mode == MODE_TOPDOWN);
    T(s.hp > 0 && s.max_hp >= s.hp);
    T(s.skills[0].level >= 1);

    T(xp_for_level(1) < xp_for_level(10));

    state_log(&s, "test log");
    T(strncmp(s.log_msg, "test log", 8) == 0);
}

static void test_world(void) {
    World w;
    T(world_init(&w));                /* load succeeds: map.bin present */
    T(world_walkable(&w, 14, 9));

    /* Multi-hit depletion: a tree takes 3 hits before it's marked depleted. */
    int idx = 4 + 13 * MAP_W;
    w.td_map[idx]  = T_TREE;
    w.node_hp[idx] = world_node_max_hp(T_TREE);
    T(w.node_hp[idx] == 3);
    T(!world_hit_node(&w, 4, 13));    /* hit 1 — still alive */
    T(w.node_hp[idx] == 2);
    T(!world_node_depleted(&w, 4, 13));
    T(!world_hit_node(&w, 4, 13));    /* hit 2 — still alive */
    T(w.node_hp[idx] == 1);
    T(world_hit_node(&w, 4, 13));     /* hit 3 — returns true (depleted) */
    T(world_node_depleted(&w, 4, 13));
    T(w.tile_destroy_timer[idx] == TILE_DESTROY_FRAMES);

    /* Depletion is permanent — no respawn, HP stays at 0. */
    for (int i = 0; i < 60; i++) world_anim_tick(&w);
    T(w.node_hp[idx] == 0);
    T(w.tile_destroy_timer[idx] == 0); /* destroy burst timer ran out */

    /* Hit-flash timer advances via world_anim_tick. */
    world_anim_on_hit(&w, 4, 13);
    T(w.tile_hit_timer[idx] == TILE_HIT_FRAMES);
    world_anim_tick(&w);
    T(w.tile_hit_timer[idx] == TILE_HIT_FRAMES - 1);

    /* Out-of-bounds hit is a no-op, not a crash. */
    T(!world_hit_node(&w, -1, -1));
    T(!world_hit_node(&w, MAP_W, MAP_H));
}

static void test_inventory(void) {
    Inventory inv;
    inventory_init_default(&inv);

    /* Tools pre-loaded in slots 4–7. */
    T(inv.slots[TOOL_SLOT_START + 0].id == ITEM_AXE);
    T(inv.slots[TOOL_SLOT_START + 1].id == ITEM_PICKAXE);
    T(inv.slots[TOOL_SLOT_START + 2].id == ITEM_FISHING_ROD);
    T(inv.slots[TOOL_SLOT_START + 3].id == ITEM_SHEARS);
    T(inventory_has_tool(&inv, ITEM_AXE));
    T(inventory_has_tool(&inv, ITEM_SHEARS));
    T(!inventory_has_tool(&inv, ITEM_COIN));

    /* First resource lands in a free non-tool slot; returns that slot. */
    int s1 = inventory_add(&inv, ITEM_OAK_LOG, 1);
    T(s1 >= 0 && s1 != TOOL_SLOT_START);
    T(!(s1 >= TOOL_SLOT_START && s1 < TOOL_SLOT_START + 4));
    T(inv.slots[s1].id == ITEM_OAK_LOG && inv.slots[s1].count == 1);

    /* Same item stacks into the existing slot. */
    int s2 = inventory_add(&inv, ITEM_OAK_LOG, 5);
    T(s2 == s1);
    T(inv.slots[s1].count == 6);
    T(inventory_count(&inv, ITEM_OAK_LOG) == 6);

    /* Different item takes a new slot. */
    int s3 = inventory_add(&inv, ITEM_STONE, 1);
    T(s3 >= 0 && s3 != s1);

    /* Invalid args return -1. */
    T(inventory_add(&inv, ITEM_NONE, 1) == -1);
    T(inventory_add(&inv, ITEM_OAK_LOG, 0) == -1);

    /* Fill-to-full: every non-tool slot maxed with oak logs. New distinct
     * item has no empty slot and no matching stack to join. */
    Inventory full;
    inventory_init_default(&full);
    for (int i = 0; i < INV_SLOTS; i++) {
        if (i >= TOOL_SLOT_START && i < TOOL_SLOT_START + 4) continue;
        full.slots[i].id    = ITEM_OAK_LOG;
        full.slots[i].count = ITEM_DEFS[ITEM_OAK_LOG].max_stack;
    }
    T(inventory_add(&full, ITEM_COIN, 1) == -1);
    /* And more oak_log has nowhere to stack (every slot is at max). */
    T(inventory_add(&full, ITEM_OAK_LOG, 1) == -1);
}

static void test_item_fly(void) {
    GameState s;
    state_init(&s);

    /* Spawn a fly targeting hotbar slot 2. */
    state_spawn_item_fly(&s, 32.0f, 32.0f, ITEM_OAK_LOG, 2);
    T(s.item_flies[0].active);
    T(s.item_flies[0].timer == ITEM_FLY_FRAMES);
    T(s.item_flies[0].slot == 2);

    /* Tick to completion — slot flash should fire on landing. */
    for (int i = 0; i < ITEM_FLY_FRAMES; i++) state_anim_tick(&s);
    T(!s.item_flies[0].active);
    T(s.slot_flash[2] == SLOT_FLASH_FRAMES);  /* full flash duration visible */

    /* Flash decays over SLOT_FLASH_FRAMES ticks. */
    for (int i = 0; i < SLOT_FLASH_FRAMES; i++) state_anim_tick(&s);
    T(s.slot_flash[2] == 0);

    /* Bag-slot fly (slot >= HOTBAR_SLOTS) does NOT trigger any slot flash. */
    state_spawn_item_fly(&s, 0.0f, 0.0f, ITEM_STONE, HOTBAR_SLOTS + 3);
    for (int i = 0; i < ITEM_FLY_FRAMES; i++) state_anim_tick(&s);
    for (int i = 0; i < HOTBAR_SLOTS; i++) T(s.slot_flash[i] == 0);

    /* Pool saturation — 5th spawn with 4 active flies silently drops. */
    GameState s2;
    state_init(&s2);
    for (int i = 0; i < MAX_ITEM_FLIES; i++)
        state_spawn_item_fly(&s2, 0.0f, 0.0f, ITEM_STONE, 0);
    int active_before = 0;
    for (int i = 0; i < MAX_ITEM_FLIES; i++) if (s2.item_flies[i].active) active_before++;
    T(active_before == MAX_ITEM_FLIES);
    state_spawn_item_fly(&s2, 0.0f, 0.0f, ITEM_STONE, 0);  /* no free slot */
    int active_after = 0;
    for (int i = 0; i < MAX_ITEM_FLIES; i++) if (s2.item_flies[i].active) active_after++;
    T(active_after == MAX_ITEM_FLIES);
}

static void test_skills(void) {
    GameState s;
    state_init(&s);

    skill_add_xp(&s, SK_MINING, 1000);
    T(SKILL_INFO[SK_MINING].name[0] != '\0');
    T(s.skills[SK_MINING].xp > 0 || s.skills[SK_MINING].level > 1);

    World w;
    world_init(&w);
    s.skilling      = true;
    s.active_skill  = SK_WOODCUT;
    s.action_node_x = 1;
    s.action_node_y = 1;
    skill_complete_action(&s, &w);
    T(!s.skilling);                     /* action stops on completion */
    T(s.action_ticks_left == 0);
}

static void test_game_tick(void) {
    GameState s;
    World     w;
    state_init(&s);
    world_init(&w);
    game_tick(&s, &w);
    T(s.tick_count == 1u);
}

static void test_save_roundtrip(void) {
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

static void test_player(void) {
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

static void test_render_and_ui(void) {
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
    menu_update(&s, &w, &z);
    menu_close(&s);

    T(font_str_width("ABC", 1) > 0);
    font_draw_str("ok", 0, 0, C_TEXT_WHITE, 1);
}

int main(void) {
    hal_init();

    test_config();
    test_colors();
    test_state();
    test_world();
    test_inventory();
    test_item_fly();
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
