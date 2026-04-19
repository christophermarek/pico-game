#include "menu.h"
#include "hal.h"
#include "colors.h"
#include "config.h"
#include "../render/font.h"
#include "../render/sprites.h"
#include "../game/skills.h"
#include "../data/species.h"
#include "../data/equipment.h"

#define TAB_COUNT 4

static const char *TAB_NAMES[TAB_COUNT] = {
    "SKILLS", "ITEMS", "PARTY", "CRAFT"
};

/* ------------------------------------------------------------------ */
/* Open / close                                                         */
/* ------------------------------------------------------------------ */
void menu_open(GameState *s) {
    s->prev_mode   = s->mode;
    s->mode        = MODE_MENU;
    s->menu_tab    = MTAB_SKILLS;
    s->menu_cursor = 0;
    s->menu_scroll = 0;
}

void menu_close(GameState *s) {
    s->mode = s->prev_mode;
}

/* ------------------------------------------------------------------ */
/* Item names (abbreviated for grid display)                           */
/* ------------------------------------------------------------------ */
static const char *ITEM_NAMES[ITEM_COUNT] = {
    "---", "Ore", "Stone", "Fish", "Sweed",
    "Log", "Branch", "Gem", "Meal", "Bread",
    "Egg", "Coin", "Thread", "Hide", "Herb",
    "Potion", "Orb", "Oran", "Sitrus", "Pecha",
    "Rawst", "Leppa", "Wiki", "Mago", "Lum",
};

/* ------------------------------------------------------------------ */
/* Small draw_int helper                                                */
/* ------------------------------------------------------------------ */
static void draw_int_m(int x, int y, int val, uint16_t color) {
    char buf[8]; int n = 0;
    if (val == 0) { buf[n++] = '0'; }
    else {
        char tmp[7]; int tn = 0;
        while (val > 0) { tmp[tn++] = (char)('0' + val % 10); val /= 10; }
        for (int i = tn - 1; i >= 0; i--) buf[n++] = tmp[i];
    }
    buf[n] = '\0';
    font_draw_str(buf, x, y, color, 1);
}

/* ------------------------------------------------------------------ */
/* Horizontal XP bar                                                    */
/* ------------------------------------------------------------------ */
static void draw_bar_m(int x, int y, int w, int cur, int max, uint16_t color) {
    hal_fill_rect(x, y, w, 3, C_BG);
    if (max > 0) {
        int fill = (max > 0) ? (cur * w / max) : 0;
        if (fill > w) fill = w;
        if (fill > 0) hal_fill_rect(x, y, fill, 3, color);
    }
}

/* ------------------------------------------------------------------ */
/* Update                                                               */
/* ------------------------------------------------------------------ */
void menu_update(GameState *s, const Input *inp) {
    /* Tab switch with left/right */
    static bool lr_prev = false, ll_prev = false;
    if (inp->right && !lr_prev) {
        if ((int)s->menu_tab < TAB_COUNT - 1) {
            s->menu_tab    = (MenuTab)(s->menu_tab + 1);
            s->menu_cursor = 0;
            s->menu_scroll = 0;
        }
    }
    if (inp->left && !ll_prev) {
        if (s->menu_tab > 0) {
            s->menu_tab    = (MenuTab)(s->menu_tab - 1);
            s->menu_cursor = 0;
            s->menu_scroll = 0;
        }
    }
    lr_prev = inp->right;
    ll_prev = inp->left;

    static bool u_prev = false, d_prev = false;

    /* Cursor navigation within tab */
    switch (s->menu_tab) {
    case MTAB_SKILLS: {
        int max_c = SKILL_COUNT - 1;
        if (inp->up   && !u_prev && s->menu_cursor > 0)      s->menu_cursor--;
        if (inp->down && !d_prev && s->menu_cursor < (uint8_t)max_c) s->menu_cursor++;
        break;
    }
    case MTAB_ITEMS: {
        int max_c = INV_SLOTS - 1;
        if (inp->up   && !u_prev && s->menu_cursor > 0)      s->menu_cursor--;
        if (inp->down && !d_prev && s->menu_cursor < (uint8_t)max_c) s->menu_cursor++;
        break;
    }
    case MTAB_PARTY: {
        int max_c = s->party_count > 0 ? s->party_count - 1 : 0;
        if (inp->up   && !u_prev && s->menu_cursor > 0)           s->menu_cursor--;
        if (inp->down && !d_prev && s->menu_cursor < (uint8_t)max_c) s->menu_cursor++;
        /* A to set active */
        if (inp->a_press && s->menu_cursor < s->party_count)
            s->active_pet = s->menu_cursor;
        break;
    }
    case MTAB_CRAFTING:
    case MTAB_EQUIP: {
        int max_c = EQUIP_COUNT - 1;
        if (inp->up   && !u_prev && s->menu_cursor > 0)       s->menu_cursor--;
        if (inp->down && !d_prev && s->menu_cursor < (uint8_t)max_c) s->menu_cursor++;
        /* A to craft */
        if (inp->a_press) {
            uint8_t equip_idx = (uint8_t)(s->menu_cursor + 1);
            if (can_craft(s, equip_idx)) {
                consume_recipe(s, equip_idx);
                state_log(s, "Crafted!");
            } else {
                state_log(s, "Missing items.");
            }
        }
        break;
    }
    }

    u_prev = inp->up;
    d_prev = inp->down;

    if (inp->b_press) menu_close(s);
}

