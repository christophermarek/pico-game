#include "iso_spritesheet.h"
#include "hal.h"
#include "colors.h"
#include "config.h"
#include "../game/items.h"

typedef struct {
    int x, y, w, h;
    int ox, oy;
} PngFrame;

typedef struct {
    HalImageRGBA img;
    bool loaded;
    bool tried;
} PngAtlas;

static PngAtlas g_tiles, g_chars, g_items, g_ui;

#define ISO_TILE_W  64
#define ISO_TILE_H  48
#define ISO_TILE_OX (-ISO_TILE_W / 2)
#define ISO_TILE_OY (-ISO_TILE_H / 3)

/*
 * Character sprites are authored at 2× game pixels; draw them at 1:1 via
 * a half-size nearest-neighbour downsample. Source ox/oy are in source
 * pixels and get halved during the draw.
 */
#define CHAR_SCALE_NUM 1
#define CHAR_SCALE_DEN 2

static const PngFrame PNG_TD_PLAYER[8] = {
    {   0, 0, 32, 32, -16, -14 }, /* DIR_DOWN  walk0 */
    {  32, 0, 32, 32, -16, -14 }, /* DIR_DOWN  walk1 */
    {  64, 0, 32, 32, -16, -14 }, /* DIR_UP    walk0 */
    {  96, 0, 32, 32, -16, -14 }, /* DIR_UP    walk1 */
    { 128, 0, 32, 32, -16, -14 }, /* DIR_LEFT  walk0 */
    { 160, 0, 32, 32, -16, -14 }, /* DIR_LEFT  walk1 */
    { 192, 0, 32, 32, -16, -14 }, /* DIR_RIGHT walk0 */
    { 224, 0, 32, 32, -16, -14 }, /* DIR_RIGHT walk1 */
};

/* NPC sprites live on row 2 of the chars sheet, indexed by NpcKind-1
 * (NPC_NONE=0 has no sprite). Add a column when a new NPC ships. */
static const PngFrame PNG_NPC[] = {
    { 0, 32, 32, 32, -16, -14 }, /* NPC_SHOPKEEPER */
};

static void try_load_atlas(PngAtlas *a, const char *p0, const char *p1)
{
    if (a->tried) return;
    a->tried  = true;
    a->loaded = hal_image_load_rgba(p0, &a->img);
    if (!a->loaded)
        a->loaded = hal_image_load_rgba(p1, &a->img);
}

static bool load_tiles(void)
{
    try_load_atlas(&g_tiles, "assets_iso_tiles.png", "build/assets_iso_tiles.png");
    return g_tiles.loaded;
}

static bool load_chars(void)
{
    try_load_atlas(&g_chars, "assets_chars.png", "build/assets_chars.png");
    return g_chars.loaded;
}

static bool load_items(void)
{
    try_load_atlas(&g_items, "assets_items.png", "build/assets_items.png");
    return g_items.loaded;
}

static bool load_ui(void)
{
    try_load_atlas(&g_ui, "assets_ui.png", "build/assets_ui.png");
    return g_ui.loaded;
}

static void draw_png_frame(const PngAtlas *a, const PngFrame *f, int ax, int ay)
{
    if (!a->loaded || !a->img.rgba) return;
    if (f->x < 0 || f->y < 0 || f->x + f->w > a->img.w || f->y + f->h > a->img.h) return;

    int ox = ax + f->ox;
    int oy = ay + f->oy;
    for (int y = 0; y < f->h; y++) {
        int sy = oy + y;
        if (sy < 0 || sy >= DISPLAY_H) continue;
        for (int x = 0; x < f->w; x++) {
            int sx = ox + x;
            if (sx < 0 || sx >= DISPLAY_W) continue;
            int src = ((f->y + y) * a->img.w + (f->x + x)) * 4;
            if (a->img.rgba[src + 3] < 16) continue;
            hal_pixel(sx, sy, RGB565(a->img.rgba[src], a->img.rgba[src+1], a->img.rgba[src+2]));
        }
    }
}

/*
 * orient selects how the output pixel (dx, dy) maps back to the source
 * frame. For rotations (D/U), the output's w is compared against the
 * source's h (and vice-versa) — this is safe because all rotation-using
 * callers are square. A non-square sprite passed with D/U would read
 * outside the source; we document that limitation on the public API.
 */
