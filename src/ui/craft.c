#include "craft.h"
#include "hal.h"
#include "colors.h"
#include "config.h"
#include "../render/font.h"
#include "../render/iso_spritesheet.h"
#include "../game/items.h"
#include "../game/skills.h"
#include "../game/structures.h"
#include "../game/world.h"
#include <stdio.h>
#include <string.h>

/* Filter = station the player is currently looking at. STATION_COUNT
 * means "show all" (the general CRAFT tab path); a real station id
 * filters to its rows. */
static CraftStation g_station     = STATION_COUNT;
static uint8_t      g_cursor      = 0;
static int          g_visible[32];   /* indices into RECIPES for the filter */
static int          g_visible_n   = 0;

/* In-flight craft state: -1 idle, else recipe index. */
static int          g_in_flight   = -1;
static uint16_t     g_timer_left  = 0;
static uint16_t     g_timer_total = 0;
/* Pre-computed placement target if this craft places a structure. */
static int          g_place_tx    = -1;
static int          g_place_ty    = -1;

static void refilter(void) {
    g_visible_n = 0;
    for (int i = 0; i < RECIPES_COUNT && g_visible_n < 32; i++) {
        if (g_station == STATION_COUNT || RECIPES[i].station == g_station)
            g_visible[g_visible_n++] = i;
    }
    if (g_cursor >= g_visible_n) g_cursor = 0;
}

void craft_set_station(CraftStation station) {
    g_station = station;
    refilter();
}

bool craft_in_flight(void) { return g_in_flight >= 0; }

/* True if (tx, ty) is in-bounds, walkable terrain, and not already
 * occupied by a structure or the player's foot tile. */
static bool placement_ok(const GameState *s, const World *w, int tx, int ty) {
    if (tx < 0 || tx >= w->w || ty < 0 || ty >= w->h) return false;
    if (!world_walkable(w, tx, ty))                   return false;
    if (structure_at(s, tx, ty) != NULL)              return false;
    if (tx == s->td.tile_x && ty == s->td.tile_y)     return false;
    return true;
}

/* Pick a placement tile in front of the player. Returns false (and
 * leaves out_tx/out_ty unset) if the front tile is unsuitable. */
static bool find_placement_target(const GameState *s, const World *w,
                                  int *out_tx, int *out_ty) {
    int dx = 0, dy = 0;
    switch (s->td.dir) {
    case DIR_UP:    dy = -1; break;
    case DIR_DOWN:  dy =  1; break;
    case DIR_LEFT:  dx = -1; break;
    case DIR_RIGHT: dx =  1; break;
    }
    int tx = s->td.tile_x + dx;
    int ty = s->td.tile_y + dy;
    if (!placement_ok(s, w, tx, ty)) return false;
    *out_tx = tx; *out_ty = ty;
    return true;
}

static void finish_craft(GameState *s) {
    const Recipe *r = &RECIPES[g_in_flight];
    if (!recipe_craft(r, &s->inv)) {
        state_log(s, "Missing ingredients.");
        goto done;
    }
    if (r->skill_xp > 0) skill_add_xp(s, SK_SMELTING, r->skill_xp);

    if (r->places != RECIPE_NO_PLACE && g_place_tx >= 0 && g_place_ty >= 0) {
        if (structure_place(s, (StructureKind)r->places,
                            g_place_tx, g_place_ty)) {
            char buf[40];
            snprintf(buf, sizeof(buf), "Built %s.",
                     structure_name((StructureKind)r->places));
            state_log(s, buf);
        } else {
            /* Should be unreachable thanks to the pre-check, but a
             * defensive fallback: log and accept the lost materials.
             * Beats a phantom-placed structure mid-air. */
            state_log(s, "Placement failed.");
        }
    } else {
        char buf[36];
        snprintf(buf, sizeof(buf), "Crafted %s.", r->name);
        state_log(s, buf);
    }
done:
    g_in_flight   = -1;
    g_timer_left  = 0;
    g_timer_total = 0;
    g_place_tx = g_place_ty = -1;
}

