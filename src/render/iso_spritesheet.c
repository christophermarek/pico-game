#include "iso_spritesheet.h"
#include "hal.h"
#include "colors.h"
#include "config.h"

typedef struct {
    int x, y, w, h;
    int ox, oy; /* top-left = anchor + (ox, oy); anchor is iso cell centre (cx, cy) */
} PngFrame;

typedef struct {
    HalImageRGBA img;
    bool loaded;
    bool tried;
} PngAtlas;

static PngAtlas g_actors, g_tiles, g_mansion, g_chars, g_base;

/* Actor sprites drawn smaller than atlas cells (foot anchor preserved). */
#define ISO_ACTOR_SN 3
#define ISO_ACTOR_SD 4

/*
 * PNG atlases (assets/ relative to project root, or ../assets/ from build/).
 * All baked by tools/bake_iso_assets.py — re-run after changing art.
 *
 *  assets_iso_actors.png  160×64  — iso-foot sprites, 32×32 cells:
 *    Row 0: player ×4 (dir: DOWN/UP/LEFT/RIGHT)
 *    Row 1: monster iso-foot ×4 (evo stages 0-3)
 *
 *  assets_iso_tiles.png   832×48  — iso terrain, 64×48 cells:
 *    Cols 0-3: water (4 animation frames)
 *    Cols 4-11: grass/path/sand/tree/rock/ore/flower/tgrass
 *    Col  12: depleted-node overlay (center anchor, drawn on top)
 *
 *  assets_iso_mansion.png 768×128 — mansion building, 192×128 cells:
 *    4 bearing frames (0=N, 1=E, 2=S, 3=W), anchor at (96, 56)
 *
 *  assets_chars.png       256×192 — character/monster sprites at SPRITE_SCALE=2
 *    (stored at 2× game pixels; rendered via draw_png_frame_scaled(...,1,2))
 *    1 row per sprite type; frame index within each row noted below.
 *    Row 0 (y=0,   h=32): SV player ×4           (32px cells, cols 0-3)
 *    Row 1 (y=32,  h=32): SV tiles ×2  tree/ore   (32px cells, cols 0-1)
 *    Row 2 (y=64,  h=32): TD player ×8            (32px cells, cols 0-7)
 *    Row 3 (y=96,  h=32): monster-small ×8        (32px cells, cols 0-7)
 *    Row 4 (y=128, h=64): monster-large ×4        (64px cells, cols 0-3)
 *
 *  assets_base.png        224×32  — furniture sprites at SPRITE_SCALE=2, 32px cells:
 *    Cols 0-6: FURN_BED/WORKBENCH/FORGE/FARM/CHEST/PET_BED/PET_HOUSE (type indices 1-7)
 */
#define MANSION_FW  192
#define MANSION_FH  128
#define MANSION_FAX  96
#define MANSION_FAY  56
static const PngFrame PNG_MANSION[4] = {
    {   0, 0, MANSION_FW, MANSION_FH, -MANSION_FAX, -MANSION_FAY },
    { 192, 0, MANSION_FW, MANSION_FH, -MANSION_FAX, -MANSION_FAY },
    { 384, 0, MANSION_FW, MANSION_FH, -MANSION_FAX, -MANSION_FAY },
    { 576, 0, MANSION_FW, MANSION_FH, -MANSION_FAX, -MANSION_FAY },
};
/* assets_chars.png  256×192  — 1 row per sprite type, SPRITE_SCALE=2.
 * Rendered via draw_png_frame_scaled(...,1,2) to halve stored size. */
static const PngFrame PNG_SV_PLAYER[4] = {        /* Row 0  y=0   */
    {   0,  0, 32, 32, 0, 0 },  /* walk0_right */
    {  32,  0, 32, 32, 0, 0 },  /* walk1_right */
    {  64,  0, 32, 32, 0, 0 },  /* walk0_left  */
    {  96,  0, 32, 32, 0, 0 },  /* walk1_left  */
};
static const PngFrame PNG_SV_TREE = {  0, 32, 32, 32, 0, 0 }; /* Row 1  y=32 */
static const PngFrame PNG_SV_ORE  = { 32, 32, 32, 32, 0, 0 }; /* Row 1  y=32 */

