#include "shop.h"
#include "hal.h"
#include "colors.h"
#include "config.h"
#include "../render/font.h"
#include "../render/iso_spritesheet.h"
#include "../game/items.h"
#include <stdio.h>

/* Beat 1/2 economy: shopkeeper sells only tier-1 gear. Shears is the
 * tutorial gift but also re-buyable if the player somehow loses theirs. */
const ShopItem SHOP_ITEMS[] = {
    { ITEM_SHEARS,         5 },
    { ITEM_BRONZE_AXE,    15 },
    { ITEM_BRONZE_PICKAXE,15 },
    { ITEM_BRONZE_ROD,     8 },
};
const int SHOP_ITEM_COUNT = (int)(sizeof(SHOP_ITEMS) / sizeof(SHOP_ITEMS[0]));

void shop_open(GameState *s) {
    s->mode        = MODE_SHOP;
    s->menu_cursor = 0;
}

static void buy_selected(GameState *s) {
    const ShopItem *si = &SHOP_ITEMS[s->menu_cursor];
    int coins = inventory_count(&s->inv, ITEM_COIN);
    if (coins < si->price) {
        state_log(s, "Not enough coins.");
        return;
    }
    /* Deduct coins: walk inventory, decrement until price satisfied. */
    int remaining = si->price;
    for (int i = 0; i < INV_SLOTS && remaining > 0; i++) {
        if (s->inv.slots[i].id != ITEM_COIN) continue;
        uint8_t take = (s->inv.slots[i].count < remaining)
                       ? s->inv.slots[i].count : (uint8_t)remaining;
        s->inv.slots[i].count -= take;
        remaining -= take;
        if (s->inv.slots[i].count == 0) s->inv.slots[i].id = ITEM_NONE;
    }
    inventory_add(&s->inv, si->item, 1);
    char buf[36];
    snprintf(buf, sizeof(buf), "Bought %s.", ITEM_DEFS[si->item].name);
    state_log(s, buf);
}

void shop_update(GameState *s, const Input *inp) {
    static bool pu = false, pd = false;
    bool prev_u = pu, prev_d = pd;
    pu = inp->up; pd = inp->down;

    if (inp->up && !prev_u && s->menu_cursor > 0)
        s->menu_cursor--;
    if (inp->down && !prev_d &&
        s->menu_cursor < (uint8_t)(SHOP_ITEM_COUNT - 1))
        s->menu_cursor++;
    if (inp->a_press) buy_selected(s);
    if (inp->b_press) s->mode = MODE_TOPDOWN;
}

void shop_render(const GameState *s) {
    /* Panel: centred 200×160, dark translucent look. */
    int pw = 200, ph = 160;
    int px = (DISPLAY_W - pw) / 2;
    int py = (DISPLAY_H - ph) / 2;
    hal_fill_rect(px, py, pw, ph, C_BG_DARK);
    hal_fill_rect(px,            py,           pw, 1,  C_BORDER_ACT);
    hal_fill_rect(px,            py + ph - 1,  pw, 1,  C_BORDER_ACT);
    hal_fill_rect(px,            py,           1,  ph, C_BORDER_ACT);
    hal_fill_rect(px + pw - 1,   py,           1,  ph, C_BORDER_ACT);

    font_draw_str("Shop", px + 8, py + 6, C_GOLD, 1);

    char coins_line[24];
    snprintf(coins_line, sizeof(coins_line), "Coins: %d",
             inventory_count(&s->inv, ITEM_COIN));
    int cw = font_str_width(coins_line, 1);
    font_draw_str(coins_line, px + pw - cw - 8, py + 6, C_TEXT_WHITE, 1);

    int row_h = 24;
    for (int i = 0; i < SHOP_ITEM_COUNT; i++) {
        const ShopItem *si = &SHOP_ITEMS[i];
        int ry = py + 24 + i * row_h;
        bool sel = (s->menu_cursor == (uint8_t)i);
        uint16_t bdr = sel ? C_BORDER_ACT : C_BORDER;
        hal_fill_rect(px + 6, ry, pw - 12, row_h - 4, sel ? C_PANEL : C_BG);
        hal_fill_rect(px + 6, ry,               pw - 12, 1, bdr);
        hal_fill_rect(px + 6, ry + row_h - 5,   pw - 12, 1, bdr);
        iso_draw_item_icon(si->item, px + 10, ry + 2);
        font_draw_str(ITEM_DEFS[si->item].name, px + 32, ry + 6, C_TEXT_WHITE, 1);

        char p[8];
        snprintf(p, sizeof(p), "%uc", (unsigned)si->price);
        int pwid = font_str_width(p, 1);
        font_draw_str(p, px + pw - pwid - 12, ry + 6, C_GOLD, 1);
    }

    font_draw_str("A: Buy  B: Close", px + 8, py + ph - 14, C_TEXT_DIM, 1);
}
