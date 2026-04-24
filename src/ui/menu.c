#include "menu.h"
#include "hal.h"
#include "colors.h"
#include "config.h"
#include "../render/font.h"
#include "../game/skills.h"
#include "../game/state.h"
#include "../game/items.h"
#include "../game/world.h"

#define TAB_COUNT 3

typedef enum {
    SETTINGS_DEBUG = 0,
    SETTINGS_RESET,
    SETTINGS_ITEM_COUNT,
} SettingsItem;

static const char *TAB_NAMES[TAB_COUNT] = { "SKILLS", "SETTINGS", "BAG" };

/* Scope the prior-mode stash here — only open/close care about it. */
static GameMode g_menu_prev_mode;

void menu_open(GameState *s) {
    g_menu_prev_mode = s->mode;
    s->mode          = MODE_MENU;
    s->menu_cursor   = 0;
}

void menu_close(GameState *s) {
    s->mode = g_menu_prev_mode;
}

static void draw_int(int x, int y, int val, uint16_t color) {
    char buf[8];
    int  n = 0;
    if (val == 0) {
        buf[n++] = '0';
    } else {
        char tmp[7];
        int  tn = 0;
        while (val > 0) { tmp[tn++] = (char)('0' + val % 10); val /= 10; }
        for (int i = tn - 1; i >= 0; i--) buf[n++] = tmp[i];
    }
    buf[n] = '\0';
    font_draw_str(buf, x, y, color, 1);
}

static void draw_xp_bar(int x, int y, int w, int cur, int max, uint16_t color) {
    hal_fill_rect(x, y, w, 3, C_BG);
    if (max > 0) {
        int fill = cur * w / max;
        if (fill > w) fill = w;
        if (fill > 0) hal_fill_rect(x, y, fill, 3, color);
    }
}

void menu_update(GameState *s, World *w, const Input *inp) {
    /* Save edge-trigger prev-state BEFORE updating so per-tab nav can use them. */
    static bool pl = false, pr = false, pu = false, pd = false;
    bool prev_l = pl, prev_r = pr, prev_u = pu, prev_d = pd;
    pl = inp->left; pr = inp->right; pu = inp->up; pd = inp->down;

    /* Tab switching: left/right at menu level (consumes the press). */
    bool tab_changed = false;
    if (inp->right && !prev_r && (int)s->menu_tab < TAB_COUNT - 1) {
        s->menu_tab = (MenuTab)(s->menu_tab + 1);
        s->menu_cursor = 0;
        tab_changed = true;
    }
    if (inp->left && !prev_l && s->menu_tab > 0) {
        s->menu_tab = (MenuTab)(s->menu_tab - 1);
        s->menu_cursor = 0;
        tab_changed = true;
    }
    if (inp->start_press) {
        s->menu_tab = (MenuTab)(((int)s->menu_tab + 1) % TAB_COUNT);
        s->menu_cursor = 0;
    }

    /* Per-tab navigation. */
    switch (s->menu_tab) {
    case MTAB_SKILLS: {
        int max_c = SKILL_COUNT - 1;
        if (inp->up   && !prev_u && s->menu_cursor > 0)              s->menu_cursor--;
        if (inp->down && !prev_d && s->menu_cursor < (uint8_t)max_c) s->menu_cursor++;
        break;
    }
    case MTAB_INVENTORY: {
        int max_slot = INV_SLOTS - 1;
        if (inp->up    && !prev_u && s->menu_cursor >= 4)
            s->menu_cursor = (uint8_t)(s->menu_cursor - 4);
        if (inp->down  && !prev_d && (int)s->menu_cursor + 4 <= max_slot)
            s->menu_cursor = (uint8_t)(s->menu_cursor + 4);
        /* Left/right within a row (only if tab didn't just switch). */
        if (!tab_changed) {
            if (inp->right && !prev_r && s->menu_cursor < max_slot &&
                    (s->menu_cursor % 4) < 3)
                s->menu_cursor++;
            if (inp->left && !prev_l && s->menu_cursor > 0 &&
                    (s->menu_cursor % 4) > 0)
                s->menu_cursor--;
        }
        /* A on a hotbar slot sets it as the active tool. */
        if (inp->a_press && s->menu_cursor < HOTBAR_SLOTS)
            s->active_slot = s->menu_cursor;
        break;
    }
    case MTAB_SETTINGS:
        if (inp->up   && !prev_u && s->settings_cursor > 0)
            s->settings_cursor--;
        if (inp->down && !prev_d && s->settings_cursor < SETTINGS_ITEM_COUNT - 1)
            s->settings_cursor++;
        if (inp->a_press) {
            if (s->settings_cursor == SETTINGS_DEBUG) {
                s->debug_mode = !s->debug_mode;
            } else if (s->settings_cursor == SETTINGS_RESET) {
                bool dbg = s->debug_mode;
                world_init(w);
                state_init(s);
                s->debug_mode = dbg;
                menu_close(s);
            }
        }
        break;
    }

    if (inp->b_press) menu_close(s);
}