static const PngFrame PNG_TD_PLAYER[8] = {        /* Row 2  y=64  */
    {   0, 64, 32, 32, 0, 0 }, /* DIR_DOWN  walk0 */
    {  32, 64, 32, 32, 0, 0 }, /* DIR_DOWN  walk1 */
    {  64, 64, 32, 32, 0, 0 }, /* DIR_UP    walk0 */
    {  96, 64, 32, 32, 0, 0 }, /* DIR_UP    walk1 */
    { 128, 64, 32, 32, 0, 0 }, /* DIR_LEFT  walk0 */
    { 160, 64, 32, 32, 0, 0 }, /* DIR_LEFT  walk1 */
    { 192, 64, 32, 32, 0, 0 }, /* DIR_RIGHT walk0 */
    { 224, 64, 32, 32, 0, 0 }, /* DIR_RIGHT walk1 */
};

static const PngFrame PNG_MON_SM[8] = {           /* Row 3  y=96  */
    {   0, 96, 32, 32, 0, 0 }, /* stage0 f0 */
    {  32, 96, 32, 32, 0, 0 }, /* stage0 f1 */
    {  64, 96, 32, 32, 0, 0 }, /* stage1 f0 */
    {  96, 96, 32, 32, 0, 0 }, /* stage1 f1 */
    { 128, 96, 32, 32, 0, 0 }, /* stage2 f0 */
    { 160, 96, 32, 32, 0, 0 }, /* stage2 f1 */
    { 192, 96, 32, 32, 0, 0 }, /* stage3 f0 */
    { 224, 96, 32, 32, 0, 0 }, /* stage3 f1 */
};

/* assets_base.png  224×32  — furniture types 1-7 in cols 0-6 (32×32 cells) */
static const PngFrame PNG_FURNITURE[7] = {
    {   0, 0, 32, 32, 0, 0 }, /* FURN_BED       (1) */
    {  32, 0, 32, 32, 0, 0 }, /* FURN_WORKBENCH (2) */
    {  64, 0, 32, 32, 0, 0 }, /* FURN_FORGE     (3) */
    {  96, 0, 32, 32, 0, 0 }, /* FURN_FARM      (4) */
    { 128, 0, 32, 32, 0, 0 }, /* FURN_CHEST     (5) */
    { 160, 0, 32, 32, 0, 0 }, /* FURN_PET_BED   (6) */
    { 192, 0, 32, 32, 0, 0 }, /* FURN_PET_HOUSE (7) */
};

/* assets_iso_tiles.png col 12 — iso depleted-node overlay (center anchor).
 * Literal values: ISO_TILE_W=64, ISO_TILE_H=48, ox=-32, oy=-24. */
static const PngFrame PNG_DEPLETED = { 768, 0, 64, 48, -32, -24 };

/* iso_actors.png — iso-foot sprites for player and monster */
static const PngFrame PNG_PLAYER[4] = {
    {   0,   0, 32, 32, -16, -31 },
    {  32,   0, 32, 32, -16, -31 },
    {  64,   0, 32, 32, -16, -31 },
    {  96,   0, 32, 32, -16, -31 },
};
static const PngFrame PNG_MON[4] = {
    {   0,  32, 32, 32, -16, -21 },
    {  32,  32, 32, 32, -16, -21 },
    {  64,  32, 32, 32, -16, -21 },
    {  96,  32, 32, 32, -16, -21 },
};
#define ISO_TILE_W 64
#define ISO_TILE_H 48
#define ISO_TILE_OX (-ISO_TILE_W / 2)
#define ISO_TILE_OY (-ISO_TILE_H / 2)

