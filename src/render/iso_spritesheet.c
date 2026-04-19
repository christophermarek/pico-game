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
 * PNG atlases (repo root or parent when cwd is build/):
 *  assets/assets_iso_actors.png  — 4 player + 3 monster iso-foot frames (32×32 cells)
 *  assets/assets_iso_tiles.png   — iso terrain (64×48 cells, col 12 = depleted mark)
 *  assets/assets_iso_mansion.png — 4 bearing frames (192×128 each)
 *  assets/assets_chars.png       — 224×64: SV/TD player, SV tiles, monsters (all sizes)
 *  assets/assets_base.png        — 128×16: furniture types 1-7 (16×16 cells)
 *
 * assets_chars.png layout:
 *   Row 0 (y=0,  h=16): SV player×4, SV tiles×2, TD player×8  (16px cells)
 *   Row 1 (y=16, h=16): monster-small×8                         (16px cells)
 *   Row 2 (y=32, h=32): monster-large×4                         (32px cells)
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
/* assets_chars.png — row 0: SV player (ox/oy=0 → top-left anchor) */
static const PngFrame PNG_SV_PLAYER[4] = {
    {  0, 0, 16, 16, 0, 0 },   /* walk0_right */
    { 16, 0, 16, 16, 0, 0 },   /* walk1_right */
    { 32, 0, 16, 16, 0, 0 },   /* walk0_left  */
    { 48, 0, 16, 16, 0, 0 },   /* walk1_left  */
};
static const PngFrame PNG_SV_TREE = {  64, 0, 16, 16, 0, 0 };
static const PngFrame PNG_SV_ORE  = {  80, 0, 16, 16, 0, 0 };

/* assets_chars.png — row 0: TD player (dir * 2 + walk_frame, cols 6-13) */
static const PngFrame PNG_TD_PLAYER[8] = {
    {  96, 0, 16, 16, 0, 0 }, /* DIR_DOWN  walk0 */
    { 112, 0, 16, 16, 0, 0 }, /* DIR_DOWN  walk1 */
    { 128, 0, 16, 16, 0, 0 }, /* DIR_UP    walk0 */
    { 144, 0, 16, 16, 0, 0 }, /* DIR_UP    walk1 */
    { 160, 0, 16, 16, 0, 0 }, /* DIR_LEFT  walk0 */
    { 176, 0, 16, 16, 0, 0 }, /* DIR_LEFT  walk1 */
    { 192, 0, 16, 16, 0, 0 }, /* DIR_RIGHT walk0 */
    { 208, 0, 16, 16, 0, 0 }, /* DIR_RIGHT walk1 */
};

/* assets_chars.png — row 1: monster-small (stage*2 + walk_frame) */
static const PngFrame PNG_MON_SM[8] = {
    {   0, 16, 16, 16, 0, 0 }, /* stage0 f0 */
    {  16, 16, 16, 16, 0, 0 }, /* stage0 f1 */
    {  32, 16, 16, 16, 0, 0 }, /* stage1 f0 */
    {  48, 16, 16, 16, 0, 0 }, /* stage1 f1 */
    {  64, 16, 16, 16, 0, 0 }, /* stage2 f0 */
    {  80, 16, 16, 16, 0, 0 }, /* stage2 f1 */
    {  96, 16, 16, 16, 0, 0 }, /* stage3 f0 */
    { 112, 16, 16, 16, 0, 0 }, /* stage3 f1 */
};

/* assets_chars.png — rows 2-3: monster-large 32×32 (stage index) */
static const PngFrame PNG_MON_LG[4] = {
    {  0, 32, 32, 32, 0, 0 },
    { 32, 32, 32, 32, 0, 0 },
    { 64, 32, 32, 32, 0, 0 },
    { 96, 32, 32, 32, 0, 0 },
};

