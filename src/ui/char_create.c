#include "char_create.h"
#include "hal.h"
#include "colors.h"
#include "config.h"
#include "../render/font.h"
#include "../render/sprites.h"
#include "../render/iso_spritesheet.h"

/*
 * Character creation state machine:
 *  Section 0: Name entry  (cc_char_pos = char index 0-7)
 *  Section 1: Skin tone
 *  Section 2: Hair colour
 *  Section 3: Outfit colour
 *  Section 4: Confirm / BEGIN
 *
 * Right 60 px of the screen (x=180..239) is a persistent character preview
 * sidebar.  All section content is laid out to stay within x < 178 so it
 * does not bleed into the sidebar.
 */

#define CC_NAME    0
#define CC_SKIN    1
#define CC_HAIR    2
#define CC_OUTFIT  3
#define CC_CONFIRM 4

/* Characters allowed in name: A-Z, 0-9, space */
static const char CHARSET[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 ";
#define CHARSET_LEN 37

static uint8_t cc_section     = CC_NAME;
static uint8_t cc_char_pos    = 0;
static uint8_t cc_char_idx[8];
static bool    cc_init        = false;
static uint8_t cc_preview_dir = DIR_DOWN;

/*
 * Direction rotation lookup for the preview.
 * CW  (cam_r) sequence: DOWN → LEFT → UP → RIGHT → DOWN (clock-face rotation)
 * CCW (cam_l) sequence: DOWN → RIGHT → UP → LEFT → DOWN
 */
static const uint8_t DIR_CW[4]  = { DIR_LEFT,  DIR_RIGHT, DIR_UP,   DIR_DOWN };
static const uint8_t DIR_CCW[4] = { DIR_RIGHT, DIR_LEFT,  DIR_DOWN, DIR_UP   };

static void cc_ensure_init(GameState *s) {
    if (!cc_init) {
        cc_section     = CC_NAME;
        cc_char_pos    = 0;
        cc_preview_dir = DIR_DOWN;
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

static void rebuild_name(GameState *s) {
    for (int i = 0; i < 8; i++)
        s->custom.name[i] = CHARSET[cc_char_idx[i]];
    s->custom.name[8] = '\0';
    for (int i = 7; i >= 0 && s->custom.name[i] == ' '; i--)
        s->custom.name[i] = '\0';
    if (s->custom.name[0] == '\0') {
        s->custom.name[0] = 'H'; s->custom.name[1] = 'e';
        s->custom.name[2] = 'r'; s->custom.name[3] = 'o';
        s->custom.name[4] = '\0';
    }
}

/* ------------------------------------------------------------------ */
/* Update                                                               */
/* ------------------------------------------------------------------ */
void char_create_update(GameState *s, const Input *inp) {
    cc_ensure_init(s);

    static bool up_prev=false, dn_prev=false, lt_prev=false, rt_prev=false;
    bool up = inp->up    && !up_prev;
    bool dn = inp->down  && !dn_prev;
    bool lt = inp->left  && !lt_prev;
    bool rt = inp->right && !rt_prev;
    bool a  = inp->a_press;
    bool b  = inp->b_press;
    up_prev = inp->up; dn_prev = inp->down;
    lt_prev = inp->left; rt_prev = inp->right;

    /* Camera shoulder buttons ([/]) rotate the preview from any section. */
    if (inp->cam_l_press) cc_preview_dir = DIR_CCW[cc_preview_dir];
    if (inp->cam_r_press) cc_preview_dir = DIR_CW[cc_preview_dir];

    switch (cc_section) {

    case CC_NAME:
        if (up) cc_char_idx[cc_char_pos] =
                    (uint8_t)((cc_char_idx[cc_char_pos] + 1) % CHARSET_LEN);
        if (dn) cc_char_idx[cc_char_pos] =
                    (uint8_t)((cc_char_idx[cc_char_pos] + CHARSET_LEN - 1) % CHARSET_LEN);
        if (rt && cc_char_pos < 7) cc_char_pos++;
        if (lt && cc_char_pos > 0) cc_char_pos--;
        if (a) { rebuild_name(s); cc_section = CC_SKIN; }
        break;

    case CC_SKIN:
        if (rt && s->custom.skin_idx < 3)  s->custom.skin_idx++;
        if (lt && s->custom.skin_idx > 0)  s->custom.skin_idx--;
        if (a) cc_section = CC_HAIR;
        if (b) cc_section = CC_NAME;
        break;

    case CC_HAIR:
        if (rt && s->custom.hair_idx < 5)  s->custom.hair_idx++;
        if (lt && s->custom.hair_idx > 0)  s->custom.hair_idx--;
        if (a) cc_section = CC_OUTFIT;
        if (b) cc_section = CC_SKIN;
        break;

    case CC_OUTFIT:
        if (rt && s->custom.outfit_idx < 5) s->custom.outfit_idx++;
        if (lt && s->custom.outfit_idx > 0) s->custom.outfit_idx--;
        if (a) cc_section = CC_CONFIRM;
        if (b) cc_section = CC_HAIR;
        break;

    case CC_CONFIRM:
        /* Left/right also rotate the preview on the confirm screen. */
        if (lt) cc_preview_dir = DIR_CCW[cc_preview_dir];
        if (rt) cc_preview_dir = DIR_CW[cc_preview_dir];
        if (b) cc_section = CC_OUTFIT;
        if (a) {
            rebuild_name(s);
            s->mode = MODE_TOPDOWN;
            cc_init = false;
        }
        break;
    }
}

/* ------------------------------------------------------------------ */
/* Render                                                               */
/* ------------------------------------------------------------------ */

/* Right-side preview sidebar geometry */
#define SIDEBAR_X   178   /* divider x-position              */
#define SIDEBAR_CX  209   /* centre-x of sidebar (178+31)    */
#define SIDEBAR_W    61   /* 239-178                          */

/* Direction labels (4-char max) and their x offset from SIDEBAR_CX
 * to visually centre them between the rotation arrows. */
static const char *const DIR_LABELS[4] = { "DOWN", "UP", "LEFT", "RGHT" };
static const int         DIR_LBL_OX[4] = {  -12,   -6,   -12,   -12  };

void char_create_render(GameState *s) {
    cc_ensure_init(s);

    hal_fill(C_BG);

    /* Title */
    font_draw_str("GRUMBLEQUEST", 30, 6, C_TEXT_MAIN, 2);
    font_draw_str("Character Creation", 35, 26, C_TEXT_DIM, 1);

    /* Section highlight bar (full-width; sidebar will paint over right side) */
    static const int SECT_Y[5] = { 40, 80, 115, 150, 185 };
    hal_fill_rect(0, SECT_Y[cc_section], DISPLAY_W, 32, C_PANEL);
    hal_fill_rect(0, SECT_Y[cc_section], DISPLAY_W, 1,  C_BORDER_ACT);

    /* ---- Name entry ----
     * 8 slots, 14 px wide, 16 px step, starting x=44; last slot ends at x=170. */
    {
        int y = SECT_Y[CC_NAME];
        font_draw_str("Name:", 4, y + 2, C_TEXT_DIM, 1);
        for (int i = 0; i < 8; i++) {
            int      bx = 44 + i * 16;
            uint16_t bg = (cc_section == CC_NAME && cc_char_pos == i)
                          ? C_BORDER_ACT : C_BORDER;
            hal_fill_rect(bx,      y + 1, 14, 18, C_BG_DARK);
            hal_fill_rect(bx,      y + 1, 14,  1, bg);
            hal_fill_rect(bx,      y + 18, 14, 1, bg);
            char ch[2] = { CHARSET[cc_char_idx[i]], '\0' };
            font_draw_str(ch, bx + 4, y + 6, C_TEXT_WHITE, 1);
        }
        if (cc_section == CC_NAME)
            font_draw_str("UP/DN=char LR=move A=ok", 4, y + 22, C_TEXT_DIM, 1);
    }

    /* ---- Skin tone ----
     * 4 swatches, 24 px wide, 30 px step; last swatch ends at x=164. */
    {
        int y = SECT_Y[CC_SKIN];
        font_draw_str("Skin:", 4, y + 4, C_TEXT_DIM, 1);
        for (int i = 0; i < 4; i++) {
            int bx = 50 + i * 30;
            hal_fill_rect(bx, y + 2, 24, 24, C_SKIN_TONES[i]);
            if (s->custom.skin_idx == i) {
                hal_fill_rect(bx - 1, y + 1,  26,  1, C_BORDER_ACT);
                hal_fill_rect(bx - 1, y + 26, 26,  1, C_BORDER_ACT);
                hal_fill_rect(bx - 1, y + 1,   1, 26, C_BORDER_ACT);
                hal_fill_rect(bx + 24, y + 1,  1, 26, C_BORDER_ACT);
            }
        }
    }

    /* ---- Hair colour ----
     * 6 swatches, 18 px wide, 21 px step; last swatch ends at x=173. */
    {
        int y = SECT_Y[CC_HAIR];
        font_draw_str("Hair:", 4, y + 4, C_TEXT_DIM, 1);
        for (int i = 0; i < 6; i++) {
            int bx = 50 + i * 21;
            hal_fill_rect(bx, y + 2, 18, 20, C_HAIR_COLS[i]);
            if (s->custom.hair_idx == i) {
                hal_fill_rect(bx - 1, y + 1,  20,  1, C_BORDER_ACT);
                hal_fill_rect(bx - 1, y + 22, 20,  1, C_BORDER_ACT);
                hal_fill_rect(bx - 1, y + 1,   1, 22, C_BORDER_ACT);
                hal_fill_rect(bx + 18, y + 1,  1, 22, C_BORDER_ACT);
            }
        }
    }

    /* ---- Outfit colour ----
     * 6 swatches, 18 px wide, 21 px step; last swatch ends at x=173. */
    {
        int y = SECT_Y[CC_OUTFIT];
        font_draw_str("Outfit:", 4, y + 4, C_TEXT_DIM, 1);
        for (int i = 0; i < 6; i++) {
            int bx = 50 + i * 21;
            hal_fill_rect(bx, y + 2, 18, 20, C_OUTFIT_COLS[i]);
            if (s->custom.outfit_idx == i) {
                hal_fill_rect(bx - 1, y + 1,  20,  1, C_BORDER_ACT);
                hal_fill_rect(bx - 1, y + 22, 20,  1, C_BORDER_ACT);
                hal_fill_rect(bx - 1, y + 1,   1, 22, C_BORDER_ACT);
                hal_fill_rect(bx + 18, y + 1,  1, 22, C_BORDER_ACT);
            }
        }
    }

    /* ---- Confirm / BEGIN ---- */
    {
        int y = SECT_Y[CC_CONFIRM];

        /* Name display on the left */
        font_draw_str(s->custom.name, 4, y + 4, C_TEXT_DIM, 1);

        /* BEGIN button (constrained to left portion, x=4..171) */
        uint16_t btn_col = (cc_section == CC_CONFIRM) ? C_BORDER_ACT : C_BORDER;
        hal_fill_rect(4,   y + 14, 168, 16, C_PANEL);
        hal_fill_rect(4,   y + 14, 168,  1, btn_col);
        hal_fill_rect(4,   y + 29, 168,  1, btn_col);
        font_draw_str("BEGIN ADVENTURE", 22, y + 18,
                      (cc_section == CC_CONFIRM) ? C_TEXT_WHITE : C_TEXT_DIM, 1);
    }

    /* ---- Right sidebar — persistent character preview ---- */
    {
        /* Vertical divider */
        hal_fill_rect(SIDEBAR_X, 40, 1, 190, C_BORDER);

        /* "PREVIEW" label */
        font_draw_str("CHAR", SIDEBAR_CX - 12, 43, C_TEXT_DIM, 1);

        /* Character at 3× game pixels (48×48 px output).
         * Atlas stores at 2× (32×32); scale sn=3,sd=2 → 48 px. */
        int char_x = SIDEBAR_CX - 24;  /* left edge = 185 */
        int char_y = 56;
        if (!iso_draw_td_player_char_scaled(char_x, char_y,
                                            cc_preview_dir, 0, 3, 2)) {
            /* procedural fallback */
            spr_player_td(char_x, char_y, cc_preview_dir, 0.0f,
                          s->custom.skin_idx, s->custom.hair_idx,
                          s->custom.outfit_idx);
        }

        /* Rotation arrows flanking the direction label */
        int lbl_x = SIDEBAR_CX + DIR_LBL_OX[cc_preview_dir];
        font_draw_str("<",                    181, 110, C_TEXT_DIM, 1);
        font_draw_str(DIR_LABELS[cc_preview_dir], lbl_x, 110, C_TEXT_WHITE, 1);
        font_draw_str(">",                    229, 110, C_TEXT_DIM, 1);

        /* Rotation hint */
        font_draw_str("[/]turn", SIDEBAR_CX - 21, 121, C_TEXT_DIM, 1);
    }

    /* Bottom hint — show LR=turn hint only on confirm screen where LR is active */
    if (cc_section == CC_CONFIRM)
        font_draw_str("A=begin B=back LR=turn", 16, DISPLAY_H - 10, C_TEXT_DIM, 1);
    else
        font_draw_str("A=confirm  B=back", 40, DISPLAY_H - 10, C_TEXT_DIM, 1);
}
