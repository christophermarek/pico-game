#!/usr/bin/env python3
"""
Bake iso overworld PNG atlases from embedded character grids (kept in sync with
historical art; runtime loads only the PNGs — see iso_spritesheet.c).
Requires: pip install pillow

Writes under `<repo>/assets/`:
  assets_iso_actors.png   128x64   (32x32 cells — matches PNG_PLAYER / PNG_MON)
  assets_iso_tiles.png    768x48   (64x48 iso terrain cells — matches iso_tile_frame in C)
  assets_iso_mansion.png  768x128  (192x128 per bearing × 4 bearings — matches PNG_MANSION)
"""

from __future__ import annotations

import os
import sys

try:
    from PIL import Image
except ImportError:
    print("Install Pillow: pip install pillow", file=sys.stderr)
    sys.exit(1)


def asset_out_path(root: str, filename: str) -> str:
    d = os.path.join(root, "assets")
    os.makedirs(d, exist_ok=True)
    return os.path.join(d, filename)


def rgb_to_rgb565(r: int, g: int, b: int) -> int:
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b >> 3) & 0x1F)


def hex_rgb(h: int) -> tuple[int, int, int]:
    return ((h >> 16) & 0xFF, (h >> 8) & 0xFF, h & 0xFF)


def rgb565_to_rgba(u16: int) -> tuple[int, int, int, int]:
    r5 = (u16 >> 11) & 0x1F
    g6 = (u16 >> 5) & 0x3F
    b5 = u16 & 0x1F
    r = (r5 << 3) | (r5 >> 2)
    g = (g6 << 2) | (g6 >> 4)
    b = (b5 << 3) | (b5 >> 2)
    return (r, g, b, 255)


def h(hx: int) -> int:
    r, g, b = hex_rgb(hx)
    return rgb_to_rgb565(r, g, b)


# --- Palette (mirrors colors.h + iso_spritesheet.c map_*) ---
C_SKIN_TONES = [h(0xFDE68A), h(0xF5CBA7), h(0xC68642), h(0x8D5524)]
C_HAIR_COLS = [h(0x422006), h(0x1A1A1A), h(0xFBBF24), h(0xEF4444), h(0x8B5CF6), h(0xE2E8F0)]
C_OUTFIT_COLS = [h(0x7C3AED), h(0x1D4ED8), h(0x16A34A), h(0xDC2626), h(0xEA580C), h(0x4B5563)]
C_PLAYER_LEGS = h(0x3730A3)
C_PLAYER_BODY = h(0x7C3AED)
C_EYE = h(0x1A0A2E)
C_BLACK = 0
C_MON_PURPLE_M = h(0xA21CAF)
C_MON_PURPLE_D = h(0x6B21A8)
C_MON_PURPLE_B = h(0x7C3AED)
C_MON_CROWN    = h(0xFBBF24)
C_MON_DARK     = h(0x1E1B4B)
C_MON_RED      = h(0xDC2626)
C_MON_RED_L    = h(0xEF4444)
C_ROCK_MID  = h(0x6B7280)
C_ROCK_DARK = h(0x4B5563)
C_TREE_TRUNK = h(0x5C3A1E)
C_GOLD   = h(0xFBBF24)
C_TREE_MID = h(0x2D7A2D)
C_BG_DARK = h(0x050510)


def map_player(ch: str, skin_idx: int = 0, hair_idx: int = 0, outfit_idx: int = 0) -> int:
    if ch == "s":
        return C_SKIN_TONES[min(skin_idx, 3)]
    if ch == "h":
        return C_HAIR_COLS[min(hair_idx, 5)]
    if ch == "o":
        return C_OUTFIT_COLS[min(outfit_idx, 5)]
    if ch == "l":
        return C_PLAYER_LEGS
    if ch == "k":
        return C_EYE
    return C_BLACK


def map_mon(ch: str) -> int:
    if ch == "m":
        return C_MON_PURPLE_M
    if ch == "d":
        return C_MON_PURPLE_D
    if ch == "D":
        return C_MON_DARK
    if ch == "c":
        return C_MON_CROWN
    if ch == "e":
        return C_EYE
    if ch == "r":
        return C_MON_RED_L
    return C_BLACK


# --- Terrain (mirrors colors.h + sprites.c iso tiles) ---
C_GRASS_DARK = h(0x2D5A2D)
C_GRASS_LIGHT = h(0x3A7A3A)
C_PATH_DARK = h(0x8B6F47)
C_PATH_LIGHT = h(0xA0845A)
C_WATER_DARK = h(0x2A5A8A)
C_WATER_MID = h(0x3A7CB5)
C_WATER_LIGHT = h(0x5CA0D8)
C_SAND = h(0xC9A876)
C_SAND_DARK = h(0xB39060)
C_TREE_DARK = h(0x1A4D1A)
C_TREE_LIGHT = h(0x4A9A4A)
C_ROCK_LIGHT = h(0x9CA3AF)
C_FLOWER_PINK = h(0xEC4899)
C_TALL_GRASS = h(0x1A6B1A)
C_ORE_VEIN = h(0xFBBF24)
C_ORE_SHINE = h(0xFDE68A)


TILE = 16

# Tile IDs (mirrors config.h)
T_TREE = 4
T_ORE  = 9

# Furniture type IDs (mirrors state.h)
FURN_BED       = 1
FURN_WORKBENCH = 2
FURN_FORGE     = 3
FURN_FARM      = 4
FURN_CHEST     = 5
FURN_PET_BED   = 6
FURN_PET_HOUSE = 7

