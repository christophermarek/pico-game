#include "battle_ui.h"
#include "hal.h"
#include "colors.h"
#include "config.h"
#include "../render/font.h"
#include "../render/sprites.h"
#include "../data/species.h"
#include "../data/moves.h"
#include "../game/state.h"

/* ------------------------------------------------------------------ */
/* Helpers                                                              */
/* ------------------------------------------------------------------ */
static void draw_hp_bar(int x, int y, int w, int cur, int max) {
    hal_fill_rect(x, y, w, 5, C_BG_DARK);
    if (max > 0) {
        int fill = cur * w / max;
        if (fill > w) fill = w;
        uint16_t col = C_HP_GREEN;
        if (cur < max / 4) col = C_WARN_RED;
        else if (cur < max / 2) col = C_HUNGER_ORG;
        if (fill > 0) hal_fill_rect(x, y, fill, 5, col);
    }
}


static void draw_int_b2(int x, int y, int val, uint16_t color) {
    char buf[8]; int n = 0;
    if (val == 0) { buf[n++] = '0'; }
    else {
        char tmp[7]; int tn = 0;
        int v = val;
        while (v > 0) { tmp[tn++] = (char)('0' + v % 10); v /= 10; }
        for (int i = tn - 1; i >= 0; i--) buf[n++] = tmp[i];
    }
    buf[n] = '\0';
    font_draw_str(buf, x, y, color, 1);
}