static void draw_png_frame_scaled(const PngAtlas *a, const PngFrame *f, int ax, int ay,
                                  int sn, int sd, IconOrient orient)
{
    if (!a->loaded || !a->img.rgba || sn <= 0 || sd <= 0) return;
    if (f->x < 0 || f->y < 0 || f->x + f->w > a->img.w || f->y + f->h > a->img.h) return;

    int out_w = (f->w * sn) / sd;
    int out_h = (f->h * sn) / sd;
    if (out_w < 1 || out_h < 1) return;

    int ox0 = ax + (f->ox * sn) / sd;
    int oy0 = ay + (f->oy * sn) / sd;

    for (int dy = 0; dy < out_h; dy++) {
        int screen_y = oy0 + dy;
        if (screen_y < 0 || screen_y >= DISPLAY_H) continue;
        for (int dx = 0; dx < out_w; dx++) {
            int screen_x = ox0 + dx;
            if (screen_x < 0 || screen_x >= DISPLAY_W) continue;

            int sample_x, sample_y;
            switch (orient) {
                default:
                case ICON_ORIENT_R:
                    sample_x = dx;              sample_y = dy;            break;
                case ICON_ORIENT_L:
                    sample_x = out_w - 1 - dx;  sample_y = dy;            break;
                case ICON_ORIENT_D:              /* 90° CW */
                    sample_x = dy;              sample_y = out_w - 1 - dx; break;
                case ICON_ORIENT_U:              /* 90° CCW */
                    sample_x = out_h - 1 - dy;  sample_y = dx;            break;
            }
            int src_x = f->x + (sample_x * f->w) / out_w;
            int src_y = f->y + (sample_y * f->h) / out_h;
            int src   = (src_y * a->img.w + src_x) * 4;
            if (a->img.rgba[src + 3] < 16) continue;
            hal_pixel(screen_x, screen_y,
                      RGB565(a->img.rgba[src], a->img.rgba[src+1], a->img.rgba[src+2]));
        }
    }
}

static bool iso_tile_id_on_sheet(uint8_t tile_id)
{
    switch (tile_id) {
    case T_GRASS: case T_PATH:  case T_WATER: case T_SAND:
    case T_TREE:  case T_ROCK:  case T_ORE:
    case T_FLOWER: case T_TGRASS:
    case T_ORE_COPPER: case T_ORE_TIN:
    case T_ORE_SILVER: case T_ORE_GOLD:
    case T_TREE_PINE: case T_TREE_MAPLE: case T_TREE_YEW:
    case T_WATER_RIVER: case T_WATER_DEEP: case T_WATER_DARK:
        return true;
    default:
        return false;
    }
}

static bool tile_is_overlay(uint8_t tile_id)
{
    switch (tile_id) {
    case T_TREE:  case T_ROCK: case T_ORE: case T_FLOWER: case T_TGRASS:
    case T_ORE_COPPER: case T_ORE_TIN:
    case T_ORE_SILVER: case T_ORE_GOLD:
    case T_TREE_PINE: case T_TREE_MAPLE: case T_TREE_YEW:
        return true;
    default:
        return false;
    }
}

static uint8_t tile_floor_id(uint8_t tile_id)
{
    switch (tile_id) {
    case T_ORE: case T_ORE_COPPER: case T_ORE_TIN:
    case T_ORE_SILVER: case T_ORE_GOLD:
        return T_PATH;   /* ores sit on dirt / stone, not grass */
    default:
        return T_GRASS;
    }
}

static void iso_tile_frame(uint8_t tile_id, uint32_t tick, PngFrame *f)
{
    f->w  = ISO_TILE_W;
    f->h  = ISO_TILE_H;
    f->ox = ISO_TILE_OX;
    f->oy = ISO_TILE_OY;

    int col = 0, row = 0;
    switch (tile_id) {
    case T_WATER:        col = (int)((tick / 15u) % 4u); row = 0; break;
    case T_GRASS:        col = 0; row = 1; break;
    case T_PATH:         col = 1; row = 1; break;
    case T_SAND:         col = 2; row = 1; break;
    case T_TREE:         col = 3; row = 1; break;
    case T_ROCK:         col = 0; row = 2; break;
    case T_ORE:          col = 1; row = 2; break;
    case T_FLOWER:       col = 2; row = 2; break;
    case T_TGRASS:       col = 3; row = 2; break;
    case T_ORE_COPPER:   col = 0; row = 3; break;
    case T_ORE_TIN:      col = 1; row = 3; break;
    case T_ORE_SILVER:   col = 2; row = 3; break;
    case T_ORE_GOLD:     col = 3; row = 3; break;
    case T_TREE_PINE:    col = 0; row = 4; break;
    case T_TREE_MAPLE:   col = 1; row = 4; break;
    case T_TREE_YEW:     col = 2; row = 4; break;
    case T_WATER_RIVER:  col = 0; row = 5; break;
    case T_WATER_DEEP:   col = 1; row = 5; break;
    case T_WATER_DARK:   col = 2; row = 5; break;
    default:             col = 0; row = 0; break;
    }
    f->x = col * ISO_TILE_W;
    f->y = row * ISO_TILE_H;
}

/*
 * Overlay Y-offset per tile.
 *   Trees: content extends to the sprite bottom → base-anchor at sprite foot.
 *   Everything else: content is centred within the cell, so we centre on
 *     the tile anchor (the floor-tile oy=-16 would push them into the
 *     diamond's lower half because their content is centred around row 24).
 */