# ---- Mansion colours (matching sprites.c and colors.h) ----
MC_PLINTH_BODY = h(0x4B5563)   # C_ROCK_DARK
MC_PLINTH_TOP  = h(0x6B7280)   # C_ROCK_MID
MC_PLINTH_RIM  = h(0x9CA3AF)   # C_ROCK_LIGHT
MC_WALL_FRONT  = h(0x1A1030)   # C_PANEL
MC_WALL_SIDE   = h(0x110825)
MC_WALL_TRIM   = h(0x3730A3)   # C_BORDER
MC_WALL_SHADE  = h(0x070312)
MC_WIN_GLASS   = h(0x2D1B4E)   # C_SKY_MID
MC_WIN_GLOW    = h(0xFDE68A)   # C_GOLD_LIGHT
MC_DOOR_BODY   = h(0x5C3A1E)   # C_TREE_TRUNK
MC_DOOR_PAN    = h(0x8B6F47)   # C_PATH_DARK
MC_DOOR_KNOB   = h(0xFBBF24)   # C_GOLD
MC_MAT_OUTER   = h(0xA0845A)   # C_PATH_LIGHT
MC_MAT_INNER   = h(0xFDE68A)   # C_GOLD_LIGHT
MC_LINTEL      = h(0xFBBF24)   # C_BORDER_ACT
MC_ROOF_LIGHT  = h(0x9A1D4A)
MC_ROOF_MID    = h(0x7A1238)
MC_ROOF_DARK   = h(0x4D0A22)
MC_SMOKE_D     = h(0x6B7280)
MC_SMOKE_L     = h(0x9CA3AF)

# Mansion sprite frame layout (4 bearings side by side)
MANSION_FW  = 192   # per-frame width
MANSION_FH  = 128   # per-frame height
MANSION_FAX = 96    # anchor x (footprint centre) within frame
MANSION_FAY = 56    # anchor y (footprint centre) within frame


def put_px(im: Image.Image, x: int, y: int, u16: int) -> None:
    if x < 0 or y < 0 or x >= im.width or y >= im.height:
        return
    r, g, b, a = rgb565_to_rgba(u16)
    if a:
        im.putpixel((x, y), (r, g, b, a))


def horiz(im: Image.Image, x: int, y: int, w: int, u16: int) -> None:
    for i in range(w):
        put_px(im, x + i, y, u16)


def fill_rect_px(im: Image.Image, x: int, y: int, w: int, h: int, u16: int) -> None:
    for yy in range(h):
        for xx in range(w):
            put_px(im, x + xx, y + yy, u16)


def iso_diamond(im: Image.Image, cx: int, cy: int, half_w: int, half_h: int, u16: int) -> None:
    if half_h <= 0:
        half_h = 1
    for r in range(-half_h, half_h + 1):
        aw = (half_w * (half_h - abs(r)) + half_h - 1) // half_h
        if aw < 0:
            aw = 0
        x0 = cx - aw - 1
        w = aw * 2 + 3
        horiz(im, x0, cy + r, w, u16)


def iso_diamond_lit(
    im: Image.Image, cx: int, cy: int, hw: int, hh: int, base: int, rim_hi: int, rim_lo: int
) -> None:
    iso_diamond(im, cx, cy, hw, hh, base)
    for r in range(-hh, hh + 1):
        aw = (hw * (hh - abs(r)) + hh - 1) // hh
        if aw <= 0:
            continue
        put_px(im, cx - aw - 1, cy + r, rim_hi)
        put_px(im, cx + aw + 1, cy + r, rim_lo)


def new_cell() -> Image.Image:
    return Image.new("RGBA", (64, 48), (0, 0, 0, 0))


def bake_cell_water(phase: int) -> Image.Image:
    im = new_cell()
    cx, cy = 32, 24
    hw, hh = TILE, TILE // 2
    iso_diamond_lit(im, cx, cy, hw, hh, C_WATER_DARK, C_WATER_MID, h(0x1A3A5A))
    for row in range(-2, 3):
        aw = (hw * (hh - abs(row)) + hh - 1) // hh
        off = (phase + row * 2) % 5
        x = cx - aw + off
        while x < cx + aw:
            horiz(im, x, cy + row, 2, C_WATER_LIGHT)
            x += 5
    return im


def bake_cell_grass() -> Image.Image:
    im = new_cell()
    cx, cy = 32, 24
    hw, hh = TILE, TILE // 2
    iso_diamond_lit(im, cx, cy, hw, hh, C_GRASS_DARK, C_GRASS_LIGHT, h(0x1E3D1E))
    return im


def bake_cell_path() -> Image.Image:
    im = new_cell()
    cx, cy = 32, 24
    hw, hh = TILE, TILE // 2
    iso_diamond_lit(im, cx, cy, hw, hh, C_PATH_DARK, C_PATH_LIGHT, h(0x5C4A32))
    horiz(im, cx - 5, cy, 2, C_PATH_LIGHT)
    horiz(im, cx + 3, cy - 2, 2, C_PATH_LIGHT)
    horiz(im, cx - 1, cy + 3, 3, C_PATH_LIGHT)
    return im


def bake_cell_sand() -> Image.Image:
    im = new_cell()
    cx, cy = 32, 24
    hw, hh = TILE, TILE // 2
    iso_diamond_lit(im, cx, cy, hw, hh, C_SAND, C_SAND_DARK, h(0xA08050))
    horiz(im, cx - 2, cy + 1, 2, C_SAND_DARK)
    horiz(im, cx + 2, cy - 1, 3, C_SAND)
    return im


def bake_cell_tree() -> Image.Image:
    """Overlay-only: trunk + canopy on transparent bg (no grass base).
    The floor pass draws T_GRASS beneath this tile at render time."""
    im = new_cell()
    cx, cy = 32, 24
    hw, hh = TILE, TILE // 2
    for y in range(8):
        put_px(im, cx - 1, cy - 4 + y, C_TREE_TRUNK)
        put_px(im, cx, cy - 4 + y, C_TREE_TRUNK)
    iso_diamond(im, cx, cy - 6, hw - 4, hh - 2, C_TREE_DARK)
    iso_diamond(im, cx, cy - 9, hw - 8, hh - 4, C_TREE_MID)
    horiz(im, cx - 2, cy - 11, 4, C_TREE_LIGHT)
    return im


def bake_cell_rock() -> Image.Image:
    im = new_cell()
    cx, cy = 32, 24
    hw, hh = TILE, TILE // 2
    iso_diamond(im, cx, cy, hw - 4, hh - 1, h(0x2A323C))
    iso_diamond(im, cx, cy - 2, hw - 4, hh - 1, C_ROCK_DARK)
    iso_diamond(im, cx, cy - 4, hw - 8, hh - 3, C_ROCK_MID)
    horiz(im, cx - 3, cy - 6, 6, C_ROCK_LIGHT)
    return im


def bake_cell_ore() -> Image.Image:
    im = bake_cell_rock()
    cx, cy = 32, 24
    fill_rect_px(im, cx - 2, cy - 4, 2, 2, C_ORE_VEIN)
    fill_rect_px(im, cx + 2, cy - 2, 3, 2, C_ORE_VEIN)
    fill_rect_px(im, cx, cy, 2, 1, C_ORE_SHINE)
    return im


