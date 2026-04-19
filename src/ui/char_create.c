#include "char_create.h"
#include "hal.h"
#include "colors.h"
#include "config.h"
#include "../render/font.h"
#include "../render/sprites.h"

/*
 * Character creation state machine:
 *  Section 0: Name entry  (char_cursor = char index 0-7)
 *  Section 1: Skin tone
 *  Section 2: Hair colour
 *  Section 3: Outfit colour
 *  Section 4: Confirm / BEGIN
 */

#define CC_NAME   0
#define CC_SKIN   1
#define CC_HAIR   2
#define CC_OUTFIT 3
#define CC_CONFIRM 4

/* Characters allowed in name: A-Z, 0-9, space */
static const char CHARSET[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 ";
#define CHARSET_LEN 37

/* Current char_cursor overloaded:
 *   bits 7-4 = section (0-4)
 *   bits 3-0 = position within section
 * But we'll use a simpler separate static to keep sections cleaner.
 */
static uint8_t cc_section  = CC_NAME;
static uint8_t cc_char_pos = 0;     /* cursor within name (0-7) */
static uint8_t cc_char_idx[8];      /* index into CHARSET for each name char */
static bool cc_init = false;

static void cc_ensure_init(GameState *s) {
    if (!cc_init) {
        cc_section  = CC_NAME;
        cc_char_pos = 0;
        /* Map existing name to charset indices */
        for (int i = 0; i < 8; i++) {
            char c = s->custom.name[i];
            cc_char_idx[i] = 0;
            for (int j = 0; j < CHARSET_LEN; j++) {
                if (CHARSET[j] == c) { cc_char_idx[i] = (uint8_t)j; break; }
            }
        }
        cc_init = true;
    }
}

/* Rebuild name from char indices */
static void rebuild_name(GameState *s) {
    for (int i = 0; i < 8; i++) {
        s->custom.name[i] = CHARSET[cc_char_idx[i]];
    }
    s->custom.name[8] = '\0';
    /* Trim trailing spaces */
    for (int i = 7; i >= 0 && s->custom.name[i] == ' '; i--)
        s->custom.name[i] = '\0';
    if (s->custom.name[0] == '\0') {
        s->custom.name[0] = 'H';
        s->custom.name[1] = 'e';
        s->custom.name[2] = 'r';
        s->custom.name[3] = 'o';
        s->custom.name[4] = '\0';
    }
}

/* ------------------------------------------------------------------ */
/* Update                                                               */
/* ------------------------------------------------------------------ */
void char_create_update(GameState *s, const Input *inp) {
    cc_ensure_init(s);

    static bool up_prev=false, dn_prev=false, lt_prev=false, rt_prev=false;
    bool up  = inp->up    && !up_prev;
    bool dn  = inp->down  && !dn_prev;
    bool lt  = inp->left  && !lt_prev;
    bool rt  = inp->right && !rt_prev;
    bool a   = inp->a_press;
    bool b   = inp->b_press;
    up_prev = inp->up; dn_prev = inp->down;
    lt_prev = inp->left; rt_prev = inp->right;

    switch (cc_section) {

    case CC_NAME:
        if (up) cc_char_idx[cc_char_pos] = (uint8_t)((cc_char_idx[cc_char_pos] + 1) % CHARSET_LEN);
        if (dn) cc_char_idx[cc_char_pos] = (uint8_t)((cc_char_idx[cc_char_pos] + CHARSET_LEN - 1) % CHARSET_LEN);
        if (rt) { if (cc_char_pos < 7) cc_char_pos++; }
        if (lt) { if (cc_char_pos > 0) cc_char_pos--; }
        if (a)  { rebuild_name(s); cc_section = CC_SKIN; }
        break;

    case CC_SKIN:
        if (rt && s->custom.skin_idx < 3)  s->custom.skin_idx++;
        if (lt && s->custom.skin_idx > 0)  s->custom.skin_idx--;
        if (a)  cc_section = CC_HAIR;
        if (b)  cc_section = CC_NAME;
        break;

    case CC_HAIR:
        if (rt && s->custom.hair_idx < 5)  s->custom.hair_idx++;
        if (lt && s->custom.hair_idx > 0)  s->custom.hair_idx--;
        if (a)  cc_section = CC_OUTFIT;
        if (b)  cc_section = CC_SKIN;
        break;

    case CC_OUTFIT:
        if (rt && s->custom.outfit_idx < 5) s->custom.outfit_idx++;
        if (lt && s->custom.outfit_idx > 0) s->custom.outfit_idx--;
        if (a)  cc_section = CC_CONFIRM;
        if (b)  cc_section = CC_HAIR;
        break;

    case CC_CONFIRM:
        if (b)  cc_section = CC_OUTFIT;
        if (a) {
            /* Finalise and begin game */
            rebuild_name(s);
            s->mode  = MODE_TOPDOWN;
            cc_init  = false;
        }
        break;
    }
}

/* ------------------------------------------------------------------ */
/* Render                                                               */
/* ------------------------------------------------------------------ */
void char_create_render(GameState *s) {
    cc_ensure_init(s);

    hal_fill(C_BG);

    /* Title */
    font_draw_str("GRUMBLEQUEST", 30, 6, C_TEXT_MAIN, 2);
    font_draw_str("Character Creation", 35, 26, C_TEXT_DIM, 1);

    /* Section highlight bar */
    static const int SECT_Y[5] = {40, 80, 115, 150, 185};
    hal_fill_rect(0, SECT_Y[cc_section], DISPLAY_W, 32, C_PANEL);
    hal_fill_rect(0, SECT_Y[cc_section], DISPLAY_W, 1, C_BORDER_ACT);

    /* ---- Name entry ---- */
    {
        int y = SECT_Y[CC_NAME];
        font_draw_str("Name:", 4, y + 2, C_TEXT_DIM, 1);
        /* Draw 8 character slots */
        for (int i = 0; i < 8; i++) {
            int bx = 50 + i * 22;
            uint16_t bg  = (cc_section == CC_NAME && cc_char_pos == i) ? C_BORDER_ACT : C_BORDER;
            hal_fill_rect(bx, y + 1, 20, 18, C_BG_DARK);
            hal_fill_rect(bx, y + 1, 20, 1,  bg);
            hal_fill_rect(bx, y + 18, 20, 1, bg);

            char ch[2] = { CHARSET[cc_char_idx[i]], '\0' };
            font_draw_str(ch, bx + 7, y + 6, C_TEXT_WHITE, 1);
        }
        /* Arrows hint */
        if (cc_section == CC_NAME)
            font_draw_str("UP/DN=char LR=move A=ok", 4, y + 22, C_TEXT_DIM, 1);
    }

    /* ---- Skin tone ---- */
    {
        int y = SECT_Y[CC_SKIN];
        font_draw_str("Skin:", 4, y + 4, C_TEXT_DIM, 1);
        for (int i = 0; i < 4; i++) {
            int bx = 50 + i * 30;
            hal_fill_rect(bx, y + 2, 24, 24, C_SKIN_TONES[i]);
            if (s->custom.skin_idx == i) {
                hal_fill_rect(bx - 1, y + 1, 26, 1,  C_BORDER_ACT);
                hal_fill_rect(bx - 1, y + 26, 26, 1, C_BORDER_ACT);
                hal_fill_rect(bx - 1, y + 1,  1, 26, C_BORDER_ACT);
                hal_fill_rect(bx + 24, y + 1, 1, 26, C_BORDER_ACT);
            }
        }
        if (cc_section == CC_SKIN) font_draw_str("A=next", 170, y + 10, C_TEXT_DIM, 1);
    }

    /* ---- Hair colour ---- */
    {
        int y = SECT_Y[CC_HAIR];
        font_draw_str("Hair:", 4, y + 4, C_TEXT_DIM, 1);
        for (int i = 0; i < 6; i++) {
            int bx = 50 + i * 24;
            hal_fill_rect(bx, y + 2, 20, 20, C_HAIR_COLS[i]);
            if (s->custom.hair_idx == i) {
                hal_fill_rect(bx - 1, y + 1, 22, 1,  C_BORDER_ACT);
                hal_fill_rect(bx - 1, y + 22, 22, 1, C_BORDER_ACT);
                hal_fill_rect(bx - 1, y + 1,  1, 22, C_BORDER_ACT);
                hal_fill_rect(bx + 20, y + 1, 1, 22, C_BORDER_ACT);
            }
        }
        if (cc_section == CC_HAIR) font_draw_str("A=next", 194, y + 6, C_TEXT_DIM, 1);
    }

    /* ---- Outfit colour ---- */
    {
        int y = SECT_Y[CC_OUTFIT];
        font_draw_str("Outfit:", 4, y + 4, C_TEXT_DIM, 1);
        for (int i = 0; i < 6; i++) {
            int bx = 52 + i * 24;
            hal_fill_rect(bx, y + 2, 20, 20, C_OUTFIT_COLS[i]);
            if (s->custom.outfit_idx == i) {
                hal_fill_rect(bx - 1, y + 1, 22, 1,  C_BORDER_ACT);
                hal_fill_rect(bx - 1, y + 22, 22, 1, C_BORDER_ACT);
                hal_fill_rect(bx - 1, y + 1,  1, 22, C_BORDER_ACT);
                hal_fill_rect(bx + 20, y + 1, 1, 22, C_BORDER_ACT);
            }
        }
        if (cc_section == CC_OUTFIT) font_draw_str("A=next", 194, y + 6, C_TEXT_DIM, 1);
    }

    /* ---- Preview + Confirm ---- */
    {
        int y = SECT_Y[CC_CONFIRM];

        /* Preview player sprite */
        spr_player_td(185, y - 30, DIR_DOWN, 0.0f,
                      s->custom.skin_idx, s->custom.hair_idx, s->custom.outfit_idx);

        /* BEGIN button */
        uint16_t btn_col = (cc_section == CC_CONFIRM) ? C_BORDER_ACT : C_BORDER;
        hal_fill_rect(40, y + 2, 150, 22, C_PANEL);
        hal_fill_rect(40, y + 2, 150, 1, btn_col);
        hal_fill_rect(40, y + 23, 150, 1, btn_col);
        font_draw_str("BEGIN ADVENTURE", 55, y + 8,
                      (cc_section == CC_CONFIRM) ? C_TEXT_WHITE : C_TEXT_DIM, 1);

        /* Name display */
        font_draw_str(s->custom.name, 4, y + 8, C_TEXT_DIM, 1);
    }

    /* Section hints */
    font_draw_str("A=confirm  B=back", 40, DISPLAY_H - 10, C_TEXT_DIM, 1);
}