bool craft_tab_update(GameState *s, const World *w, const Input *inp) {
    /* Lazy refilter — first frame after entering with no filter set. */
    if (g_visible_n == 0) refilter();

    static bool pu = false, pd = false;
    bool prev_u = pu, prev_d = pd;
    pu = inp->up; pd = inp->down;

    if (g_in_flight >= 0) {
        if (inp->b_press) {
            state_log(s, "Craft cancelled.");
            g_in_flight = -1;
            g_timer_left = g_timer_total = 0;
            g_place_tx = g_place_ty = -1;
            return true;
        }
        if (g_timer_left > 0) g_timer_left--;
        if (g_timer_left == 0) finish_craft(s);
        return true;
    }

    if (inp->up && !prev_u && g_cursor > 0) g_cursor--;
    if (inp->down && !prev_d && g_cursor + 1 < (uint8_t)g_visible_n) g_cursor++;

    if (inp->a_press && g_visible_n > 0) {
        const Recipe *r = &RECIPES[g_visible[g_cursor]];
        if (!recipe_can_craft(r, &s->inv)) {
            state_log(s, "Missing ingredients.");
            return false;
        }
        /* If this recipe builds a structure, lock in the placement
         * target now — fail early so the player keeps their materials. */
        g_place_tx = g_place_ty = -1;
        if (r->places != RECIPE_NO_PLACE) {
            if (!find_placement_target(s, w, &g_place_tx, &g_place_ty)) {
                state_log(s, "No room to build there.");
                return false;
            }
        }
        g_in_flight   = g_visible[g_cursor];
        g_timer_total = r->craft_ticks ? r->craft_ticks : RECIPE_DEFAULT_TICKS;
        g_timer_left  = g_timer_total;
    }
    return false;
}

static void draw_recipe_row(int x, int y, int w, int row_h,
                            const Recipe *r, const Inventory *inv,
                            bool selected, bool is_in_flight,
                            uint16_t progress_fill_w)
{
    bool craftable = recipe_can_craft(r, inv);
    uint16_t fg  = craftable ? C_TEXT_WHITE : C_TEXT_DIM;
    uint16_t bdr = selected  ? C_BORDER_ACT : C_BORDER;

    hal_fill_rect(x,         y,             w, row_h - 2, selected ? C_PANEL : C_BG);
    hal_fill_rect(x,         y,             w, 1,         bdr);
    hal_fill_rect(x,         y + row_h - 3, w, 1,         bdr);

    if (r->output != ITEM_NONE)
        iso_draw_item_icon(r->output, x + 4, y + 2);
    font_draw_str(r->name, x + 24, y + 6, fg, 1);

    char ing[32] = {0};
    int n = 0;
    for (int i = 0; i < RECIPE_MAX_INPUTS; i++) {
        if (r->inputs[i].id == ITEM_NONE) continue;
        int have = inventory_count(inv, r->inputs[i].id);
        int need = (int)r->inputs[i].count;
        int sz = snprintf(ing + n, sizeof(ing) - n, "%s%d/%d",
                          n ? " " : "", have, need);
        if (sz > 0) n += sz;
        if (n >= (int)sizeof(ing) - 1) break;
    }
    int iw = font_str_width(ing, 1);
    font_draw_str(ing, x + w - iw - 6, y + 6,
                  craftable ? C_HP_GREEN : C_WARN_RED, 1);

    if (is_in_flight && progress_fill_w > 0)
        hal_fill_rect(x + 1, y + row_h - 4, (int)progress_fill_w, 2, C_BORDER_ACT);
}

void craft_tab_render(const GameState *s, int y0, int h) {
    /* Single-line header showing the active station. */
    const char *header = (g_station == STATION_COUNT)
                         ? "All recipes" : station_name(g_station);
    font_draw_str(header, 5, y0, C_GOLD, 1);

    const int row_h    = 20;
    const int list_y0  = y0 + 12;
    const int list_w   = DISPLAY_W - 10;
    const int avail    = h - 12 - 14;     /* leave room for the hint line */
    const int visible_rows = avail / row_h;

    int first = 0;
    if (g_cursor >= visible_rows) first = g_cursor - visible_rows + 1;

    for (int i = 0; i < visible_rows; i++) {
        int idx = first + i;
        if (idx >= g_visible_n) break;
        int rec_idx = g_visible[idx];
        bool is_this_in_flight = (rec_idx == g_in_flight);
        uint16_t bar_w = 0;
        if (is_this_in_flight && g_timer_total > 0) {
            uint16_t done = (uint16_t)(g_timer_total - g_timer_left);
            bar_w = (uint16_t)((int)(list_w - 2) * done / g_timer_total);
        }
        draw_recipe_row(5, list_y0 + i * row_h, list_w, row_h,
                        &RECIPES[rec_idx], &s->inv,
                        (uint8_t)idx == g_cursor,
                        is_this_in_flight, bar_w);
    }

    const char *hint = (g_in_flight >= 0)
                     ? "B: cancel craft"
                     : "A: Craft  B: Close";
    font_draw_str(hint, 5, y0 + h - 12, C_TEXT_DIM, 1);
}
