#include "menu.h"
#include "hal.h"
#include "colors.h"
#include "config.h"
#include "../render/font.h"
#include "../game/skills.h"
#include "../game/state.h"
#include "../game/world.h"

#define TAB_COUNT 2
#define SETTINGS_ITEM_COUNT 2
#define SETTINGS_DEBUG 0
#define SETTINGS_RESET 1

static const char *TAB_NAMES[TAB_COUNT] = { "SKILLS", "SETTINGS" };

void menu_open(GameState *s) {
    s->prev_mode   = s->mode;
    s->mode        = MODE_MENU;
    s->menu_cursor = 0;
}

void menu_close(GameState *s) {
    s->mode = s->prev_mode;
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
    static bool lr_prev = false, ll_prev = false;
    if (inp->right && !lr_prev && (int)s->menu_tab < TAB_COUNT - 1) {
        s->menu_tab    = (MenuTab)(s->menu_tab + 1);
        s->menu_cursor = 0;
    }
    if (inp->left && !ll_prev && s->menu_tab > 0) {
        s->menu_tab    = (MenuTab)(s->menu_tab - 1);
        s->menu_cursor = 0;
    }
    if (inp->start_press) {
        s->menu_tab    = (MenuTab)(((int)s->menu_tab + 1) % TAB_COUNT);
        s->menu_cursor = 0;
    }
    lr_prev = inp->right;
    ll_prev = inp->left;

    static bool u_prev = false, d_prev = false;

    switch (s->menu_tab) {
    case MTAB_SKILLS: {
        int max_c = SKILL_COUNT - 1;
        if (inp->up   && !u_prev && s->menu_cursor > 0)              s->menu_cursor--;
        if (inp->down && !d_prev && s->menu_cursor < (uint8_t)max_c) s->menu_cursor++;
        break;
    }
    case MTAB_SETTINGS:
        if (inp->up   && !u_prev && s->settings_cursor > 0)
            s->settings_cursor--;
        if (inp->down && !d_prev && s->settings_cursor < SETTINGS_ITEM_COUNT - 1)
            s->settings_cursor++;
        if (inp->a_press) {
            if (s->settings_cursor == SETTINGS_DEBUG) {
                s->debug_mode = !s->debug_mode;
            } else if (s->settings_cursor == SETTINGS_RESET) {
                bool dbg = s->debug_mode;  /* preserve across reset */
                world_init(w);
                state_init(s);
                s->debug_mode = dbg;
                menu_close(s);
            }
        }
        break;
    }

    u_prev = inp->up;
    d_prev = inp->down;

    if (inp->b_press) menu_close(s);
}

void menu_render(GameState *s) {
    hal_fill_rect(0, 0, DISPLAY_W, DISPLAY_H, C_BG_DARK);

    for (int i = 0; i < TAB_COUNT; i++) {
        int      tx  = 5 + i * 118;
        uint16_t bg  = (i == (int)s->menu_tab) ? C_BORDER : C_PANEL;
        uint16_t col = (i == (int)s->menu_tab) ? C_TEXT_WHITE : C_TEXT_DIM;
        hal_fill_rect(tx, 2, 112, 12, bg);
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
    }

    if (s->menu_tab != MTAB_SETTINGS)
        font_draw_str("B:Close", 190, DISPLAY_H - 12, C_TEXT_DIM, 1);
}
