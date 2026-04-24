#pragma once
/*
 * Craftable recipes. A recipe takes 1–3 ingredient stacks and produces one
 * output item. Each recipe is pinned to a crafting station — workbench,
 * workshop, forge, or campfire. The "bare hands" station lets us bootstrap
 * the first workbench from raw wood/stone before any station exists.
 *
 * Smelt recipes award Smelting XP; everything else is free XP-wise.
 */
#include <stdint.h>
#include <stdbool.h>
#include "items.h"

typedef enum {
    STATION_HAND = 0,   /* no placed station — only used for the workbench build */
    STATION_WORKBENCH,
    STATION_WORKSHOP,
    STATION_FORGE,
    STATION_CAMPFIRE,
    STATION_COUNT,
} CraftStation;

#define RECIPE_MAX_INPUTS 3

typedef struct {
    item_id_t id;
    uint8_t   count;
} RecipeInput;

/* Recipe.places sentinel — "this recipe doesn't build a structure".
 * Matches StructureKind STK_NONE; using 0 keeps recipes.h
 * independent of structures.h and lines up with the default-zero of
 * uninitialised C designated-initialiser fields. */
#define RECIPE_NO_PLACE 0

typedef struct {
    const char   *name;
    CraftStation  station;
    RecipeInput   inputs[RECIPE_MAX_INPUTS]; /* unused entries have id == ITEM_NONE */
    item_id_t     output;
    uint8_t       output_count;
    uint8_t       skill_xp;    /* 0 = no XP; otherwise Smelting XP for smelt rows  */
    uint8_t       craft_ticks; /* frames the craft takes; 0 = default 60           */
    uint8_t       places;      /* StructureKind to place; RECIPE_NO_PLACE for none */
} Recipe;

extern const Recipe RECIPES[];
extern const int    RECIPES_COUNT;

/* Default craft duration in frames when a recipe leaves `craft_ticks` at 0. */
#define RECIPE_DEFAULT_TICKS 60u

/* True if `inv` has every ingredient of `r` in the listed quantity. */
bool recipe_can_craft(const Recipe *r, const Inventory *inv);

/* Deducts ingredients from `inv` and adds the output. Returns false and
 * leaves inventory untouched if ingredients are missing or the output
 * can't fit. */
bool recipe_craft(const Recipe *r, Inventory *inv);

/* Station display name. */
const char *station_name(CraftStation st);
