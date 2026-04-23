#include "hud.h"
#include "hal.h"
#include "colors.h"
#include "config.h"
#include "../render/font.h"
#include <stdio.h>

/* ------------------------------------------------------------------ */
/* Layout constants (base values live in config.h)                     */
/* ------------------------------------------------------------------ */
#define STRIP_H      HUD_STRIP_H
#define STRIP_Y      (DISPLAY_H - STRIP_H)

/* Three stat bars, side by side, no text labels */
#define BAR_X0       2
#define BAR_W        18
#define BAR_H        10
#define BAR_Y        (STRIP_Y + 4)   /* centred vertically in the strip */
#define BAR_GAP      2               /* pixels between bars */

/* Message / idle-day text starts here */
#define MSG_X        (BAR_X0 + 3 * (BAR_W + BAR_GAP) + 4)
#define MSG_Y        (STRIP_Y + 5)   /* vertically centred: (18-8)/2 */

#define MSG_TIMEOUT_MS  HUD_LOG_TIMEOUT_MS

/* Tab shortcut icons on the right */
#define ICON_W       13
#define ICON_H       16              /* strip is 18 px; 1 px margin top+bottom */
#define ICON_Y       (STRIP_Y + 1)
#define ICON_GAP     1
#define ICON_COUNT   3
/* Total icon block width: ICON_COUNT * ICON_W + (ICON_COUNT-1) * ICON_GAP */
#define ICON_BLOCK_W (ICON_COUNT * ICON_W + (ICON_COUNT - 1) * ICON_GAP)
#define ICON_X0      (DISPLAY_W - ICON_BLOCK_W - 2)

/* Room for message text: from MSG_X to start of icons */
#define MSG_MAX_X    (ICON_X0 - 2)

/* ------------------------------------------------------------------ */
/* Tab icon definitions                                                 */
/* Order matches the user-visible toolbar (bag first per UX request)   */
/* ------------------------------------------------------------------ */
static const struct {
    uint8_t     mtab;
    const char *label;
    uint16_t    color;
} ICONS[ICON_COUNT] = {
    { MTAB_ITEMS,    "BG", C_GOLD        },
    { MTAB_SKILLS,   "SK", C_ENERGY_BLUE },
    { MTAB_SETTINGS, "ST", C_WARN_RED    },
};

/* ------------------------------------------------------------------ */
/* Helpers                                                              */
/* ------------------------------------------------------------------ */
static void draw_bar(int x, int y, int w, int h,
                     int cur, int max, uint16_t color) {
    hal_fill_rect(x, y, w, h, C_BG_DARK);
    if (max > 0) {
        int fill = cur * w / max;
        if (fill > w) fill = w;
        if (fill > 0) hal_fill_rect(x, y, fill, h, color);
    }
}

/* ------------------------------------------------------------------ */
/* Clipped string draw — truncates to fit within max_x pixels          */
/* Builds a null-terminated copy limited by available width.           */
/* ------------------------------------------------------------------ */
static void draw_str_clipped(const char *str, int x, int y,
                              uint16_t color, int max_x)
{
    int avail = max_x - x;
    if (avail <= 0) return;

    /* Find how many chars fit (font_str_width measures whole strings,
     * so binary-search by building a prefix).  A simple linear scan is
     * fine since messages are short (< 40 chars). */
    char buf[40];
    int  n = 0;
    for (; str[n] && n < 38; n++) {
        buf[n] = str[n]; buf[n + 1] = '\0';
        if (font_str_width(buf, 1) > avail) {
            buf[n] = '\0'; /* revert — previous length was the limit */
            break;
        }
    }
    font_draw_str(buf, x, y, color, 1);
}

/* ------------------------------------------------------------------ */
/* Four tab shortcut icons (right side of strip)                        */
/* ------------------------------------------------------------------ */
static void draw_tab_icons(const GameState *s) {
    for (int i = 0; i < ICON_COUNT; i++) {
        int      ix     = ICON_X0 + i * (ICON_W + ICON_GAP);
        bool     active = ((uint8_t)s->menu_tab == ICONS[i].mtab);
        uint16_t acc    = ICONS[i].color;

        /* Background */
        hal_fill_rect(ix, ICON_Y, ICON_W, ICON_H,
                      active ? acc : C_BG_DARK);

        /* 1-pixel border in accent colour */
        hal_fill_rect(ix,           ICON_Y,            ICON_W, 1,      acc);
        hal_fill_rect(ix,           ICON_Y + ICON_H-1, ICON_W, 1,      acc);
        hal_fill_rect(ix,           ICON_Y,            1,      ICON_H, acc);
        hal_fill_rect(ix + ICON_W-1, ICON_Y,           1,      ICON_H, acc);

        /* 2-char label centred: 12px wide in 13px box → 1px from left edge */
        uint16_t fg = active ? C_TEXT_WHITE : acc;
        font_draw_str(ICONS[i].label, ix + 1, ICON_Y + 4, fg, 1);
    }
}

/* ------------------------------------------------------------------ */
/* Main HUD draw                                                        */
/* ------------------------------------------------------------------ */
void hud_draw(GameState *s) {
    /* === Bottom toolbar strip === */
    hal_fill_rect(0, STRIP_Y,     DISPLAY_W, STRIP_H, C_BG_DARK);
    hal_fill_rect(0, STRIP_Y,     DISPLAY_W, 1,       C_BORDER);    /* top border */

    /* Stat bars: HP (green) | Energy (blue) */
    draw_bar(BAR_X0,                       BAR_Y, BAR_W, BAR_H,
             s->hp,     s->max_hp,         C_HP_GREEN);
    draw_bar(BAR_X0 + BAR_W + BAR_GAP,    BAR_Y, BAR_W, BAR_H,
             s->energy, 100,               C_ENERGY_BLUE);

    /* Message (3-second timeout) — falls back to "Day X" when idle */
    uint32_t now = hal_ticks_ms();
    if (s->log_count > 0 && s->log_ms > 0 &&
        now - s->log_ms < MSG_TIMEOUT_MS) {
        draw_str_clipped(s->log[0], MSG_X, MSG_Y, C_TEXT_DIM, MSG_MAX_X);
    } else {
        /* Idle: show current day */
        char buf[12]; int n = 0;
        const char *pre = "Day ";
        while (*pre) buf[n++] = *pre++;
        uint16_t d = s->day;
        if (d >= 100) buf[n++] = (char)('0' + d / 100);
        if (d >=  10) buf[n++] = (char)('0' + (d / 10) % 10);
        buf[n++] = (char)('0' + d % 10);
        buf[n] = '\0';
        font_draw_str(buf, MSG_X, MSG_Y, C_TEXT_DIM, 1);
    }

    /* Tile coords (always) + orientation detail (debug mode only) */
    {
        static const char DIR_CH[4] = { 'D', 'U', 'L', 'R' };
        char coords[28];
        if (s->debug_mode) {
            snprintf(coords, sizeof(coords), "%d,%d s%c w%c c%d",
                     (int)s->td.tile_x, (int)s->td.tile_y,
                     DIR_CH[s->td.screen_dir & 3],
                     DIR_CH[s->td.dir       & 3],
                     (int)s->td_cam_bearing);
        } else {
            snprintf(coords, sizeof(coords), "%d,%d",
                     (int)s->td.tile_x, (int)s->td.tile_y);
        }
        int cw = font_str_width(coords, 1);
        font_draw_str(coords, ICON_X0 - cw - 3, MSG_Y, C_TEXT_DIM, 1);
    }

    /* Tab shortcut icons */
    draw_tab_icons(s);
}