static void try_load_atlas(PngAtlas *a, const char *p0, const char *p1)
{
    if (a->tried)
        return;
    a->tried = true;
    a->loaded = hal_image_load_rgba(p0, &a->img);
    if (!a->loaded)
        a->loaded = hal_image_load_rgba(p1, &a->img);
}

static void draw_png_frame(const PngAtlas *a, const PngFrame *f, int ax, int ay)
{
    if (!a->loaded || !a->img.rgba)
        return;
    if (f->x < 0 || f->y < 0 || f->x + f->w > a->img.w || f->y + f->h > a->img.h)
        return;

    int ox = ax + f->ox;
    int oy = ay + f->oy;
    for (int y = 0; y < f->h; y++) {
        int sy = oy + y;
        if (sy < 0 || sy >= DISPLAY_H)
            continue;
        for (int x = 0; x < f->w; x++) {
            int sx = ox + x;
            if (sx < 0 || sx >= DISPLAY_W)
                continue;
            int src = ((f->y + y) * a->img.w + (f->x + x)) * 4;
            uint8_t alpha = a->img.rgba[src + 3];
            if (alpha < 16)
                continue;
            uint8_t r = a->img.rgba[src + 0];
            uint8_t g = a->img.rgba[src + 1];
            uint8_t b = a->img.rgba[src + 2];
            hal_pixel(sx, sy, RGB565(r, g, b));
        }
    }
}

/* Nearest-neighbour scale from atlas sub-rect to screen.
 * Output size = frame size * (sn/sd).  ax/ay are the anchor point on screen
 * (same semantics as draw_png_frame: top-left = anchor + frame.ox/oy). */
static void draw_png_frame_scaled(const PngAtlas *a, const PngFrame *f, int ax, int ay,
                                  int sn, int sd)
{
    if (!a->loaded || !a->img.rgba || sn <= 0 || sd <= 0)
        return;
    if (f->x < 0 || f->y < 0 || f->x + f->w > a->img.w || f->y + f->h > a->img.h)
        return;

    int out_w = (f->w * sn) / sd;
    int out_h = (f->h * sn) / sd;
    if (out_w < 1 || out_h < 1)
        return;

    int ox0 = ax + (f->ox * sn) / sd;
    int oy0 = ay + (f->oy * sn) / sd;

    for (int dy = 0; dy < out_h; dy++) {
        int screen_y = oy0 + dy;
        if (screen_y < 0 || screen_y >= DISPLAY_H)
            continue;
        int src_y = f->y + (dy * f->h) / out_h;
        for (int dx = 0; dx < out_w; dx++) {
            int screen_x = ox0 + dx;
            if (screen_x < 0 || screen_x >= DISPLAY_W)
                continue;
            int src_x = f->x + (dx * f->w) / out_w;
            int src = (src_y * a->img.w + src_x) * 4;
            uint8_t alpha = a->img.rgba[src + 3];
            if (alpha < 16)
                continue;
            hal_pixel(screen_x, screen_y,
                      RGB565(a->img.rgba[src + 0], a->img.rgba[src + 1], a->img.rgba[src + 2]));
        }
    }
}