/* assets_base.png — furniture types 1-7 in cols 0-6 */
static const PngFrame PNG_FURNITURE[7] = {
    {  0, 0, 16, 16, 0, 0 }, /* FURN_BED       (1) */
    { 16, 0, 16, 16, 0, 0 }, /* FURN_WORKBENCH (2) */
    { 32, 0, 16, 16, 0, 0 }, /* FURN_FORGE     (3) */
    { 48, 0, 16, 16, 0, 0 }, /* FURN_FARM      (4) */
    { 64, 0, 16, 16, 0, 0 }, /* FURN_CHEST     (5) */
    { 80, 0, 16, 16, 0, 0 }, /* FURN_PET_BED   (6) */
    { 96, 0, 16, 16, 0, 0 }, /* FURN_PET_HOUSE (7) */
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
static const PngFrame PNG_MON[3] = {
    {   0,  32, 32, 32, -16, -21 },
    {  32,  32, 32, 32, -16, -21 },
    {  64,  32, 32, 32, -16, -21 },
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

/* Nearest-neighbour down/up scale from atlas sub-rect to screen. */
static void draw_png_frame_scaled(const PngAtlas *a, const PngFrame *f, int foot_sx, int foot_sy,
                                  int sn, int sd)
{
    if (!a->loaded || !a->img.rgba || sd <= 0)
        return;
    if (f->x < 0 || f->y < 0 || f->x + f->w > a->img.w || f->y + f->h > a->img.h)
        return;

    int out_w = (f->w * sn) / sd;
    int out_h = (f->h * sn) / sd;
    if (out_w < 1)
        out_w = 1;
    if (out_h < 1)
        out_h = 1;
    int oxs = (f->ox * sn) / sd;
    int oys = (f->oy * sn) / sd;
    int ox0 = foot_sx + oxs;
    int oy0 = foot_sy + oys;

    for (int dy = 0; dy < out_h; dy++) {
        int sy = oy0 + dy;
        if (sy < 0 || sy >= DISPLAY_H)
            continue;
        int src_y = f->y + (dy * f->h) / out_h;
        if (src_y >= f->y + f->h)
            src_y = f->y + f->h - 1;
        for (int dx = 0; dx < out_w; dx++) {
            int sx = ox0 + dx;
            if (sx < 0 || sx >= DISPLAY_W)
                continue;
            int src_x = f->x + (dx * f->w) / out_w;
            if (src_x >= f->x + f->w)
                src_x = f->x + f->w - 1;
            int src = (src_y * a->img.w + src_x) * 4;
            uint8_t alpha = a->img.rgba[src + 3];
            if (alpha < 16)
                continue;
            hal_pixel(sx, sy,
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

bool iso_draw_tile_full(uint8_t tile_id, int cx, int cy, uint32_t tick)
{
    if (!iso_tile_id_on_sheet(tile_id))
        return false;
    try_load_atlas(&g_tiles, "assets/assets_iso_tiles.png", "../assets/assets_iso_tiles.png");
    if (!g_tiles.loaded)
        return false;
    PngFrame f;
    iso_tile_frame(tile_id, tick, &f);
    draw_png_frame(&g_tiles, &f, cx, cy);
    return true;
}

bool iso_tile_png_covers_onlay(uint8_t tile_id)
{
    if (!iso_tile_id_on_sheet(tile_id))
        return false;
    try_load_atlas(&g_tiles, "assets/assets_iso_tiles.png", "../assets/assets_iso_tiles.png");
    return g_tiles.loaded;
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
    uint8_t e = (evo_stage > 2) ? 2 : evo_stage;
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

bool iso_draw_sv_player(int sx, int sy, uint8_t dir, uint8_t walk_frame)
{
    try_load_atlas(&g_chars, "assets/assets_chars.png", "../assets/assets_chars.png");
    if (!g_chars.loaded)
        return false;
    uint8_t f = ((dir == DIR_LEFT) ? 2u : 0u) + (walk_frame & 1u);
    draw_png_frame(&g_chars, &PNG_SV_PLAYER[f], sx, sy);
    return true;
}

bool iso_draw_sv_tile(int sx, int sy, uint8_t tile_id)
{
    try_load_atlas(&g_chars, "assets/assets_chars.png", "../assets/assets_chars.png");
    if (!g_chars.loaded)
        return false;
    if (tile_id == T_TREE)
        draw_png_frame(&g_chars, &PNG_SV_TREE, sx, sy);
    else if (tile_id == T_ORE)
        draw_png_frame(&g_chars, &PNG_SV_ORE, sx, sy);
    else
        return false;
    return true;
}

bool iso_draw_td_player_char(int sx, int sy, uint8_t dir, uint8_t walk_frame)
{
    try_load_atlas(&g_chars, "assets/assets_chars.png", "../assets/assets_chars.png");
    if (!g_chars.loaded)
        return false;
    uint8_t d = (dir > DIR_RIGHT) ? DIR_DOWN : dir;
    draw_png_frame(&g_chars, &PNG_TD_PLAYER[d * 2u + (walk_frame & 1u)], sx, sy);
    return true;
}

bool iso_draw_monster_sm(int sx, int sy, uint8_t evo_stage, uint8_t walk_frame)
{
    try_load_atlas(&g_chars, "assets/assets_chars.png", "../assets/assets_chars.png");
    if (!g_chars.loaded)
        return false;
    uint8_t s = (evo_stage > 3u) ? 3u : evo_stage;
    draw_png_frame(&g_chars, &PNG_MON_SM[s * 2u + (walk_frame & 1u)], sx, sy);
    return true;
}

bool iso_draw_monster_lg(int sx, int sy, uint8_t evo_stage)
{
    try_load_atlas(&g_chars, "assets/assets_chars.png", "../assets/assets_chars.png");
    if (!g_chars.loaded)
        return false;
    uint8_t s = (evo_stage > 3u) ? 3u : evo_stage;
    draw_png_frame(&g_chars, &PNG_MON_LG[s], sx, sy);
    return true;
}

bool iso_draw_furniture(int sx, int sy, uint8_t furn_type)
{
    if (furn_type < 1u || furn_type > 7u)
        return false;
    try_load_atlas(&g_base, "assets/assets_base.png", "../assets/assets_base.png");
    if (!g_base.loaded)
        return false;
    draw_png_frame(&g_base, &PNG_FURNITURE[furn_type - 1u], sx, sy);
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