def bake_cell_flower() -> Image.Image:
    """Overlay-only: stem + petals on transparent bg (no grass base).
    The floor pass draws T_GRASS beneath this tile at render time."""
    im = new_cell()
    cx, cy = 32, 24
    horiz(im, cx - 1, cy + 1, 2, C_GRASS_LIGHT)
    for y in range(3):
        put_px(im, cx, cy + 1 + y, C_GRASS_LIGHT)
    horiz(im, cx - 3, cy - 2, 6, C_FLOWER_PINK)
    for y in range(4):
        put_px(im, cx - 1, cy - 4 + y, C_FLOWER_PINK)
        put_px(im, cx, cy - 4 + y, C_FLOWER_PINK)
    horiz(im, cx - 1, cy - 2, 2, C_GOLD)
    return im


def bake_cell_tgrass() -> Image.Image:
    im = new_cell()
    cx, cy = 32, 24
    hw, hh = TILE, TILE // 2
    iso_diamond_lit(im, cx, cy, hw, hh, h(0x234D23), C_TALL_GRASS, h(0x143814))
    for i in range(4):
        rr = -hh + 2 + i * 3
        if rr > hh - 2:
            break
        aw = (hw * (hh - abs(rr)) + hh - 1) // hh
        denom = max(aw * 2 - 2, 1)
        lx = cx - aw + 1 + (i * 5) % denom
        for y in range(5):
            put_px(im, lx, cy + rr + y, C_TALL_GRASS)
    return im


def _w2f(bearing: int, wx: float, wy: float) -> tuple[int, int]:
    """World offset from mansion footprint centre → frame pixel coordinates."""
    if bearing == 0:   rx, ry = wx, wy
    elif bearing == 1: rx, ry = -wy, wx
    elif bearing == 2: rx, ry = -wx, -wy
    else:              rx, ry = wy, -wx
    return MANSION_FAX + round(rx - ry), MANSION_FAY + round((rx + ry) * 0.5)


def _fill_wall_m(im: Image.Image, ax: int, ay: int, bx: int, by: int, H: int, u16: int) -> None:
    """Port of C fill_wall: column-by-column parallelogram."""
    if H < 1:
        return
    if ax == bx:
        y0, y1 = min(ay, by) - H, max(ay, by)
        for y in range(y0, y1 + 1):
            put_px(im, ax, y, u16)
        return
    if ax > bx:
        ax, bx, ay, by = bx, ax, by, ay
    dx = bx - ax
    for x in range(ax, bx + 1):
        y_bot = ay + ((by - ay) * (x - ax) + (dx >> 1)) // dx
        for yy in range(y_bot - H, y_bot + 1):
            put_px(im, x, yy, u16)


def _fill_tri_m(im: Image.Image, x1: int, y1: int, x2: int, y2: int,
                x3: int, y3: int, u16: int) -> None:
    """Port of C fill_tri: scanline triangle rasteriser."""
    pts = sorted([(y1, x1), (y2, x2), (y3, x3)])
    y1, x1 = pts[0]; y2, x2 = pts[1]; y3, x3 = pts[2]
    if y3 == y1:
        horiz(im, min(x1, x2, x3), y1, max(x1, x2, x3) - min(x1, x2, x3) + 1, u16)
        return
    dy_long = y3 - y1; dx_long = x3 - x1
    dy_top  = y2 - y1; dx_top  = x2 - x1
    for y in range(y1, y2 + 1):
        xa = x1 if dy_top == 0 else x1 + dx_top * (y - y1) // dy_top
        xb = x1 + dx_long * (y - y1) // dy_long
        if xa > xb: xa, xb = xb, xa
        horiz(im, xa, y, xb - xa + 1, u16)
    dy_bot = y3 - y2; dx_bot = x3 - x2
    for y in range(y2 + 1, y3 + 1):
        xa = x2 if dy_bot == 0 else x2 + dx_bot * (y - y2) // dy_bot
        xb = x1 + dx_long * (y - y1) // dy_long
        if xa > xb: xa, xb = xb, xa
        horiz(im, xa, y, xb - xa + 1, u16)


def _fill_iso_quad_m(im: Image.Image, ax: int, ay: int, bx: int, by: int,
                     cx: int, cy: int, dx: int, dy: int, u16: int) -> None:
    _fill_tri_m(im, ax, ay, bx, by, cx, cy, u16)
    _fill_tri_m(im, ax, ay, cx, cy, dx, dy, u16)


