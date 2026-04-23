#include "hud.h"
#include "hal.h"
#include "colors.h"
#include "config.h"
#include "../render/font.h"
#include <stdio.h>

#define STRIP_H      HUD_STRIP_H
#define STRIP_Y      (DISPLAY_H - STRIP_H)

#define BAR_X0       2
#define BAR_W        18
#define BAR_H        10
#define BAR_Y        (STRIP_Y + 4)
#define BAR_GAP      2

#define MSG_X        (BAR_X0 + 3 * (BAR_W + BAR_GAP) + 4)
#define MSG_Y        (STRIP_Y + 5)

#define ICON_W       13
#define ICON_H       16
#define ICON_Y       (STRIP_Y + 1)
#define ICON_GAP     1
#define ICON_COUNT   3
#define ICON_BLOCK_W (ICON_COUNT * ICON_W + (ICON_COUNT - 1) * ICON_GAP)
#define ICON_X0      (DISPLAY_W - ICON_BLOCK_W - 2)

#define MSG_MAX_X    (ICON_X0 - 2)

static const struct {
    uint8_t     mtab;
    const char *label;
    uint16_t    color;
} ICONS[ICON_COUNT] = {
    { MTAB_ITEMS,    "BG", C_GOLD        },
    { MTAB_SKILLS,   "SK", C_ENERGY_BLUE },
    { MTAB_SETTINGS, "ST", C_WARN_RED    },
};

static void draw_bar(int x, int y, int w, int h,
                     int cur, int max, uint16_t color) {
    hal_fill_rect(x, y, w, h, C_BG_DARK);
    if (max > 0) {
        int fill = cur * w / max;
        if (fill > w) fill = w;
        if (fill > 0) hal_fill_rect(x, y, fill, h, color);
    }
}

/* Render str truncated to whatever fits within max_x; font_str_width only
 * measures whole strings, so a linear scan is fine for short log messages. */
static void draw_str_clipped(const char *str, int x, int y,
                             uint16_t color, int max_x) {
    int avail = max_x - x;
    if (avail <= 0) return;

    char buf[40];
    int  n = 0;
    for (; str[n] && n < 38; n++) {
        buf[n]     = str[n];
        buf[n + 1] = '\0';
        if (font_str_width(buf, 1) > avail) {
            buf[n] = '\0';
            break;
        }
    }
    font_draw_str(buf, x, y, color, 1);
}

static void draw_tab_icons(const GameState *s) {
    for (int i = 0; i < ICON_COUNT; i++) {
        int      ix     = ICON_X0 + i * (ICON_W + ICON_GAP);
        bool     active = ((uint8_t)s->menu_tab == ICONS[i].mtab);
        uint16_t acc    = ICONS[i].color;

        hal_fill_rect(ix, ICON_Y, ICON_W, ICON_H, active ? acc : C_BG_DARK);
        hal_fill_rect(ix,             ICON_Y,              ICON_W, 1,      acc);
        hal_fill_rect(ix,             ICON_Y + ICON_H - 1, ICON_W, 1,      acc);
        hal_fill_rect(ix,             ICON_Y,              1,      ICON_H, acc);
        hal_fill_rect(ix + ICON_W - 1, ICON_Y,             1,      ICON_H, acc);

        uint16_t fg = active ? C_TEXT_WHITE : acc;
        font_draw_str(ICONS[i].label, ix + 1, ICON_Y + 4, fg, 1);
    }
}

void hud_draw(GameState *s) {
    hal_fill_rect(0, STRIP_Y, DISPLAY_W, STRIP_H, C_BG_DARK);
    hal_fill_rect(0, STRIP_Y, DISPLAY_W, 1,       C_BORDER);

    draw_bar(BAR_X0,                    BAR_Y, BAR_W, BAR_H,
             s->hp,     s->max_hp,      C_HP_GREEN);
    draw_bar(BAR_X0 + BAR_W + BAR_GAP, BAR_Y, BAR_W, BAR_H,
             s->energy, STAT_MAX,       C_ENERGY_BLUE);

    uint32_t now = hal_ticks_ms();
    if (s->log_count > 0 && s->log_ms > 0 &&
        now - s->log_ms < HUD_LOG_TIMEOUT_MS) {
        draw_str_clipped(s->log[0], MSG_X, MSG_Y, C_TEXT_DIM, MSG_MAX_X);
    } else {
        char buf[12];
        int  n = 0;
        const char *pre = "Day ";
        while (*pre) buf[n++] = *pre++;
        uint16_t d = s->day;
        if (d >= 100) buf[n++] = (char)('0' + d / 100);
        if (d >=  10) buf[n++] = (char)('0' + (d / 10) % 10);
        buf[n++] = (char)('0' + d % 10);
        buf[n]   = '\0';
        font_draw_str(buf, MSG_X, MSG_Y, C_TEXT_DIM, 1);
    }

    {
        static const char DIR_CH[4] = { 'D', 'U', 'L', 'R' };
        char coords[28];
        if (s->debug_mode) {
            snprintf(coords, sizeof(coords), "%d,%d s%c w%c c%d",
                     (int)s->td.tile_x, (int)s->td.tile_y,
                     DIR_CH[s->td.screen_dir & 3],
                     DIR_CH[s->td.dir        & 3],
                     (int)s->td_cam_bearing);
        } else {
            snprintf(coords, sizeof(coords), "%d,%d",
                     (int)s->td.tile_x, (int)s->td.tile_y);
        }
        int cw = font_str_width(coords, 1);
        font_draw_str(coords, ICON_X0 - cw - 3, MSG_Y, C_TEXT_DIM, 1);
    }

    draw_tab_icons(s);
}
