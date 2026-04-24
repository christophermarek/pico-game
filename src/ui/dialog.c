#include "dialog.h"
#include "shop.h"
#include "hal.h"
#include "colors.h"
#include "config.h"
#include "../render/font.h"
#include <string.h>

/* File-local — no need to survive saves. Repopulated on every dialog_open. */
static const Npc *g_talking_to = NULL;

void dialog_open(GameState *s, const Npc *npc) {
    g_talking_to = npc;
    s->mode = MODE_DIALOG;
}

void dialog_update(GameState *s, const Input *inp) {
    if (inp->a_press) {
        /* Shopkeepers jump from their greeting straight into the shop
         * menu. Other NPCs (none yet) would just close the dialog. */
        if (g_talking_to && g_talking_to->kind == NPC_SHOPKEEPER) {
            shop_open(s);
        } else {
            s->mode = MODE_TOPDOWN;
        }
    }
    if (inp->b_press) s->mode = MODE_TOPDOWN;
}

void dialog_render(const GameState *s) {
    (void)s;
    if (!g_talking_to) return;

    /* Bottom box: 224×56, one line of name + wrapped text. */
    int bw = 224, bh = 56;
    int bx = (DISPLAY_W - bw) / 2;
    int by = DISPLAY_H - bh - 6;

    hal_fill_rect(bx,          by,          bw, bh, C_BG_DARK);
    hal_fill_rect(bx,          by,          bw, 1,  C_BORDER_ACT);
    hal_fill_rect(bx,          by + bh - 1, bw, 1,  C_BORDER_ACT);
    hal_fill_rect(bx,          by,          1,  bh, C_BORDER_ACT);
    hal_fill_rect(bx + bw - 1, by,          1,  bh, C_BORDER_ACT);

    font_draw_str(g_talking_to->name, bx + 8, by + 6, C_GOLD, 1);
    font_draw_str(g_talking_to->greeting, bx + 8, by + 22, C_TEXT_WHITE, 1);
    font_draw_str("A: continue  B: cancel", bx + 8, by + bh - 12, C_TEXT_DIM, 1);
}
