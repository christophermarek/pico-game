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

static PngAtlas g_tiles, g_chars, g_items;

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

static const PngFrame PNG_DEPLETED = { 0, 144, 64, 48, -32, -24 };

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

static void draw_png_frame_scaled(const PngAtlas *a, const PngFrame *f, int ax, int ay,
                                  int sn, int sd)
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
        int src_y = f->y + (dy * f->h) / out_h;
        for (int dx = 0; dx < out_w; dx++) {
            int screen_x = ox0 + dx;
            if (screen_x < 0 || screen_x >= DISPLAY_W) continue;
            int src_x = f->x + (dx * f->w) / out_w;
            int src = (src_y * a->img.w + src_x) * 4;
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
        return true;
    default:
        return false;
    }
}

static bool tile_is_overlay(uint8_t tile_id)
{
    return tile_id == T_TREE   || tile_id == T_ROCK ||
           tile_id == T_ORE    || tile_id == T_FLOWER ||
           tile_id == T_TGRASS;
}

static uint8_t tile_floor_id(uint8_t tile_id)
{
    if (tile_id == T_ORE) return T_PATH;
    return T_GRASS;
}

static void iso_tile_frame(uint8_t tile_id, uint32_t tick, PngFrame *f)
{
    f->w  = ISO_TILE_W;
    f->h  = ISO_TILE_H;
    f->ox = ISO_TILE_OX;
    f->oy = ISO_TILE_OY;

    int col = 0, row = 0;
    switch (tile_id) {
    case T_WATER:  col = (int)((tick / 15u) % 4u); row = 0; break;
    case T_GRASS:  col = 0; row = 1; break;
    case T_PATH:   col = 1; row = 1; break;
    case T_SAND:   col = 2; row = 1; break;
    case T_TREE:   col = 3; row = 1; break;
    case T_ROCK:   col = 0; row = 2; break;
    case T_ORE:    col = 1; row = 2; break;
    case T_FLOWER: col = 2; row = 2; break;
    case T_TGRASS: col = 3; row = 2; break;
    default:       col = 0; row = 0; break;
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
    case T_TREE:   return -47;
    default:       return -ISO_TILE_H / 2;
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

bool iso_draw_depleted_mark(int cx, int cy)
{
    if (!load_tiles()) return false;
    draw_png_frame(&g_tiles, &PNG_DEPLETED, cx, cy);
    return true;
}

bool iso_draw_td_player_char(int sx, int sy, uint8_t dir, uint8_t walk_frame)
{
    if (!load_chars()) return false;
    uint8_t d = (dir > DIR_RIGHT) ? DIR_DOWN : dir;
    draw_png_frame_scaled(&g_chars, &PNG_TD_PLAYER[d * 2u + (walk_frame & 1u)],
                          sx, sy, CHAR_SCALE_NUM, CHAR_SCALE_DEN);
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
    if (id == ITEM_NONE || id >= ITEM_COUNT) return false;
    if (!load_items()) return false;
    PngFrame f;
    item_frame(id, &f);
    draw_png_frame(&g_items, &f, sx, sy);
    return true;
}

/* Scale num/den (e.g. 2/3 ⇒ 16→~11 px). Used for the tool-in-hand render. */
bool iso_draw_item_icon_scaled(item_id_t id, int sx, int sy, int sn, int sd)
{
    if (id == ITEM_NONE || id >= ITEM_COUNT) return false;
    if (!load_items()) return false;
    PngFrame f;
    item_frame(id, &f);
    draw_png_frame_scaled(&g_items, &f, sx, sy, sn, sd);
    return true;
}