static int overlay_oy(uint8_t tile_id)
{
    switch (tile_id) {
    case T_TREE: case T_TREE_PINE: case T_TREE_MAPLE: case T_TREE_YEW:
        return -47;
    default:
        return -ISO_TILE_H / 2;
    }
}

bool iso_draw_tile_full(uint8_t tile_id, int cx, int cy, uint32_t tick)
{
    if (!iso_tile_id_on_sheet(tile_id)) return false;
    if (!load_tiles()) return false;
    uint8_t floor_id = tile_is_overlay(tile_id) ? tile_floor_id(tile_id) : tile_id;
    PngFrame f;
    iso_tile_frame(floor_id, tick, &f);
    draw_png_frame(&g_tiles, &f, cx, cy);
    return true;
}

bool iso_draw_tile_onlay(uint8_t tile_id, int cx, int cy)
{
    if (!tile_is_overlay(tile_id)) return false;
    if (!load_tiles()) return false;
    PngFrame f;
    iso_tile_frame(tile_id, 0, &f);
    f.oy = overlay_oy(tile_id);
    draw_png_frame(&g_tiles, &f, cx, cy);
    return true;
}

bool iso_draw_td_player_char(int sx, int sy, uint8_t dir, uint8_t walk_frame)
{
    if (!load_chars()) return false;
    uint8_t d = (dir > DIR_RIGHT) ? DIR_DOWN : dir;
    draw_png_frame_scaled(&g_chars, &PNG_TD_PLAYER[d * 2u + (walk_frame & 1u)],
                          sx, sy, CHAR_SCALE_NUM, CHAR_SCALE_DEN,
                          ICON_ORIENT_R);
    return true;
}

bool iso_draw_npc(uint8_t npc_kind, int sx, int sy)
{
    if (npc_kind == 0) return false;
    int idx = (int)npc_kind - 1;
    if (idx < 0 || idx >= (int)(sizeof(PNG_NPC) / sizeof(PNG_NPC[0])))
        return false;
    if (!load_chars()) return false;
    draw_png_frame_scaled(&g_chars, &PNG_NPC[idx],
                          sx, sy, CHAR_SCALE_NUM, CHAR_SCALE_DEN,
                          ICON_ORIENT_R);
    return true;
}

/* Structures live on rows 6-7 of the iso-tiles atlas; layout matches
 * the manifest.json grid. STK_NONE = 0 returns false. Order:
 * workbench, shack, chest, campfire, forge, workshop. */
bool iso_draw_structure(uint8_t kind, int cx, int cy)
{
    if (kind == 0) return false;
    int idx = (int)kind - 1;        /* WORKBENCH→0 .. WORKSHOP→5 */
    if (idx < 0 || idx >= 6) return false;
    if (!load_tiles()) return false;
    int col = idx % 4, row = 6 + (idx / 4);
    PngFrame f = {
        .x = col * ISO_TILE_W, .y = row * ISO_TILE_H,
        .w = ISO_TILE_W,       .h = ISO_TILE_H,
        .ox = ISO_TILE_OX,     .oy = -ISO_TILE_H / 2,
    };
    draw_png_frame(&g_tiles, &f, cx, cy);
    return true;
}

#define ITEM_ICON_W 16
#define ITEM_ICON_H 16

static void item_frame(item_id_t id, PngFrame *f)
{
    f->x  = (int)(id - 1) * ITEM_ICON_W;
    f->y  = 0;
    f->w  = ITEM_ICON_W;
    f->h  = ITEM_ICON_H;
    f->ox = 0;
    f->oy = 0;
}

bool iso_draw_item_icon(item_id_t id, int sx, int sy)
{
    return iso_draw_item_icon_scaled(id, sx, sy, 1, 1, ICON_ORIENT_R);
}

/* sn/sd = scale (e.g. 2/3 ⇒ 16→~11 px). orient picks which of the four
 * right/left/down/up presentations the atlas-right-facing source maps to.
 * Items are 16×16 so rotation stays in-bounds. */
bool iso_draw_item_icon_scaled(item_id_t id, int sx, int sy,
                               int sn, int sd, IconOrient orient)
{
    if (id == ITEM_NONE || id >= ITEM_COUNT) return false;
    if (!load_items()) return false;
    PngFrame f;
    item_frame(id, &f);
    draw_png_frame_scaled(&g_items, &f, sx, sy, sn, sd, orient);
    return true;
}

#define UI_ICON_W 8
#define UI_ICON_H 8

bool iso_draw_ui_icon(UiIcon icon, int sx, int sy)
{
    if (icon < 0 || icon >= UI_ICON_COUNT) return false;
    if (!load_ui()) return false;
    PngFrame f = {
        .x  = (int)icon * UI_ICON_W,
        .y  = 0,
        .w  = UI_ICON_W,
        .h  = UI_ICON_H,
        .ox = 0,
        .oy = 0,
    };
    draw_png_frame(&g_ui, &f, sx, sy);
    return true;
}