static bool iso_tile_id_on_sheet(uint8_t tile_id)
{
    switch (tile_id) {
    case T_GRASS:
    case T_PATH:
    case T_WATER:
    case T_SAND:
    case T_TREE:
    case T_ROCK:
    case T_ORE:
    case T_FLOWER:
    case T_TGRASS:
    case T_HOME:
    case T_HOME_SOLID:
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
    case T_WATER:
        f->x = (int)((tick / 4u) % 4u) * ISO_TILE_W;
        f->y = 0;
        break;
    case T_GRASS:
        f->x = 4 * ISO_TILE_W;
        f->y = 0;
        break;
    case T_PATH:
        f->x = 5 * ISO_TILE_W;
        f->y = 0;
        break;
    case T_SAND:
        f->x = 6 * ISO_TILE_W;
        f->y = 0;
        break;
    case T_TREE:
        f->x = 7 * ISO_TILE_W;
        f->y = 0;
        break;
    case T_ROCK:
        f->x = 8 * ISO_TILE_W;
        f->y = 0;
        break;
    case T_ORE:
        f->x = 9 * ISO_TILE_W;
        f->y = 0;
        break;
    case T_FLOWER:
        f->x = 10 * ISO_TILE_W;
        f->y = 0;
        break;
    case T_TGRASS:
        f->x = 11 * ISO_TILE_W;
        f->y = 0;
        break;
    case T_HOME:
    case T_HOME_SOLID:
        f->x = 4 * ISO_TILE_W; /* same as T_GRASS */
        f->y = 0;
        break;
    default:
        f->x = f->y = 0;
        break;
    }
}

/* T_TREE / T_ROCK / T_ORE / T_FLOWER are overlay tiles: their PNG cell contains
 * only the feature sprite (transparent background).  The floor pass draws the
 * grass base; the overlay pass draws the feature on top via iso_draw_tile_onlay. */
static bool tile_is_overlay(uint8_t tile_id)
{
    return tile_id == T_TREE || tile_id == T_ROCK ||
           tile_id == T_ORE  || tile_id == T_FLOWER;
}

bool iso_draw_tile_full(uint8_t tile_id, int cx, int cy, uint32_t tick)
{
    if (!iso_tile_id_on_sheet(tile_id))
        return false;
    try_load_atlas(&g_tiles, "assets/assets_iso_tiles.png", "../assets/assets_iso_tiles.png");
    if (!g_tiles.loaded)
        return false;
    /* Overlay tiles show grass as their floor; the feature is drawn in the
     * overlay pass by iso_draw_tile_onlay. */
    uint8_t floor_id = tile_is_overlay(tile_id) ? T_GRASS : tile_id;
    PngFrame f;
    iso_tile_frame(floor_id, tick, &f);
    draw_png_frame(&g_tiles, &f, cx, cy);
    return true;
}

bool iso_draw_tile_onlay(uint8_t tile_id, int cx, int cy)
{
    if (!tile_is_overlay(tile_id))
        return false;
    try_load_atlas(&g_tiles, "assets/assets_iso_tiles.png", "../assets/assets_iso_tiles.png");
    if (!g_tiles.loaded)
        return false;
    PngFrame f;
    iso_tile_frame(tile_id, 0, &f);
    draw_png_frame(&g_tiles, &f, cx, cy);
    return true;
}

void iso_draw_player_foot(int foot_sx, int foot_sy, uint8_t dir, uint8_t skin_idx,
                          uint8_t hair_idx, uint8_t outfit_idx)
{
    (void)skin_idx;
    (void)hair_idx;
    (void)outfit_idx;
    uint8_t d = dir;
    if (d > DIR_RIGHT)
        d = DIR_DOWN;
    try_load_atlas(&g_actors, "assets/assets_iso_actors.png", "../assets/assets_iso_actors.png");
    draw_png_frame_scaled(&g_actors, &PNG_PLAYER[d], foot_sx, foot_sy, ISO_ACTOR_SN, ISO_ACTOR_SD);
}

void iso_draw_monster_foot(int foot_sx, int foot_sy, uint8_t evo_stage, uint8_t frame)
{
    (void)frame;
    uint8_t e = (evo_stage > 3u) ? 3u : evo_stage;
    try_load_atlas(&g_actors, "assets/assets_iso_actors.png", "../assets/assets_iso_actors.png");
    draw_png_frame_scaled(&g_actors, &PNG_MON[e], foot_sx, foot_sy, ISO_ACTOR_SN, ISO_ACTOR_SD);
}