void menu_render(GameState *s) {
    hal_fill_rect(0, 0, DISPLAY_W, DISPLAY_H, C_BG_DARK);

    /* 3 tabs: each ~78px wide with 2px gap */
    for (int i = 0; i < TAB_COUNT; i++) {
        int      tw  = (DISPLAY_W - 8) / TAB_COUNT;
        int      tx  = 4 + i * (tw + 2);
        uint16_t bg  = (i == (int)s->menu_tab) ? C_BORDER : C_PANEL;
        uint16_t col = (i == (int)s->menu_tab) ? C_TEXT_WHITE : C_TEXT_DIM;
        hal_fill_rect(tx, 2, tw, 12, bg);
        font_draw_str(TAB_NAMES[i], tx + 2, 4, col, 1);
    }
    hal_fill_rect(0, 14, DISPLAY_W, 1, C_BORDER);

    int content_y = 17;

    switch (s->menu_tab) {
    case MTAB_SKILLS:
        for (int i = 0; i < SKILL_COUNT; i++) {
            int sx = 5;
            int sy = content_y + i * 22;

            uint16_t border_c = (s->menu_cursor == i) ? C_BORDER_ACT : C_BORDER;
            hal_fill_rect(sx, sy,      230, 20, C_BG);
            hal_fill_rect(sx, sy,      230,  1, border_c);
            hal_fill_rect(sx, sy + 19, 230,  1, border_c);

            font_draw_str(SKILL_INFO[i].icon_str, sx + 2,   sy + 2, C_TEXT_MAIN,  1);
            font_draw_str(SKILL_INFO[i].name,     sx + 20,  sy + 2, C_TEXT_WHITE, 1);
            draw_int(sx + 200, sy + 2, s->skills[i].level, C_GOLD);

            uint32_t cur_xp  = s->skills[i].xp - xp_for_level(s->skills[i].level);
            uint32_t next_xp = xp_for_level((uint8_t)(s->skills[i].level + 1)) -
                               xp_for_level(s->skills[i].level);
            draw_xp_bar(sx + 2, sy + 14, 226, (int)cur_xp,
                        (int)(next_xp ? next_xp : 1), C_XP_PURPLE);
        }
        break;

    case MTAB_SETTINGS: {
        bool     dsel = (s->settings_cursor == SETTINGS_DEBUG);
        uint16_t dbdr = dsel ? C_BORDER_ACT : C_BORDER;
        hal_fill_rect(5, content_y,      230, 20, C_BG);
        hal_fill_rect(5, content_y,      230,  1, dbdr);
        hal_fill_rect(5, content_y + 19, 230,  1, dbdr);
        font_draw_str("Debug Mode", 10, content_y + 6, C_TEXT_WHITE, 1);
        font_draw_str(s->debug_mode ? "ON" : "OFF", 200, content_y + 6,
                      s->debug_mode ? C_HP_GREEN : C_TEXT_DIM, 1);
        content_y += 26;

        bool     rsel = (s->settings_cursor == SETTINGS_RESET);
        uint16_t rbdr = rsel ? C_BORDER_ACT : C_BORDER;
        hal_fill_rect(5, content_y,      230, 20, rsel ? C_WARN_RED : C_BG);
        hal_fill_rect(5, content_y,      230,  1, rbdr);
        hal_fill_rect(5, content_y + 19, 230,  1, rbdr);
        font_draw_str("Reset Game", 10, content_y + 6,
                      rsel ? C_TEXT_WHITE : C_TEXT_DIM, 1);

        font_draw_str("A:Select  B:Close", 5, DISPLAY_H - 12, C_TEXT_DIM, 1);
        break;
    }

    case MTAB_INVENTORY: {
        /* 4-column grid: hotbar (8 slots) then bag (20 slots). */
        int col_w = 54, row_h = 22;
        int gx = 5, gy = content_y;
        int info_y = DISPLAY_H - 14;  /* item name line at bottom */

        font_draw_str("Hotbar", gx, gy - 1, C_TEXT_DIM, 1);
        gy += 8;
        for (int i = 0; i < HOTBAR_SLOTS; i++) {
            int col = i % 4, row = i / 4;
            int sx = gx + col * col_w, sy = gy + row * row_h;
            const inv_slot_t *sl = &s->inv.slots[i];
            bool filled   = (sl->id != ITEM_NONE && sl->count > 0);
            bool selected = ((int)s->menu_cursor == i);
            bool is_active = (s->active_slot == i);
            uint16_t bg  = (i >= TOOL_SLOT_START && filled) ? HEX(0x1a1040) : C_BG;
            uint16_t bdr = selected  ? C_WHITE
                         : is_active ? C_BORDER_ACT
                         : filled    ? C_BORDER
                                     : HEX(0x1e1e3a);
            hal_fill_rect(sx, sy, col_w - 2, row_h - 2, bg);
            hal_fill_rect(sx,             sy,            col_w - 2, 1, bdr);
            hal_fill_rect(sx,             sy + row_h - 3, col_w - 2, 1, bdr);
            hal_fill_rect(sx,             sy,            1, row_h - 2, bdr);
            hal_fill_rect(sx + col_w - 3, sy,            1, row_h - 2, bdr);
            if (filled) {
                font_draw_str(ITEM_DEFS[sl->id].name, sx + 2, sy + 3,
                              ITEM_DEFS[sl->id].is_tool ? C_GOLD : C_TEXT_MAIN, 1);
                if (!ITEM_DEFS[sl->id].is_tool && sl->count > 1) {
                    char cnt[5]; int n = 0, v = sl->count;
                    if (v >= 100) cnt[n++] = (char)('0' + v / 100);
                    if (v >= 10)  cnt[n++] = (char)('0' + (v / 10) % 10);
                    cnt[n++] = (char)('0' + v % 10); cnt[n] = '\0';
                    font_draw_str(cnt, sx + 2, sy + 12, C_TEXT_DIM, 1);
                }
            }
        }

        gy += 2 * row_h + 4;
        font_draw_str("Bag", gx, gy - 1, C_TEXT_DIM, 1);
        gy += 8;
        for (int i = 0; i < BAG_SLOTS; i++) {
            int col = i % 4, row = i / 4;
            int sx = gx + col * col_w, sy = gy + row * row_h;
            if (sy + row_h > info_y - 2) continue; /* don't draw over info bar */
            const inv_slot_t *sl = &s->inv.slots[HOTBAR_SLOTS + i];
            bool filled   = (sl->id != ITEM_NONE && sl->count > 0);
            bool selected = ((int)s->menu_cursor == HOTBAR_SLOTS + i);
            uint16_t bdr = selected ? C_WHITE : filled ? C_BORDER : HEX(0x1e1e3a);
            hal_fill_rect(sx, sy, col_w - 2, row_h - 2, C_BG);
            hal_fill_rect(sx,             sy,            col_w - 2, 1, bdr);
            hal_fill_rect(sx,             sy + row_h - 3, col_w - 2, 1, bdr);
            hal_fill_rect(sx,             sy,            1, row_h - 2, bdr);
            hal_fill_rect(sx + col_w - 3, sy,            1, row_h - 2, bdr);
            if (filled) {
                font_draw_str(ITEM_DEFS[sl->id].name, sx + 2, sy + 3,
                              C_TEXT_MAIN, 1);
                if (sl->count > 1) {
                    char cnt[5]; int n = 0, v = sl->count;
                    if (v >= 100) cnt[n++] = (char)('0' + v / 100);
                    if (v >= 10)  cnt[n++] = (char)('0' + (v / 10) % 10);
                    cnt[n++] = (char)('0' + v % 10); cnt[n] = '\0';
                    font_draw_str(cnt, sx + 2, sy + 12, C_TEXT_DIM, 1);
                }
            }
        }

        /* Selected slot info bar at bottom. */
        hal_fill_rect(0, info_y - 2, DISPLAY_W, 16, C_BG);
        {
            const inv_slot_t *sel = &s->inv.slots[s->menu_cursor];
            if (sel->id != ITEM_NONE && sel->count > 0) {
                font_draw_str(ITEM_DEFS[sel->id].name, 5, info_y, C_TEXT_WHITE, 1);
                if (!ITEM_DEFS[sel->id].is_tool) {
                    char cnt[12]; int n = 0, v = sel->count;
                    cnt[n++] = 'x';
                    if (v >= 100) cnt[n++] = (char)('0' + v / 100);
                    if (v >= 10)  cnt[n++] = (char)('0' + (v / 10) % 10);
                    cnt[n++] = (char)('0' + v % 10); cnt[n] = '\0';
                    int cw = font_str_width(cnt, 1);
                    font_draw_str(cnt, DISPLAY_W - cw - 5, info_y, C_TEXT_DIM, 1);
                }
            } else {
                font_draw_str("Empty", 5, info_y, C_TEXT_DIM, 1);
            }
        }
        break;
    }
    }

    if (s->menu_tab != MTAB_SETTINGS)
        font_draw_str("B:Close", 190, DISPLAY_H - 12, C_TEXT_DIM, 1);
}
