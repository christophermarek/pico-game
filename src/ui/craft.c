#include "craft.h"
#include "hal.h"
#include "colors.h"
#include "config.h"
#include "../render/font.h"
#include "../render/iso_spritesheet.h"
#include "../game/items.h"
#include "../game/skills.h"
#include <stdio.h>
#include <string.h>

/* Filter = station currently open. STATION_COUNT means "show all" —
 * dev-mode shortcut. Cursor indexes into the filtered view. */
static CraftStation g_station     = STATION_COUNT;
static uint8_t      g_cursor      = 0;
static int          g_visible[32];   /* indices into RECIPES for the filter */
static int          g_visible_n   = 0;

/* Active craft: -1 idle, else index into RECIPES for the in-progress row. */
static int          g_in_flight   = -1;
static uint16_t     g_timer_left  = 0;
static uint16_t     g_timer_total = 0;

static void refilter(void) {
    g_visible_n = 0;
    for (int i = 0; i < RECIPES_COUNT && g_visible_n < 32; i++) {
        if (g_station == STATION_COUNT || RECIPES[i].station == g_station)
            g_visible[g_visible_n++] = i;
    }
    if (g_cursor >= g_visible_n) g_cursor = 0;
}

void craft_open(GameState *s, CraftStation station) {
    g_station = station;
    g_cursor  = 0;
    g_in_flight   = -1;
    g_timer_left  = 0;
    g_timer_total = 0;
    refilter();
    s->mode = MODE_CRAFT;
}

static void finish_craft(GameState *s) {
    const Recipe *r = &RECIPES[g_in_flight];
    if (recipe_craft(r, &s->inv)) {
        if (r->skill_xp > 0) skill_add_xp(s, SK_SMELTING, r->skill_xp);
        char buf[36];
        snprintf(buf, sizeof(buf), "Crafted %s.", r->name);
        state_log(s, buf);
    } else {
        state_log(s, "Missing ingredients.");
    }
    g_in_flight   = -1;
    g_timer_left  = 0;
    g_timer_total = 0;
}

void craft_update(GameState *s, const Input *inp) {
    static bool pu = false, pd = false;
    bool prev_u = pu, prev_d = pd;
    pu = inp->up; pd = inp->down;

    /* In-flight craft: decrement timer, finish when it hits zero.
     * Input during the craft: B cancels (no ingredient loss since
     * we consume on completion, not on start). */
    if (g_in_flight >= 0) {
        if (inp->b_press) {
            state_log(s, "Craft cancelled.");
            g_in_flight = -1;
            g_timer_left = g_timer_total = 0;
            return;
        }
        if (g_timer_left > 0) g_timer_left--;
        if (g_timer_left == 0) finish_craft(s);
        return;
    }

    /* Idle — normal navigation. */
    if (inp->up && !prev_u && g_cursor > 0) g_cursor--;
    if (inp->down && !prev_d && g_cursor + 1 < (uint8_t)g_visible_n) g_cursor++;

    if (inp->a_press && g_visible_n > 0) {
        const Recipe *r = &RECIPES[g_visible[g_cursor]];
        if (!recipe_can_craft(r, &s->inv)) {
            state_log(s, "Missing ingredients.");
        } else {
            g_in_flight   = g_visible[g_cursor];
            g_timer_total = r->craft_ticks ? r->craft_ticks : RECIPE_DEFAULT_TICKS;
            g_timer_left  = g_timer_total;
        }
    }
    if (inp->b_press) s->mode = MODE_TOPDOWN;
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

    /* Progress bar overlay if this row is crafting. */
    if (is_in_flight && progress_fill_w > 0) {
        hal_fill_rect(x + 1, y + row_h - 4, (int)progress_fill_w, 2, C_BORDER_ACT);
    }
}

void craft_render(const GameState *s) {
    int pw = 224, ph = 180;
    int px = (DISPLAY_W - pw) / 2;
    int py = (DISPLAY_H - ph) / 2;

    hal_fill_rect(px, py, pw, ph, C_BG_DARK);
    hal_fill_rect(px,            py,          pw, 1,  C_BORDER_ACT);
    hal_fill_rect(px,            py + ph - 1, pw, 1,  C_BORDER_ACT);
    hal_fill_rect(px,            py,          1,  ph, C_BORDER_ACT);
    hal_fill_rect(px + pw - 1,   py,          1,  ph, C_BORDER_ACT);

    char header[32];
    if (g_station == STATION_COUNT)
        snprintf(header, sizeof(header), "Crafting (all)");
    else
        snprintf(header, sizeof(header), "%s", station_name(g_station));
    font_draw_str(header, px + 8, py + 6, C_GOLD, 1);

    const int row_h = 20;
    const int list_y0 = py + 22;
    const int list_w  = pw - 12;
    const int visible_rows = (ph - 36) / row_h;
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
        draw_recipe_row(px + 6, list_y0 + i * row_h, list_w, row_h,
                        &RECIPES[rec_idx], &s->inv,
                        (uint8_t)idx == g_cursor,
                        is_this_in_flight, bar_w);
    }

    const char *hint = (g_in_flight >= 0)
                     ? "B: cancel craft"
                     : "A: Craft  B: Close";
    font_draw_str(hint, px + 8, py + ph - 14, C_TEXT_DIM, 1);
}