/* ------------------------------------------------------------------ */
/* Render                                                               */
/* ------------------------------------------------------------------ */
void menu_render(GameState *s) {
    /* Dim overlay */
    hal_fill_rect(0, 0, DISPLAY_W, DISPLAY_H, C_BG_DARK);

    /* Tab bar */
    for (int i = 0; i < TAB_COUNT; i++) {
        int tx = 5 + i * 58;
        uint16_t bg  = (i == (int)s->menu_tab) ? C_BORDER : C_PANEL;
        uint16_t col = (i == (int)s->menu_tab) ? C_TEXT_WHITE : C_TEXT_DIM;
        hal_fill_rect(tx, 2, 55, 12, bg);
        font_draw_str(TAB_NAMES[i], tx + 2, 4, col, 1);
    }
    hal_fill_rect(0, 14, DISPLAY_W, 1, C_BORDER);

    int content_y = 17;

    switch (s->menu_tab) {

    /* ---- SKILLS ---- */
    case MTAB_SKILLS: {
        for (int i = 0; i < SKILL_COUNT; i++) {
            int col  = i % 2;
            int row  = i / 2;
            int sx   = 5  + col * 115;
            int sy   = content_y + row * 22;

            uint16_t border_c = (s->menu_cursor == i) ? C_BORDER_ACT : C_BORDER;
            hal_fill_rect(sx, sy, 110, 20, C_PANEL);
            hal_fill_rect(sx, sy, 110, 20, C_BG);
            hal_fill_rect(sx, sy, 110, 1, border_c);
            hal_fill_rect(sx, sy+19, 110, 1, border_c);

            font_draw_str(SKILL_INFO[i].icon_str, sx + 1, sy + 2, C_TEXT_MAIN, 1);
            font_draw_str(SKILL_INFO[i].name,     sx + 16, sy + 2, C_TEXT_WHITE, 1);
            draw_int_m(sx + 80, sy + 2, s->skills[i].level, C_GOLD);

            /* XP bar */
            uint32_t cur_xp  = s->skills[i].xp - xp_for_level(s->skills[i].level);
            uint32_t next_xp = xp_for_level((uint8_t)(s->skills[i].level + 1)) -
                               xp_for_level(s->skills[i].level);
            draw_bar_m(sx + 1, sy + 14, 108, (int)cur_xp, (int)(next_xp ? next_xp : 1), C_XP_PURPLE);
        }
        break;
    }

    /* ---- ITEMS ---- */
    case MTAB_ITEMS: {
        for (int i = 0; i < INV_SLOTS; i++) {
            int col = i % 4;
            int row = i / 4;
            int sx  = 5  + col * 58;
            int sy  = content_y + row * 26;

            uint16_t border_c = (s->menu_cursor == i) ? C_BORDER_ACT : C_BORDER;
            hal_fill_rect(sx, sy, 55, 24, C_PANEL);
            hal_fill_rect(sx, sy, 55,  1, border_c);
            hal_fill_rect(sx, sy+23, 55, 1, border_c);

            const InvSlot *slot = &s->inv[i];
            if (slot->count > 0 && slot->item_id < ITEM_COUNT) {
                font_draw_str(ITEM_NAMES[slot->item_id], sx + 2, sy + 3, C_TEXT_WHITE, 1);
                draw_int_m(sx + 42, sy + 14, slot->count, C_GOLD);
            } else {
                font_draw_str("---", sx + 15, sy + 8, C_TEXT_DIM, 1);
            }
        }
        break;
    }

    /* ---- PARTY ---- */
    case MTAB_PARTY: {
        for (int i = 0; i < s->party_count; i++) {
            const Pet *p  = &s->party[i];
            const Species *sp = get_species(p->species_id);
            int sy = content_y + i * 44;

            uint16_t border_c = (s->menu_cursor == i) ? C_BORDER_ACT
                              : (s->active_pet == i)   ? C_GOLD
                              : C_BORDER;
            hal_fill_rect(4, sy, DISPLAY_W - 8, 42, C_PANEL);
            hal_fill_rect(4, sy, DISPLAY_W - 8, 1, border_c);
            hal_fill_rect(4, sy+41, DISPLAY_W - 8, 1, border_c);

            /* Mini sprite */
            spr_monster(6, sy + 2, p->evo_stage, 0.0f);

            /* Name + level */
            const char *pname = sp->evo_names[p->evo_stage];
            font_draw_str(pname, 26, sy + 2, C_TEXT_WHITE, 1);
            font_draw_str("Lv", 100, sy + 2, C_TEXT_DIM, 1);
            draw_int_m(114, sy + 2, p->level, C_GOLD);

            /* HP bar */
            font_draw_str("HP", 26, sy + 14, C_HP_GREEN, 1);
            draw_bar_m(40, sy + 16, 80, p->hp, p->max_hp, C_HP_GREEN);

            /* Happiness bar */
            font_draw_str("Hap", 26, sy + 24, C_HAPPY_PINK, 1);
            draw_bar_m(48, sy + 26, 72, p->happiness, 100, C_HAPPY_PINK);

            /* Equip */
            const Equipment *e = get_equipment(p->equip_id);
            if (e) font_draw_str(e->name, 26, sy + 33, C_TEXT_DIM, 1);
        }
        if (s->party_count == 0)
            font_draw_str("No party members!", 40, content_y + 10, C_TEXT_DIM, 1);
        break;
    }

    /* ---- CRAFTING / EQUIP ---- */
    case MTAB_CRAFTING:
    case MTAB_EQUIP: {
        for (int i = 0; i < EQUIP_COUNT; i++) {
            const Equipment *e = &EQUIPMENT[i];
            int sy = content_y + i * 24;

            bool craftable = can_craft(s, (uint8_t)(i + 1));
            uint16_t border_c = (s->menu_cursor == i) ? C_BORDER_ACT : C_BORDER;
            uint16_t name_c   = craftable ? C_TEXT_WHITE : C_TEXT_DIM;

            hal_fill_rect(4, sy, DISPLAY_W - 8, 22, C_PANEL);
            hal_fill_rect(4, sy, DISPLAY_W - 8, 1, border_c);

            font_draw_str(e->name, 6, sy + 3, name_c, 1);

            /* Recipe summary */
            int rx = 100;
            for (int r = 0; r < e->recipe_count && rx < DISPLAY_W - 10; r++) {
                if (e->recipe[r].item_id > 0 && e->recipe[r].item_id < ITEM_COUNT) {
                    font_draw_str(ITEM_NAMES[e->recipe[r].item_id],
                                  rx, sy + 3, C_TEXT_DIM, 1);
                    rx += font_str_width(ITEM_NAMES[e->recipe[r].item_id], 1) + 2;
                    draw_int_m(rx, sy + 3, e->recipe[r].qty, C_GOLD);
                    rx += 12;
                }
            }

            /* Stat bonuses */
            if (s->menu_cursor == i) {
                font_draw_str("A+", 6, sy + 13, C_TEXT_DIM, 1);
                draw_int_m(18, sy + 13, e->atk_bonus, C_WARN_RED);
                font_draw_str("D+", 28, sy + 13, C_TEXT_DIM, 1);
                draw_int_m(40, sy + 13, e->def_bonus, C_ROCK_LIGHT);
                font_draw_str("S+", 50, sy + 13, C_TEXT_DIM, 1);
                draw_int_m(62, sy + 13, e->spd_bonus, C_ENERGY_BLUE);
            }
        }
        font_draw_str("A=Craft  B=Back", 60, DISPLAY_H - 12, C_TEXT_DIM, 1);
        break;
    }
    }

    /* Close hint */
    font_draw_str("B:Close", 190, DISPLAY_H - 12, C_TEXT_DIM, 1);
}
