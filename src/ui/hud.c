#include "hud.h"
#include "hal.h"
#include "colors.h"
#include "config.h"
#include "../render/font.h"
#include "../render/iso_spritesheet.h"
#include "../game/items.h"
#include <stdio.h>

/* ── Helpers ─────────────────────────────────────────────────────────────── */

static void draw_bar(int x, int y, int w, int h,
                     int cur, int max, uint16_t fill_color) {
    /* 1px dark border */
    hal_fill_rect(x - 1, y - 1, w + 2, h + 2, C_BG_DARK);
    /* Empty trough */
    hal_fill_rect(x, y, w, h, HEX(0x111122));
    if (max > 0 && cur > 0) {
        int fill = cur * w / max;
        if (fill > w) fill = w;
        /* Fill */
        hal_fill_rect(x, y, fill, h, fill_color);
        /* 1px highlight at top of fill */
        if (fill > 0 && h > 1)
            hal_fill_rect(x, y, fill, 1, C_WHITE);
    }
}

/* Checkerboard translucency: draw C_BG_DARK on every other pixel in rect. */
static void draw_translucent_bg(int x, int y, int w, int h) {
    for (int row = 0; row < h; row++) {
        for (int col = (row & 1); col < w; col += 2)
            hal_fill_rect(x + col, y + row, 1, 1, C_BG_DARK);
    }
}

/* Draw str right-aligned so its right edge is at x_right. */
static void draw_str_right(const char *str, int x_right, int y, uint16_t color) {
    int w = font_str_width(str, 1);
    font_draw_str(str, x_right - w, y, color, 1);
}

/* ── Stat block (top-left) ────────────────────────────────────────────────── */

static void draw_stats(const GameState *s) {
    int x  = HUD_STAT_X;
    int y  = HUD_STAT_Y;

    iso_draw_ui_icon(UI_ICON_HEART, x, y);
    draw_bar(x + 10, y + 1, HUD_BAR_W, HUD_BAR_H,
             s->hp, s->max_hp, C_HP_GREEN);

    iso_draw_ui_icon(UI_ICON_BOLT, x, y + HUD_BAR_ROW_GAP);
    draw_bar(x + 10, y + HUD_BAR_ROW_GAP + 1, HUD_BAR_W, HUD_BAR_H,
             s->energy, STAT_MAX, C_ENERGY_BLUE);
}

/* ── Log / day (top-center) ───────────────────────────────────────────────── */

static void draw_log(const GameState *s) {
    const char *str;
    char day_buf[12];
    uint32_t now = hal_ticks_ms();

    if (s->log_ms > 0 && now - s->log_ms < HUD_LOG_TIMEOUT_MS) {
        str = s->log_msg;
    } else {
        snprintf(day_buf, sizeof(day_buf), "Day %u", (unsigned)s->day);
        str = day_buf;
    }

    int w = font_str_width(str, 1);
    int x = (DISPLAY_W - w) / 2;
    /* 1px drop shadow */
    font_draw_str(str, x + 1, HUD_STAT_Y + 1, C_BLACK, 1);
    font_draw_str(str, x, HUD_STAT_Y, C_TEXT_WHITE, 1);
}

/* ── Debug coordinates (top-right, debug mode only) ─────────────────────── */

static void draw_top_right(const GameState *s) {
    if (!s->debug_mode) return;
    static const char DIR_CH[4] = { 'D', 'U', 'L', 'R' };
    char coords[28];
    snprintf(coords, sizeof(coords), "%d,%d s%c w%c c%d",
             (int)s->td.tile_x, (int)s->td.tile_y,
             DIR_CH[s->td.screen_dir & 3],
             DIR_CH[s->td.dir        & 3],
             (int)s->td_cam_bearing);
    draw_str_right(coords, DISPLAY_W - 4, HUD_STAT_Y, C_TEXT_DIM);
}

/* ── Hotbar (bottom overlay) ──────────────────────────────────────────────── */

static void draw_hotbar(const GameState *s) {
    int total_w = HOTBAR_SLOTS * HUD_HOTBAR_SLOT_W +
                  (HOTBAR_SLOTS - 1) * HUD_HOTBAR_SLOT_GAP;
    int x0 = (DISPLAY_W - total_w) / 2;
    int y0 = HUD_HOTBAR_Y;
    int sw = HUD_HOTBAR_SLOT_W;

    /* Translucent background behind entire hotbar */
    draw_translucent_bg(x0 - 2, y0 - 2, total_w + 4, sw + 4);

    /* Separator between resource slots (0-3) and tool slots (4-7) */
    int sep_x = x0 + TOOL_SLOT_START * (sw + HUD_HOTBAR_SLOT_GAP) - HUD_HOTBAR_SLOT_GAP;
    hal_fill_rect(sep_x, y0, 1, sw, C_BORDER);

    for (int i = 0; i < HOTBAR_SLOTS; i++) {
        int sx = x0 + i * (sw + HUD_HOTBAR_SLOT_GAP);
        const inv_slot_t *slot = &s->inv.slots[i];
        bool is_tool_slot = (i >= TOOL_SLOT_START);
        bool has_item     = (slot->id != ITEM_NONE && slot->count > 0);

        /* Slot background */
        uint16_t bg = (is_tool_slot && has_item) ? HEX(0x1a1040) : C_BG_DARK;
        hal_fill_rect(sx, y0, sw, sw, bg);

        /* Border — priority: item-land flash > active-slot highlight >
         * filled default > empty dim. */
        bool is_active = (s->active_slot == i);
        uint16_t border = (s->slot_flash[i] > 0) ? C_WHITE
                        : is_active             ? C_BORDER_ACT
                        : has_item              ? C_BORDER
                        :                         HEX(0x1e1e3a);
        hal_fill_rect(sx,          y0,          sw,  1,  border);
        hal_fill_rect(sx,          y0 + sw - 1, sw,  1,  border);
        hal_fill_rect(sx,          y0,          1,   sw, border);
        hal_fill_rect(sx + sw - 1, y0,          1,   sw, border);

        if (!has_item) continue;

        /* Item icon — centered in slot (icons are 16x16, slot is sw x sw). */
        int icon_x = sx + (sw - 16) / 2;
        int icon_y = y0 + (sw - 16) / 2;
        if (!iso_draw_item_icon(slot->id, icon_x, icon_y)) {
            /* Fallback: 2-char text if atlas not loaded yet. */
            const char *name = ITEM_DEFS[slot->id].name;
            char abbr[3] = { name[0], name[1], '\0' };
            int  tw = font_str_width(abbr, 1);
            font_draw_str(abbr, sx + (sw - tw) / 2, y0 + (sw / 2) - 3,
                          is_tool_slot ? C_GOLD : C_TEXT_MAIN, 1);
        }

        /* Count badge (skip for tools and count=1 resources) */
        if (!item_is_tool(&ITEM_DEFS[slot->id]) && slot->count > 1) {
            char cnt[5];
            snprintf(cnt, sizeof(cnt), "%u", (unsigned)slot->count);
            int cw = font_str_width(cnt, 1);
            /* Tiny dark bg for readability */
            hal_fill_rect(sx + sw - cw - 2, y0 + sw - 7, cw + 1, 6, C_BG_DARK);
            font_draw_str(cnt, sx + sw - cw - 2, y0 + sw - 7, C_TEXT_WHITE, 1);
        }
    }
}

/* ── Public ───────────────────────────────────────────────────────────────── */

void hud_draw(GameState *s) {
    draw_stats(s);
    draw_log(s);
    draw_top_right(s);
    draw_hotbar(s);
}