def bake_mansion_frame(bearing: int) -> Image.Image:
    """Render one bearing of the mansion into a 192×128 RGBA frame.

    The anchor point (pixel MANSION_FAX, MANSION_FAY = 96, 56) maps to the
    footprint centre in world space; iso_draw_mansion() in C positions the
    frame so that pixel aligns with the screen-projected footprint centre.
    """
    im = Image.new("RGBA", (MANSION_FW, MANSION_FH), (0, 0, 0, 0))

    MV_HW    = 48;  MV_HH    = 32
    H_PLINTH = 4;   H_WALL   = 28;  H_ROOF = 22
    wall_base = H_PLINTH

    # Footprint corners in world offsets from centre → frame pixel coords
    world_corners = [(-MV_HW, -MV_HH), (MV_HW, -MV_HH),
                     ( MV_HW,  MV_HH), (-MV_HW,  MV_HH)]
    cx_ = []; cy_ = []
    for wx, wy in world_corners:
        fx, fy = _w2f(bearing, wx, wy)
        cx_.append(fx); cy_.append(fy)

    fcx, fcy = MANSION_FAX, MANSION_FAY   # footprint centre in frame
    front = max(range(4), key=lambda i: cy_[i])
    edge_vis = [(e == front) or ((e + 1) % 4 == front) for e in range(4)]

    # 5. Plinth body
    for e in range(4):
        if not edge_vis[e]: continue
        a, b = e, (e + 1) % 4
        _fill_wall_m(im, cx_[a], cy_[a], cx_[b], cy_[b], H_PLINTH, MC_PLINTH_BODY)
    _fill_iso_quad_m(im,
        cx_[0], cy_[0] - H_PLINTH, cx_[1], cy_[1] - H_PLINTH,
        cx_[2], cy_[2] - H_PLINTH, cx_[3], cy_[3] - H_PLINTH, MC_PLINTH_TOP)
    # Plinth rim highlight
    for e in range(4):
        if not edge_vis[e]: continue
        a, b = e, (e + 1) % 4
        ax_, ay_ = cx_[a], cy_[a] - H_PLINTH
        bx_, by_ = cx_[b], cy_[b] - H_PLINTH
        if ax_ == bx_: continue
        if ax_ > bx_: ax_, bx_, ay_, by_ = bx_, ax_, by_, ay_
        ddx = bx_ - ax_
        for x in range(ax_, bx_ + 1):
            y = ay_ + ((by_ - ay_) * (x - ax_) + (ddx >> 1)) // ddx
            put_px(im, x, y, MC_PLINTH_RIM)

    # 6. Main walls
    fe_a = (front + 3) & 3
    fe_b = front
    mid_ay = (cy_[fe_a] + cy_[(fe_a + 1) % 4]) / 2
    mid_by = (cy_[fe_b] + cy_[(fe_b + 1) % 4]) / 2
    if mid_ay >= mid_by:
        front_edge, side_edge = fe_a, fe_b
    else:
        front_edge, side_edge = fe_b, fe_a

    a, b = front_edge, (front_edge + 1) % 4
    _fill_wall_m(im, cx_[a], cy_[a] - wall_base, cx_[b], cy_[b] - wall_base, H_WALL, MC_WALL_FRONT)
    a, b = side_edge, (side_edge + 1) % 4
    _fill_wall_m(im, cx_[a], cy_[a] - wall_base, cx_[b], cy_[b] - wall_base, H_WALL, MC_WALL_SIDE)

    # Cornice (2px trim at wall top)
    for e in range(4):
        if not edge_vis[e]: continue
        a, b = e, (e + 1) % 4
        _fill_wall_m(im, cx_[a], cy_[a] - wall_base - H_WALL + 2,
                     cx_[b], cy_[b] - wall_base - H_WALL + 2, 2, MC_WALL_TRIM)
    # Base shade (2px shadow at wall/plinth join)
    for e in range(4):
        if not edge_vis[e]: continue
        a, b = e, (e + 1) % 4
        _fill_wall_m(im, cx_[a], cy_[a] - wall_base, cx_[b], cy_[b] - wall_base, 2, MC_WALL_SHADE)
    # Vertical seam at front corner
    fill_rect_px(im, cx_[front], cy_[front] - wall_base - H_WALL, 1, H_WALL + 1, MC_WALL_SHADE)

    # 7. Windows (2 passes: front wall → side wall)
    for pass_ in range(2):
        e = front_edge if pass_ == 0 else side_edge
        a, b = e, (e + 1) % 4
        ax_, ay_ = cx_[a], cy_[a] - wall_base
        bx_, by_ = cx_[b], cy_[b] - wall_base
        wall_len  = abs(bx_ - ax_)
        count     = 4 if wall_len >= 64 else (3 if wall_len >= 36 else 2)
        is_front  = (pass_ == 0)
        skip_ctr  = (e == 2 and is_front)
        for i in range(1, count + 1):
            t   = i / (count + 1)
            wx_ = ax_ + round(t * (bx_ - ax_))
            wy_ = ay_ + round(t * (by_ - ay_))
            if skip_ctr and i == (count + 1) // 2: continue
            # upper-floor window
            ut = wy_ - H_WALL + 6
            fill_rect_px(im, wx_ - 2, ut,     5, 7, MC_WALL_SHADE)
            fill_rect_px(im, wx_ - 2, ut + 1, 5, 5, MC_WIN_GLASS)
            fill_rect_px(im, wx_,     ut + 1, 1, 5, MC_WALL_SHADE)
            fill_rect_px(im, wx_ - 2, ut + 3, 5, 1, MC_WALL_SHADE)
            fill_rect_px(im, wx_ - 1, ut + 1, 1, 1, MC_WIN_GLOW)
            # lower-floor window (front wall only)
            if is_front:
                lt = wy_ - 16
                fill_rect_px(im, wx_ - 2, lt,     5, 7, MC_WALL_SHADE)
                fill_rect_px(im, wx_ - 2, lt + 1, 5, 5, MC_WIN_GLASS)
                fill_rect_px(im, wx_,     lt + 1, 1, 5, MC_WALL_SHADE)
                fill_rect_px(im, wx_ - 2, lt + 3, 5, 1, MC_WALL_SHADE)
                fill_rect_px(im, wx_ - 1, lt + 1, 1, 1, MC_WIN_GLOW)

    # 8. Door (only when south wall / edge 2 is visible)
    if edge_vis[2]:
        a, b   = 2, 3
        ax_d   = cx_[a]; ay_d = cy_[a] - wall_base
        bx_d   = cx_[b]; by_d = cy_[b] - wall_base
        dw, dh = 11, 16
        mid_x  = (ax_d + bx_d) // 2
        dleft  = mid_x - dw // 2
        dright = mid_x + dw // 2
        denom  = bx_d - ax_d if bx_d != ax_d else 1

        def base_y(X: int) -> int:
            return round(ay_d + (by_d - ay_d) * (X - ax_d) / denom)

        _fill_wall_m(im, dleft - 1, base_y(dleft - 1),
                     dright + 1,  base_y(dright + 1), dh + 1, MC_WALL_TRIM)
        _fill_wall_m(im, dleft, base_y(dleft), dright, base_y(dright), dh, MC_DOOR_BODY)
        # panels
        panel_h = max(3, (dh - 2 - 2 - 3) // 2)
        for px0 in [dleft + 1, dright - 1 - 3]:
            px1 = px0 + 3
            py0 = base_y(px0); py1 = base_y(px1)
            _fill_wall_m(im, px0, py0 - dh + 2 + panel_h,
                         px1, py1 - dh + 2 + panel_h, panel_h, MC_DOOR_PAN)
            _fill_wall_m(im, px0, py0 - 2, px1, py1 - 2, panel_h, MC_DOOR_PAN)
        # lintel
        _fill_wall_m(im, dleft + 1, base_y(dleft + 1) - dh + 2,
                     dright - 1,   base_y(dright - 1) - dh + 2, 1, MC_LINTEL)
        # knob
        kx = dright - 3
        fill_rect_px(im, kx, base_y(kx) - dh // 2, 2, 2, MC_DOOR_KNOB)
        # welcome mat
        mat_cx = mid_x
        mat_cy = (ay_d + by_d) // 2 + 3
        iso_diamond(im, mat_cx, mat_cy, 7, 3, MC_MAT_OUTER)
        iso_diamond(im, mat_cx, mat_cy, 4, 2, MC_MAT_INNER)

    # 9. Roof (4 triangular panels, back → front)
    rtop_x = cx_[:]
    rtop_y = [cy_[i] - wall_base - H_WALL for i in range(4)]
    apex_x = fcx
    apex_y = fcy - wall_base - H_WALL - H_ROOF
    panels = sorted(
        [((rtop_y[e] + rtop_y[(e + 1) % 4] + apex_y) // 3, e, (e + 1) % 4)
         for e in range(4)],
        key=lambda p: p[0]
    )
    for _, a, b in panels:
        col = MC_ROOF_MID if (a == front or b == front) else MC_ROOF_DARK
        _fill_tri_m(im, rtop_x[a], rtop_y[a], rtop_x[b], rtop_y[b], apex_x, apex_y, col)
    # ridge highlights on front-facing panels
    for _, a, b in panels:
        if a != front and b != front: continue
        rfx = rtop_x[a] if a == front else rtop_x[b]
        rfy = rtop_y[a] if a == front else rtop_y[b]
        steps = max(1, abs(apex_y - rfy) + abs(apex_x - rfx))
        for i in range(steps + 1):
            put_px(im, rfx + (apex_x - rfx) * i // steps,
                       rfy + (apex_y - rfy) * i // steps, MC_ROOF_LIGHT)
    # eave line
    for e in range(4):
        if not edge_vis[e]: continue
        a, b = e, (e + 1) % 4
        _fill_wall_m(im, rtop_x[a], rtop_y[a] + 1, rtop_x[b], rtop_y[b] + 1, 1, MC_ROOF_DARK)

    # 10. Chimney (biased toward front corner along apex→front ridge)
    chx = apex_x + (rtop_x[front] - apex_x) * 4 // 10
    chy = apex_y + (rtop_y[front] - apex_y) * 4 // 10
    cw, ch = 6, 9
    fill_rect_px(im, chx - cw // 2,     chy - ch,     cw,     ch,     MC_PLINTH_BODY)
    fill_rect_px(im, chx - cw // 2,     chy - ch,     cw,     2,      MC_PLINTH_RIM)
    fill_rect_px(im, chx - cw // 2 - 1, chy - ch - 2, cw + 2, 2,      MC_PLINTH_TOP)
    fill_rect_px(im, chx - 1,           chy - ch - 4, 3,      2,      MC_SMOKE_D)
    fill_rect_px(im, chx,               chy - ch - 6, 2,      2,      MC_SMOKE_L)

    return im


def bake_mansion(root: str) -> None:
    """Generate assets_iso_mansion.png: 4 bearing frames (192×128 each) side by side."""
    sheet = Image.new("RGBA", (MANSION_FW * 4, MANSION_FH), (0, 0, 0, 0))
    for b in range(4):
        frame = bake_mansion_frame(b)
        sheet.paste(frame, (b * MANSION_FW, 0), frame)
    path = asset_out_path(root, "assets_iso_mansion.png")
    sheet.save(path)
    print("Wrote", path)


def bake_cell_depleted_mark() -> Image.Image:
    """Iso-diamond overlay for depleted resource nodes (col 12 of tiles sheet)."""
    im = new_cell()
    cx, cy = 32, 24
    hw = TILE - 4
    hh = TILE // 2 - 2
    iso_diamond(im, cx, cy,     hw,     hh,     C_ROCK_DARK)
    iso_diamond(im, cx, cy + 1, hw - 3, hh - 1, h(0x1F2937))
    return im


def bake_tiles(root: str) -> None:
    tw, th = 64, 48
    sheet = Image.new("RGBA", (13 * tw, th), (0, 0, 0, 0))
    for p in range(4):
        c = bake_cell_water(p)
        sheet.paste(c, (p * tw, 0), c)
    cells = [
        bake_cell_grass(),
        bake_cell_path(),
        bake_cell_sand(),
        bake_cell_tree(),
        bake_cell_rock(),
        bake_cell_ore(),
        bake_cell_flower(),
        bake_cell_tgrass(),
    ]
    for i, c in enumerate(cells):
        sheet.paste(c, ((4 + i) * tw, 0), c)
    # col 12: depleted-node overlay
    dm = bake_cell_depleted_mark()
    sheet.paste(dm, (12 * tw, 0), dm)
    path = asset_out_path(root, "assets_iso_tiles.png")
    sheet.save(path)
    print("Wrote", path)


def raster_cell(rows: list[str], scale: int, mapper) -> Image.Image:
    hgt = len(rows)
    wid = len(rows[0]) if rows else 0
    im = Image.new("RGBA", (wid * scale, hgt * scale), (0, 0, 0, 0))
    px = im.load()
    for y, row in enumerate(rows):
        for x, ch in enumerate(row):
            if ch == ".":
                continue
            u16 = mapper(ch)
            r, g, b, a = rgb565_to_rgba(u16)
            for dy in range(scale):
                for dx in range(scale):
                    px[x * scale + dx, y * scale + dy] = (r, g, b, a)
    return im


PLAYER_DOWN = [
    "................",
    ".....hhhhhh.....",
    "....hssssssh....",
    "....hssssssh....",
    "....hsk..ksh....",
    "....hssssssh....",
    "....oooooooo....",
    "...oooooooooo...",
    "...oooooooooo...",
    "...oooooooooo...",
    "....oolllloo....",
    "....oolllloo....",
    "....ll....ll....",
    "....ll....ll....",
    "................",
    "................",
]

PLAYER_UP = [
    "................",
    ".....hhhhhh.....",
    "....hhhhhhhh....",
    "....hssssssh....",
    "....hssssssh....",
    "....hssssssh....",
    "....oooooooo....",
    "...oooooooooo...",
    "...oooooooooo...",
    "...oooooooooo...",
    "....oolllloo....",
    "....oolllloo....",
    "....ll....ll....",
    "....ll....ll....",
    "................",
    "................",
]

PLAYER_LEFT = [
    "................",
    ".....hhhhhh.....",
    "....hssssssh....",
    "....hssssssh....",
    "....hk.ssssh....",
    "....hssssssh....",
    "....oooooooo....",
    "...oooooooooo...",
    "...oooooooooo...",
    "...oooooooooo...",
    "....oolllloo....",
    "....oolllloo....",
    "....ll....ll....",
    "....ll....ll....",
    "................",
    "................",
]

PLAYER_RIGHT = [
    "................",
    ".....hhhhhh.....",
    "....hssssssh....",
    "....hssssssh....",
    "....hssss.kh....",
    "....hssssssh....",
    "....oooooooo....",
    "...oooooooooo...",
    "...oooooooooo...",
    "...oooooooooo...",
    "....oolllloo....",
    "....oolllloo....",
    "....ll....ll....",
    "....ll....ll....",
    "................",
    "................",
]

MON0_A = [
    "................",
    "................",
    "......mmmm......",
    "....mmmmmmmm....",
    "...mmmmmmmmmm...",
    "...mmme..emmm...",
    "..mmmmmmmmmmmm..",
    "..mmmmmmmmmmmm..",
    "...mmmmmmmmmm...",
    "....mmmmmmmm....",
    ".....mm..mm.....",
    "................",
    "................",
    "................",
    "................",
    "................",
]

MON1_A = [
    "................",
    ".....d....d.....",
    "....mmmmmmmm....",
    "...mmmmmmmmmm...",
    "..mmmmmmmmmmmm..",
    "..mmme....emmm..",
    "..mmmmmmmmmmmm..",
    "..mmmmmmmmmmmm..",
    "...mmmmmmmmmm...",
    "....mmmmmmmm....",
    "....dd....dd....",
    "................",
    "................",
    "................",
    "................",
    "................",
]

MON2_A = [
    "......cc........",
    "....cccccc......",
    "...mmmmmmmmm....",
    "..mmmmmmmmmmm...",
    ".mmmmmmmmmmmmm..",
    ".mmmmme..emmmm..",
    ".mmmmmmmmmmmmm..",
    ".mmmmmmmmmmmmm..",
    "..mmmmmmmmmmm...",
    "...mmmmmmmmm....",
    "....dd....dd....",
    "................",
    "................",
    "................",
    "................",
    "................",
]

MON3_A = [
    "....cccccccc....",
    ".....c....c.....",
    "...DDDDDDDDDd...",
    "..dDDDDDDDDDDd..",
    ".DDDDrr..rrDDDD.",
    ".DDDDDDDDDDDDD..",
    ".dDDDDDDDDDDDd..",
    "..DDDDDDDDDDD...",
    "...DDDDDDDDDd...",
    "....DDDDDDDDD...",
    "....DD....DD....",
    "................",
    "................",
    "................",
    "................",
    "................",
]


def bake_actors(root: str) -> None:
    # 4 player cols + 4 monster cols = 8 cols × 32px = 256px wide; height = 64px
    out = Image.new("RGBA", (160, 64), (0, 0, 0, 0))
    players = [PLAYER_DOWN, PLAYER_UP, PLAYER_LEFT, PLAYER_RIGHT]
    for i, rows in enumerate(players):
        cell = raster_cell(rows, 2, lambda ch: map_player(ch))
        out.paste(cell, (i * 32, 0), cell)
    mons = [MON0_A, MON1_A, MON2_A, MON3_A]
    for i, rows in enumerate(mons):
        cell = raster_cell(rows, 2, map_mon)
        out.paste(cell, (i * 32, 32), cell)
    path = asset_out_path(root, "assets_iso_actors.png")
    out.save(path)
    print("Wrote", path)


SPRITE_SCALE = 2   # All new char/base sprites baked at 2× game pixels


def bake_sv_player_frame(walk_frame: int, dir_left: bool) -> Image.Image:
    """32×32 side-view player cell (2× game pixels)."""
    S = SPRITE_SCALE
    im = Image.new("RGBA", (16 * S, 16 * S), (0, 0, 0, 0))
    def R(x: int, y: int, w: int, hh: int, u16: int) -> None:
        fill_rect_px(im, x * S, y * S, w * S, hh * S, u16)
    skin   = C_SKIN_TONES[0]
    hair   = C_HAIR_COLS[0]
    outfit = C_OUTFIT_COLS[0]
    R(2, 5, 10, 7, outfit)
    if walk_frame == 0:
        R(3, 11, 3, 5, C_PLAYER_LEGS)
        R(8, 11, 3, 5, C_PLAYER_LEGS)
    else:
        R(3, 10, 3, 6, C_PLAYER_LEGS)
        R(8, 12, 3, 4, C_PLAYER_LEGS)
    R(2, 0, 10, 7, skin)
    R(2, 0, 10, 3, hair)
    if dir_left:
        R(3, 3, 2, 2, C_EYE)
    else:
        R(7, 3, 2, 2, C_EYE)
    return im


def bake_sv_tile(tile_id: int) -> Image.Image:
    """32×32 side-view tile for T_TREE or T_ORE (2× game pixels)."""
    S = SPRITE_SCALE
    im = Image.new("RGBA", (16 * S, 16 * S), (0, 0, 0, 0))
    def R(x: int, y: int, w: int, hh: int, u16: int) -> None:
        fill_rect_px(im, x * S, y * S, w * S, hh * S, u16)
    if tile_id == T_TREE:
        R(0,  0, 16, 16, C_GRASS_DARK)
        R(6, 10,  4,  6, C_TREE_TRUNK)
        R(2,  2, 12, 10, C_TREE_DARK)
        R(3,  3, 10,  8, C_TREE_MID)
        R(5,  4,  3,  2, C_TREE_LIGHT)
        R(9,  6,  2,  2, C_TREE_LIGHT)
    else:  # T_ORE
        R(0,  0, 16, 16, C_ROCK_DARK)
        R(2,  4, 12,  8, C_ROCK_MID)
        R(3,  2, 10,  4, C_ROCK_DARK)
        R(4,  5,  2,  2, C_ORE_VEIN)
        R(8,  8,  3,  2, C_ORE_VEIN)
        R(11, 5,  2,  3, C_ORE_VEIN)
        R(6, 10,  2,  1, C_ORE_SHINE)
    return im


def bake_td_player_frame(dir: int, walk_frame: int) -> Image.Image:
    """32×32 top-down player cell (2× game pixels). dir: 0=DOWN 1=UP 2=LEFT 3=RIGHT."""
    S = SPRITE_SCALE
    im = Image.new("RGBA", (16 * S, 16 * S), (0, 0, 0, 0))
    def R(x: int, y: int, w: int, hh: int, u16: int) -> None:
        fill_rect_px(im, x * S, y * S, w * S, hh * S, u16)
    skin   = C_SKIN_TONES[0]
    hair   = C_HAIR_COLS[0]
    outfit = C_OUTFIT_COLS[0]
    R(3,  6, 10, 8, outfit)
    if walk_frame == 0:
        R(4, 12, 3, 4, C_PLAYER_LEGS)
        R(9, 12, 3, 4, C_PLAYER_LEGS)
    else:
        R(4, 11, 3, 5, C_PLAYER_LEGS)
        R(9, 13, 3, 3, C_PLAYER_LEGS)
    R(3, 1, 10, 8, skin)
    R(3, 1, 10, 3, hair)
    if dir == 0:       # DIR_DOWN
        R(5, 5, 2, 2, C_EYE)
        R(9, 5, 2, 2, C_EYE)
    elif dir == 1:     # DIR_UP — back of head, no eyes
        pass
    elif dir == 2:     # DIR_LEFT
        R(4, 5, 2, 2, C_EYE)
    else:              # DIR_RIGHT
        R(10, 5, 2, 2, C_EYE)
    return im


def bake_monster_frame(evo_stage: int, walk_frame: int) -> Image.Image:
    """32×32 monster cell for given stage and walk frame (2× game pixels)."""
    S = SPRITE_SCALE
    im = Image.new("RGBA", (16 * S, 16 * S), (0, 0, 0, 0))
    def R(x: int, y: int, w: int, hh: int, u16: int) -> None:
        fill_rect_px(im, x * S, y * S, w * S, hh * S, u16)
    bob  = walk_frame & 1
    M    = C_MON_PURPLE_M
    D    = C_MON_PURPLE_D
    dark = C_MON_DARK
    if evo_stage == 0:
        R(4,  6 + bob,  8,  6, M)
        R(3,  7 + bob, 10,  4, M)
        R(5,  8 + bob,  2,  2, C_EYE)
        R(9,  8 + bob,  2,  2, C_EYE)
        R(3,  5 + bob,  3,  2, D)
        R(10, 5 + bob,  3,  2, D)
    elif evo_stage == 1:
        R(3,  4 + bob, 10,  9, M)
        R(2,  6 + bob, 12,  6, M)
        R(4,  1 + bob,  2,  4, D)
        R(10, 1 + bob,  2,  4, D)
        R(5,  6 + bob,  2,  2, C_EYE)
        R(9,  6 + bob,  2,  2, C_EYE)
        R(4,  12,        3,  3, dark)
        R(9,  12,        3,  3, dark)
    elif evo_stage == 2:
        R(2,  3 + bob, 12, 10, D)
        R(1,  5 + bob, 14,  7, M)
        R(3,  4 + bob,  4,  3, dark)
        R(9,  4 + bob,  4,  3, dark)
        R(4,  6 + bob,  3,  2, C_MON_RED)
        R(9,  6 + bob,  3,  2, C_MON_RED)
        R(3,  12,        3,  4, dark)
        R(8,  12,        3,  4, dark)
        R(12, 8 + bob,   3,  2, C_MON_PURPLE_B)
    else:  # stage 3
        R(1,  2 + bob, 14, 12, dark)
        R(2,  3 + bob, 12, 10, D)
        R(3,  4 + bob, 10,  8, M)
        R(4,  0 + bob,  8,  3, C_MON_CROWN)
        R(5,  0 + bob,  2,  4, C_MON_CROWN)
        R(9,  0 + bob,  2,  4, C_MON_CROWN)
        R(4,  6 + bob,  3,  2, C_MON_RED_L)
        R(9,  6 + bob,  3,  2, C_MON_RED_L)
        R(0,  4 + bob,  3,  4, dark)
        R(13, 4 + bob,  3,  4, dark)
        R(3,  12,        4,  4, dark)
        R(9,  12,        4,  4, dark)
    return im


def bake_monster_large_frame(evo_stage: int) -> Image.Image:
    """64×64 monster-large cell (2× of 32×32 game pixels = 4× base pixel)."""
    S = SPRITE_SCALE
    im = Image.new("RGBA", (32 * S, 32 * S), (0, 0, 0, 0))
    def R2(x: int, y: int, w: int, hh: int, u16: int) -> None:
        fill_rect_px(im, x * 2 * S, y * 2 * S, w * 2 * S, hh * 2 * S, u16)
    M    = C_MON_PURPLE_M
    D    = C_MON_PURPLE_D
    dark = C_MON_DARK
    if evo_stage == 0:
        R2(4, 6,  8,  6, M);    R2(3, 7, 10,  4, M)
        R2(5, 8,  2,  2, C_EYE); R2(9, 8,  2,  2, C_EYE)
        R2(3, 5,  3,  2, D);    R2(10, 5,  3,  2, D)
    elif evo_stage == 1:
        R2(3, 4, 10,  9, M);    R2(2, 6, 12,  6, M)
        R2(4, 1,  2,  4, D);    R2(10, 1,  2,  4, D)
        R2(5, 6,  2,  2, C_EYE); R2(9, 6,  2,  2, C_EYE)
        R2(4, 12, 3,  3, dark); R2(9, 12,  3,  3, dark)
    elif evo_stage == 2:
        R2(2,  3, 12, 10, D);    R2(1, 5, 14,  7, M)
        R2(3,  4,  4,  3, dark); R2(9, 4,  4,  3, dark)
        R2(4,  6,  3,  2, C_MON_RED);  R2(9, 6, 3, 2, C_MON_RED)
        R2(3, 12,  3,  4, dark); R2(8, 12, 3,  4, dark)
        R2(12, 8,  3,  2, C_MON_PURPLE_B)
    else:  # stage 3
        R2(1,  2, 14, 12, dark); R2(2, 3, 12, 10, D); R2(3, 4, 10, 8, M)
        R2(4,  0,  8,  3, C_MON_CROWN)
        R2(5,  0,  2,  4, C_MON_CROWN); R2(9, 0, 2, 4, C_MON_CROWN)
        R2(4,  6,  3,  2, C_MON_RED_L); R2(9, 6, 3, 2, C_MON_RED_L)
        R2(0,  4,  3,  4, dark); R2(13, 4, 3, 4, dark)
        R2(3, 12,  4,  4, dark); R2(9, 12, 4, 4, dark)
    return im


def bake_furniture_frame(furn_type: int) -> Image.Image:
    """32×32 furniture sprite (2× game pixels). furn_type matches FURN_* enum (1-7)."""
    S = SPRITE_SCALE
    im = Image.new("RGBA", (16 * S, 16 * S), (0, 0, 0, 0))
    def R(x: int, y: int, w: int, hh: int, u16: int) -> None:
        fill_rect_px(im, x * S, y * S, w * S, hh * S, u16)
    if furn_type == FURN_BED:
        R(0,  4, 16, 10, C_PATH_DARK)
        R(1,  5, 14,  8, C_PLAYER_BODY)
        R(0,  4,  5, 10, C_ROCK_LIGHT)
    elif furn_type == FURN_WORKBENCH:
        R(0,  6, 16,  6, C_TREE_TRUNK)
        R(0,  4, 16,  3, C_TREE_MID)
        R(1, 10,  3,  6, C_TREE_TRUNK)
        R(12, 10, 3,  6, C_TREE_TRUNK)
    elif furn_type == FURN_FORGE:
        R(2,  4, 12, 10, C_ROCK_DARK)
        R(3,  2,  4,  4, C_ROCK_MID)
        R(4,  3,  2,  3, C_MON_RED)
        R(3, 10, 10,  2, C_ROCK_MID)
    elif furn_type == FURN_FARM:
        R(0,  8, 16,  6, C_SAND_DARK)
        R(0,  7, 16,  2, C_SAND)
        R(7,  3,  2,  5, C_GRASS_LIGHT)
        R(5,  4,  6,  2, C_TREE_LIGHT)
    elif furn_type == FURN_CHEST:
        R(1,  4, 14, 10, C_TREE_TRUNK)
        R(1,  4, 14,  3, C_TREE_MID)
        R(7,  8,  2,  2, C_GOLD)
    elif furn_type == FURN_PET_BED:
        R(0,  8, 16,  6, C_MON_PURPLE_D)
        R(1,  9, 14,  4, C_MON_PURPLE_M)
    elif furn_type == FURN_PET_HOUSE:
        R(2,  4, 12, 10, C_ROCK_MID)
        R(0,  2, 16,  4, C_ROCK_LIGHT)
        R(6,  8,  4,  6, C_BG_DARK)
    return im


def bake_chars(root: str) -> None:
    """
    assets_chars.png  256 × 192  — 1 row per sprite type, SPRITE_SCALE=2

      Row 0 (y=0,   h=32): SV player ×4           (32px cells, cols 0-3)
      Row 1 (y=32,  h=32): SV tiles ×2  (tree/ore)(32px cells, cols 0-1)
      Row 2 (y=64,  h=32): TD player ×8            (32px cells, cols 0-7)
      Row 3 (y=96,  h=32): monster-small ×8        (32px cells, cols 0-7)
      Row 4 (y=128, h=64): monster-large ×4        (64px cells, cols 0-3)

    Frame index within each row:
      SV player:    walk_frame*2 + (1 if dir_left else 0)  — walk0/1 right, walk0/1 left
      SV tiles:     0=T_TREE  1=T_ORE
      TD player:    dir*2 + walk_frame  (dir: DOWN=0 UP=1 LEFT=2 RIGHT=3)
      monster-sm:   stage*2 + walk_frame
      monster-lg:   stage index (0-3)
    """
    CS = 16 * SPRITE_SCALE   # small cell (32px)
    CL = 32 * SPRITE_SCALE   # large cell (64px)
    W  = 8 * CS              # 256 — widest row is 8 × 32
    H  = 4 * CS + CL         # 192
    sheet = Image.new("RGBA", (W, H), (0, 0, 0, 0))

    # Row 0: SV player (walk0_right, walk1_right, walk0_left, walk1_left)
    for wf in range(2):
        for left in (False, True):
            col = wf + (2 if left else 0)
            cell = bake_sv_player_frame(wf, left)
            sheet.paste(cell, (col * CS, 0 * CS), cell)

    # Row 1: SV tiles (T_TREE=col0, T_ORE=col1)
    for i, tid in enumerate([T_TREE, T_ORE]):
        cell = bake_sv_tile(tid)
        sheet.paste(cell, (i * CS, 1 * CS), cell)

    # Row 2: TD player (dir*2 + walk_frame)
    for d in range(4):
        for wf in range(2):
            cell = bake_td_player_frame(d, wf)
            sheet.paste(cell, ((d * 2 + wf) * CS, 2 * CS), cell)

    # Row 3: monster-small (stage*2 + walk_frame)
    for stage in range(4):
        for wf in range(2):
            cell = bake_monster_frame(stage, wf)
            sheet.paste(cell, ((stage * 2 + wf) * CS, 3 * CS), cell)

    # Row 4: monster-large (64px cells, col = stage)
    for stage in range(4):
        cell = bake_monster_large_frame(stage)
        sheet.paste(cell, (stage * CL, 4 * CS), cell)

    path = asset_out_path(root, "assets_chars.png")
    sheet.save(path)
    print("Wrote", path)


def bake_base(root: str) -> None:
    """
    assets_base.png  224 × 32  (cells at 2× game pixels = SPRITE_SCALE)
      Cols 0-6: FURN_BED … FURN_PET_HOUSE  (type indices 1-7, each 32×32)
    """
    CS = 16 * SPRITE_SCALE
    sheet = Image.new("RGBA", (7 * CS, CS), (0, 0, 0, 0))
    for ft in range(1, 8):
        cell = bake_furniture_frame(ft)
        sheet.paste(cell, ((ft - 1) * CS, 0), cell)
    path = asset_out_path(root, "assets_base.png")
    sheet.save(path)
    print("Wrote", path)


def main() -> None:
    here = os.path.dirname(os.path.abspath(__file__))
    root = os.path.dirname(here)
    bake_actors(root)
    bake_tiles(root)
    bake_mansion(root)
    bake_chars(root)
    bake_base(root)


if __name__ == "__main__":
    main()
