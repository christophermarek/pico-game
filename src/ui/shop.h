#pragma once
#include "../game/state.h"
#include "hal.h"

/* Shopkeeper inventory is a static table — every row sells one item at
 * one fixed price. No random stock, no tiers beyond bronze; higher
 * tiers of tools are crafted by the player at workbenches. */
typedef struct {
    item_id_t item;
    uint16_t  price;    /* coins */
} ShopItem;

extern const ShopItem SHOP_ITEMS[];
extern const int      SHOP_ITEM_COUNT;

void shop_open(GameState *s);
void shop_update(GameState *s, const Input *inp);
void shop_render(const GameState *s);
