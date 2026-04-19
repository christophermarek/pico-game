#include "hud.h"
#include "hal.h"
#include "colors.h"
#include "config.h"
#include "../render/font.h"
#include "../data/species.h"

/* ------------------------------------------------------------------ */
/* Small helper to draw a horizontal bar                               */
/* ------------------------------------------------------------------ */
static void draw_bar(int x, int y, int w, int h,
                     int cur, int max, uint16_t color) {
    hal_fill_rect(x, y, w, h, C_BG_DARK);
    if (max > 0) {
        int fill = cur * w / max;
        if (fill > w) fill = w;
        if (fill > 0)
            hal_fill_rect(x, y, fill, h, color);
    }
}

/* ------------------------------------------------------------------ */
/* Draw a short number string without printf                           */
/* ------------------------------------------------------------------ */
static void draw_int(int x, int y, int val, uint16_t color, int scale) {
    char buf[8];
    int n = 0;
    if (val < 0) { buf[n++] = '-'; val = -val; }
    if (val == 0) { buf[n++] = '0'; }
    else {
        /* reverse digits */
        char tmp[7]; int tn = 0;
        while (val > 0) { tmp[tn++] = (char)('0' + val % 10); val /= 10; }
        for (int i = tn - 1; i >= 0; i--) buf[n++] = tmp[i];
    }
    buf[n] = '\0';
    font_draw_str(buf, x, y, color, scale);
}

/* ------------------------------------------------------------------ */
/* HUD strip at the bottom of the screen                               */
/* ------------------------------------------------------------------ */
void hud_draw(GameState *s) {
    int strip_y = DISPLAY_H - 28;
    int strip_h = 28;

    /* Dark background strip */
    hal_fill_rect(0, strip_y, DISPLAY_W, strip_h, C_BG_DARK);
    /* Border line */
    hal_fill_rect(0, strip_y, DISPLAY_W, 1, C_BORDER);

    /* HP bar (green) */
    font_draw_str("HP", 2, strip_y + 2, C_HP_GREEN, 1);
    draw_bar(16, strip_y + 3, 48, 4, s->hp, s->max_hp, C_HP_GREEN);

    /* Hunger bar (orange) */
    font_draw_str("HN", 2, strip_y + 10, C_HUNGER_ORG, 1);
    draw_bar(16, strip_y + 11, 48, 4, s->hunger, 100, C_HUNGER_ORG);

    /* Energy bar (blue) */
    font_draw_str("EN", 2, strip_y + 18, C_ENERGY_BLUE, 1);
    draw_bar(16, strip_y + 19, 48, 4, s->energy, 100, C_ENERGY_BLUE);

    /* Active pet HP bar (top-left) */
    if (s->party_count > 0 && s->active_pet < s->party_count) {
        const Pet *pet = &s->party[s->active_pet];
        const Species *sp = get_species(pet->species_id);

        /* Name */
        const char *pname = sp->evo_names[pet->evo_stage];
        font_draw_str(pname, 2, 2, C_TEXT_MAIN, 1);

        /* Lv */
        font_draw_str("Lv", 2, 10, C_TEXT_DIM, 1);
        draw_int(14, 10, pet->level, C_TEXT_WHITE, 1);

        /* HP bar */
        draw_bar(2, 20, 60, 3, pet->hp, pet->max_hp, C_HP_GREEN);
    }

    /* Day counter (top-right) */
    font_draw_str("Day", DISPLAY_W - 36, 2, C_TEXT_DIM, 1);
    draw_int(DISPLAY_W - 16, 2, (int)s->day, C_GOLD, 1);

    /* Most recent log line at bottom */
    if (s->log_count > 0) {
        font_draw_str(s->log[0], 70, strip_y + 10, C_TEXT_DIM, 1);
    }
}