/* ------------------------------------------------------------------ */
/* Full battle screen                                                   */
/* ------------------------------------------------------------------ */
void battle_ui_render(GameState *s) {
    BattleState *b = &s->battle;

    /* Background */
    hal_fill(C_BG);

    /* --- Enemy zone (top-right) --- */
    /* Enemy monster large sprite */
    spr_monster_large(130, 10, b->enemy_evo);

    /* Enemy name + level */
    const Species *esp = get_species(b->enemy_species);
    font_draw_str(esp->evo_names[b->enemy_evo], 5, 10, C_TEXT_WHITE, 1);
    font_draw_str("Lv", 5, 20, C_TEXT_DIM, 1);
    draw_int_b2(17, 20, b->enemy_level, C_GOLD);

    /* Enemy HP bar */
    font_draw_str("HP", 5, 30, C_HP_GREEN, 1);
    draw_hp_bar(20, 30, 90, b->enemy_hp, b->enemy_max_hp);
    draw_int_b2(115, 30, b->enemy_hp,     C_TEXT_WHITE);
    font_draw_str("/", 125, 30, C_TEXT_DIM, 1);
    draw_int_b2(131, 30, b->enemy_max_hp, C_TEXT_DIM);

    /* Damage flash */
    if (b->anim_timer > 0) {
        hal_fill_rect(130, 10, 60, 60, C_WARN_RED);
    }

    /* --- Player pet zone (bottom-left) --- */
    if (s->party_count > 0) {
        const Pet *pet = &s->party[s->active_pet];
        const Species *psp = get_species(pet->species_id);

        spr_monster(10, 100, pet->evo_stage, 0.0f);

        font_draw_str(psp->evo_names[pet->evo_stage], 35, 105, C_TEXT_WHITE, 1);
        font_draw_str("Lv", 35, 115, C_TEXT_DIM, 1);
        draw_int_b2(47, 115, pet->level, C_GOLD);

        font_draw_str("HP", 35, 125, C_HP_GREEN, 1);
        draw_hp_bar(50, 125, 80, pet->hp, pet->max_hp);
        draw_int_b2(135, 125, pet->hp,     C_TEXT_WHITE);
        font_draw_str("/", 147, 125, C_TEXT_DIM, 1);
        draw_int_b2(153, 125, pet->max_hp, C_TEXT_DIM);

        /* Happiness indicator */
        font_draw_str("Hap", 35, 135, C_HAPPY_PINK, 1);
        hal_fill_rect(60, 136, 40, 3, C_BG_DARK);
        int hfill = pet->happiness * 40 / 100;
        if (hfill > 0) hal_fill_rect(60, 136, hfill, 3, C_HAPPY_PINK);
    }

    /* --- Battle log (3 lines) --- */
    int log_y = 145;
    hal_fill_rect(0, log_y, DISPLAY_W, 30, C_PANEL);
    hal_fill_rect(0, log_y, DISPLAY_W, 1,  C_BORDER);
    for (int i = 0; i < b->log_count && i < 3; i++) {
        /* Most recent = top */
        uint16_t c = (i == 0) ? C_TEXT_WHITE : C_TEXT_DIM;
        font_draw_str(b->log[i], 4, log_y + 2 + i * 10, c, 1);
    }

    /* --- Menu (bottom strip) --- */
    int menu_y = 178;
    hal_fill_rect(0, menu_y, DISPLAY_W, DISPLAY_H - menu_y, C_BG_DARK);
    hal_fill_rect(0, menu_y, DISPLAY_W, 1, C_BORDER);

    if (b->show_moves) {
        /* Move list */
        if (s->party_count > 0) {
            const Pet *pet = &s->party[s->active_pet];
            for (int i = 0; i < MOVE_SLOTS; i++) {
                int mx = 4  + (i % 2) * 118;
                int my = menu_y + 2 + (i / 2) * 18;
                uint8_t mid = pet->moves[i];
                uint16_t bg = (b->move_cursor == i) ? C_BORDER : C_PANEL;
                hal_fill_rect(mx, my, 114, 16, bg);

                if (mid > 0) {
                    const Move *mv = get_move(mid);
                    font_draw_str(mv->name, mx + 2, my + 2, C_TEXT_WHITE, 1);
                    font_draw_str("PP", mx + 72, my + 2, C_TEXT_DIM, 1);
                    draw_int_b2(88, my + 2, pet->move_pp[i], C_GOLD);
                    font_draw_str("/", 98, my + 2, C_TEXT_DIM, 1);
                    draw_int_b2(104, my + 2, 30, C_TEXT_DIM);
                } else {
                    font_draw_str("---", mx + 40, my + 4, C_TEXT_DIM, 1);
                }
            }
        }
        font_draw_str("B:Back", 185, DISPLAY_H - 10, C_TEXT_DIM, 1);

    } else if (b->show_bag) {
        /* Bag — show usable items */
        for (int i = 0; i < 8 && i < INV_SLOTS; i++) {
            int bx = 4  + (i % 2) * 118;
            int by = menu_y + 2 + (i / 2) * 14;
            const InvSlot *sl = &s->inv[i];
            uint16_t bg = (b->move_cursor == i) ? C_BORDER : C_BG;
            hal_fill_rect(bx, by, 114, 12, bg);
            if (sl->count > 0 && sl->item_id > ITEM_NONE) {
                static const char *ITEM_NAMES_SHORT[ITEM_COUNT] = {
                    "---","Ore","Stone","Fish","Sweed","Log","Branch","Gem",
                    "Meal","Bread","Egg","Coin","Thread","Hide","Herb",
                    "Potion","Orb","Oran","Sitrus","Pecha","Rawst",
                    "Leppa","Wiki","Mago","Lum",
                };
                font_draw_str(ITEM_NAMES_SHORT[sl->item_id], bx + 2, by + 2, C_TEXT_WHITE, 1);
                draw_int_b2(bx + 90, by + 2, sl->count, C_GOLD);
            } else {
                font_draw_str("---", bx + 45, by + 2, C_TEXT_DIM, 1);
            }
        }
        font_draw_str("B:Back", 185, DISPLAY_H - 10, C_TEXT_DIM, 1);

    } else if (b->show_swap) {
        /* Party list */
        for (int i = 0; i < s->party_count; i++) {
            const Pet *p = &s->party[i];
            const Species *sp2 = get_species(p->species_id);
            int py = menu_y + 2 + i * 14;
            uint16_t bg = (b->move_cursor == i) ? C_BORDER : C_BG;
            hal_fill_rect(4, py, DISPLAY_W - 8, 12, bg);
            font_draw_str(sp2->evo_names[p->evo_stage], 6, py + 2, C_TEXT_WHITE, 1);
            font_draw_str("Lv", 90, py + 2, C_TEXT_DIM, 1);
            draw_int_b2(102, py + 2, p->level, C_GOLD);
            /* HP mini bar */
            hal_fill_rect(120, py + 4, 60, 4, C_BG_DARK);
            int fill = (p->max_hp > 0) ? (p->hp * 60 / p->max_hp) : 0;
            if (fill > 0) hal_fill_rect(120, py + 4, fill, 4, C_HP_GREEN);
        }
        font_draw_str("B:Back", 185, DISPLAY_H - 10, C_TEXT_DIM, 1);

    } else {
        /* Main menu: FIGHT / BAG / SWAP / RUN */
        static const char *MENU_LABELS[4] = {"FIGHT", "BAG", "SWAP", "RUN"};
        for (int i = 0; i < 4; i++) {
            int mx = 10 + i * 57;
            uint16_t bg  = (b->menu_cursor == i) ? C_BORDER_ACT : C_BORDER;
            uint16_t col = (b->menu_cursor == i) ? C_BG         : C_TEXT_WHITE;
            hal_fill_rect(mx, menu_y + 3, 50, 18, bg);
            /* Centre text */
            int tw = font_str_width(MENU_LABELS[i], 1);
            font_draw_str(MENU_LABELS[i], mx + (50 - tw) / 2, menu_y + 8, col, 1);
        }
    }
}
