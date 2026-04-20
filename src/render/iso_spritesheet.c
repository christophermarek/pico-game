#include "iso_spritesheet.h"
#include "hal.h"
#include "colors.h"
#include "config.h"

typedef struct {
    int x, y, w, h;
    int ox, oy;
} PngFrame;

typedef struct {
    HalImageRGBA img;
    bool loaded;
    bool tried;
} PngAtlas;

static PngAtlas g_actors, g_tiles, g_chars;

#define ISO_ACTOR_SN 3
#define ISO_ACTOR_SD 4

/*
 * PNG atlases (assets/ relative to project root, or ../assets/ from build/).
 *
 *  assets_iso_actors.png  160×32  — iso-foot sprites, 32×32 cells:
 *    Row 0: player ×4 (dir: DOWN/UP/LEFT/RIGHT)
 *
 *  assets_iso_tiles.png   832×48  — iso terrain, 64×48 cells:
 *    Cols 0-3: water (4 animation frames)
 *    Cols 4-11: grass/path/sand/tree/rock/ore/flower/tgrass
 *    Col  12: depleted-node overlay
 *
 *  assets_chars.png       192×96  — character sprites at SPRITE_SCALE=2
 *    Row 2 (y=64, h=32): TD player ×8  (32px cells, cols 0-7)
 */

static const PngFrame PNG_TD_PLAYER[8] = {
    {   0, 64, 32, 32, 0, 0 }, /* DIR_DOWN  walk0 */
    {  32, 64, 32, 32, 0, 0 }, /* DIR_DOWN  walk1 */
    {  64, 64, 32, 32, 0, 0 }, /* DIR_UP    walk0 */
    {  96, 64, 32, 32, 0, 0 }, /* DIR_UP    walk1 */
    { 128, 64, 32, 32, 0, 0 }, /* DIR_LEFT  walk0 */
    { 160, 64, 32, 32, 0, 0 }, /* DIR_LEFT  walk1 */
    { 192, 64, 32, 32, 0, 0 }, /* DIR_RIGHT walk0 */
    { 224, 64, 32, 32, 0, 0 }, /* DIR_RIGHT walk1 */
};

static const PngFrame PNG_DEPLETED = { 768, 0, 64, 48, -32, -24 };

static const PngFrame PNG_PLAYER[4] = {
    {   0,  0, 32, 32, -16, -31 },
    {  32,  0, 32, 32, -16, -31 },
    {  64,  0, 32, 32, -16, -31 },
    {  96,  0, 32, 32, -16, -31 },
};

#define ISO_TILE_W 64
#define ISO_TILE_H 48
#define ISO_TILE_OX (-ISO_TILE_W / 2)
#define ISO_TILE_OY (-ISO_TILE_H / 2)

static void try_load_atlas(PngAtlas *a, const char *p0, const char *p1)
{
    if (a->tried) return;
    a->tried = true;
    a->loaded = hal_image_load_rgba(p0, &a->img);
    if (!a->loaded)
        a->loaded = hal_image_load_rgba(p1, &a->img);
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
            uint8_t alpha = a->img.rgba[src + 3];
            if (alpha < 16) continue;
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
            uint8_t alpha = a->img.rgba[src + 3];
            if (alpha < 16) continue;
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

static void iso_tile_frame(uint8_t tile_id, uint32_t tick, PngFrame *f)
{
    f->w  = ISO_TILE_W;
    f->h  = ISO_TILE_H;
    f->ox = ISO_TILE_OX;
    f->oy = ISO_TILE_OY;
    switch (tile_id) {
    case T_WATER:  f->x = (int)((tick / 4u) % 4u) * ISO_TILE_W; f->y = 0; break;
    case T_GRASS:  f->x = 4  * ISO_TILE_W; f->y = 0; break;
    case T_PATH:   f->x = 5  * ISO_TILE_W; f->y = 0; break;
    case T_SAND:   f->x = 6  * ISO_TILE_W; f->y = 0; break;
    case T_TREE:   f->x = 7  * ISO_TILE_W; f->y = 0; break;
    case T_ROCK:   f->x = 8  * ISO_TILE_W; f->y = 0; break;
    case T_ORE:    f->x = 9  * ISO_TILE_W; f->y = 0; break;
    case T_FLOWER: f->x = 10 * ISO_TILE_W; f->y = 0; break;
    case T_TGRASS: f->x = 11 * ISO_TILE_W; f->y = 0; break;
    default:       f->x = f->y = 0; break;
    }
}

static bool tile_is_overlay(uint8_t tile_id)
{
    return tile_id == T_TREE || tile_id == T_ROCK ||
           tile_id == T_ORE  || tile_id == T_FLOWER;
}

bool iso_draw_tile_full(uint8_t tile_id, int cx, int cy, uint32_t tick)
{
    if (!iso_tile_id_on_sheet(tile_id)) return false;
    try_load_atlas(&g_tiles, "assets/assets_iso_tiles.png", "../assets/assets_iso_tiles.png");
    if (!g_tiles.loaded) return false;
    uint8_t floor_id = tile_is_overlay(tile_id) ? T_GRASS : tile_id;
    PngFrame f;
    iso_tile_frame(floor_id, tick, &f);
    draw_png_frame(&g_tiles, &f, cx, cy);
    return true;
}

bool iso_draw_tile_onlay(uint8_t tile_id, int cx, int cy)
{
    if (!tile_is_overlay(tile_id)) return false;
    try_load_atlas(&g_tiles, "assets/assets_iso_tiles.png", "../assets/assets_iso_tiles.png");
    if (!g_tiles.loaded) return false;
    PngFrame f;
    iso_tile_frame(tile_id, 0, &f);
    draw_png_frame(&g_tiles, &f, cx, cy);
    return true;
}

void iso_draw_player_foot(int foot_sx, int foot_sy, uint8_t dir)
{
    uint8_t d = dir;
    if (d > DIR_RIGHT) d = DIR_DOWN;
    try_load_atlas(&g_actors, "assets/assets_iso_actors.png", "../assets/assets_iso_actors.png");
    draw_png_frame_scaled(&g_actors, &PNG_PLAYER[d], foot_sx, foot_sy, ISO_ACTOR_SN, ISO_ACTOR_SD);
}

#define CHARS_SN 1
#define CHARS_SD 2

bool iso_draw_td_player_char(int sx, int sy, uint8_t dir, uint8_t walk_frame)
{
    return iso_draw_td_player_char_scaled(sx, sy, dir, walk_frame, CHARS_SN, CHARS_SD);
}

bool iso_draw_td_player_char_scaled(int sx, int sy, uint8_t dir, uint8_t walk_frame,
                                    int sn, int sd)
{
    try_load_atlas(&g_chars, "assets/assets_chars.png", "../assets/assets_chars.png");
    if (!g_chars.loaded) return false;
    uint8_t d = (dir > DIR_RIGHT) ? DIR_DOWN : dir;
    draw_png_frame_scaled(&g_chars, &PNG_TD_PLAYER[d * 2u + (walk_frame & 1u)],
                          sx, sy, sn, sd);
    return true;
}

bool iso_draw_depleted_mark(int cx, int cy)
{
    try_load_atlas(&g_tiles, "assets/assets_iso_tiles.png", "../assets/assets_iso_tiles.png");
    if (!g_tiles.loaded) return false;
    draw_png_frame(&g_tiles, &PNG_DEPLETED, cx, cy);
    return true;
}