bool iso_draw_mansion(int anchor_sx, int anchor_sy, uint8_t bearing)
{
    try_load_atlas(&g_mansion,
                   "assets/assets_iso_mansion.png",
                   "../assets/assets_iso_mansion.png");
    if (!g_mansion.loaded)
        return false;
    draw_png_frame(&g_mansion, &PNG_MANSION[bearing & 3u], anchor_sx, anchor_sy);
    return true;
}

/* SPRITE_SCALE = 2: chars/base frames are stored at 2× game pixels.
 * Render via draw_png_frame_scaled(..., 1, 2) to get back to 1:1 game pixels. */
#define CHARS_SN 1
#define CHARS_SD 2

bool iso_draw_sv_player(int sx, int sy, uint8_t dir, uint8_t walk_frame)
{
    try_load_atlas(&g_chars, "assets/assets_chars.png", "../assets/assets_chars.png");
    if (!g_chars.loaded)
        return false;
    uint8_t f = ((dir == DIR_LEFT) ? 2u : 0u) + (walk_frame & 1u);
    draw_png_frame_scaled(&g_chars, &PNG_SV_PLAYER[f], sx, sy, CHARS_SN, CHARS_SD);
    return true;
}

bool iso_draw_sv_tile(int sx, int sy, uint8_t tile_id)
{
    try_load_atlas(&g_chars, "assets/assets_chars.png", "../assets/assets_chars.png");
    if (!g_chars.loaded)
        return false;
    if (tile_id == T_TREE)
        draw_png_frame_scaled(&g_chars, &PNG_SV_TREE, sx, sy, CHARS_SN, CHARS_SD);
    else if (tile_id == T_ORE)
        draw_png_frame_scaled(&g_chars, &PNG_SV_ORE, sx, sy, CHARS_SN, CHARS_SD);
    else
        return false;
    return true;
}

bool iso_draw_td_player_char(int sx, int sy, uint8_t dir, uint8_t walk_frame)
{
    return iso_draw_td_player_char_scaled(sx, sy, dir, walk_frame, CHARS_SN, CHARS_SD);
}

bool iso_draw_td_player_char_scaled(int sx, int sy, uint8_t dir, uint8_t walk_frame,
                                    int sn, int sd)
{
    try_load_atlas(&g_chars, "assets/assets_chars.png", "../assets/assets_chars.png");
    if (!g_chars.loaded)
        return false;
    uint8_t d = (dir > DIR_RIGHT) ? DIR_DOWN : dir;
    draw_png_frame_scaled(&g_chars, &PNG_TD_PLAYER[d * 2u + (walk_frame & 1u)],
                          sx, sy, sn, sd);
    return true;
}

bool iso_draw_monster_sm(int sx, int sy, uint8_t evo_stage, uint8_t walk_frame)
{
    try_load_atlas(&g_chars, "assets/assets_chars.png", "../assets/assets_chars.png");
    if (!g_chars.loaded)
        return false;
    uint8_t s = (evo_stage > 3u) ? 3u : evo_stage;
    draw_png_frame_scaled(&g_chars, &PNG_MON_SM[s * 2u + (walk_frame & 1u)],
                          sx, sy, CHARS_SN, CHARS_SD);
    return true;
}

bool iso_draw_furniture(int sx, int sy, uint8_t furn_type)
{
    if (furn_type < 1u || furn_type > 7u)
        return false;
    try_load_atlas(&g_base, "assets/assets_base.png", "../assets/assets_base.png");
    if (!g_base.loaded)
        return false;
    draw_png_frame_scaled(&g_base, &PNG_FURNITURE[furn_type - 1u],
                          sx, sy, CHARS_SN, CHARS_SD);
    return true;
}

bool iso_draw_depleted_mark(int cx, int cy)
{
    try_load_atlas(&g_tiles, "assets/assets_iso_tiles.png", "../assets/assets_iso_tiles.png");
    if (!g_tiles.loaded)
        return false;
    draw_png_frame(&g_tiles, &PNG_DEPLETED, cx, cy);
    return true;
}

