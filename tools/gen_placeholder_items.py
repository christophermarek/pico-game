#!/usr/bin/env python3
"""
gen_placeholder_items.py — generate 16x16 placeholder item icon PNGs.

Run once (or whenever you want fresh placeholders before real art is ready):
    python3 tools/gen_placeholder_items.py

Outputs to assets/sprites/items/<name>.png.
"""

from pathlib import Path
from PIL import Image, ImageDraw

OUT_DIR = Path(__file__).parent.parent / "assets" / "sprites" / "items"
OUT_DIR.mkdir(parents=True, exist_ok=True)

W, H = 16, 16

def new(bg=(0, 0, 0, 0)):
    return Image.new("RGBA", (W, H), bg)

def save(img, name):
    img.save(OUT_DIR / f"{name}.png")
    print(f"  {name}.png")


# ── oak_log ──────────────────────────────────────────────────────────────────
# Brown circle (log cross-section) with concentric rings
def make_oak_log():
    img = new()
    d = ImageDraw.Draw(img)
    d.ellipse([1, 1, 14, 14], fill=(100, 60, 20), outline=(60, 35, 8))
    d.ellipse([3, 3, 12, 12], fill=(139, 86, 35))
    d.ellipse([5, 5, 10, 10], fill=(160, 105, 50))
    d.ellipse([7, 7,  8,  8], fill=(180, 130, 70))
    return img

# ── stone ─────────────────────────────────────────────────────────────────────
# Gray rounded lump
def make_stone():
    img = new()
    d = ImageDraw.Draw(img)
    d.ellipse([1, 3, 14, 13], fill=(100, 100, 110), outline=(65, 65, 75))
    d.ellipse([3, 5,  9,  9], fill=(130, 130, 140))  # highlight
    d.point([(2, 4), (13, 5), (3, 11)], fill=(70, 70, 80))  # shadow dots
    return img

# ── iron_ore ──────────────────────────────────────────────────────────────────
# Dark rock with rust/orange veins
def make_iron_ore():
    img = new()
    d = ImageDraw.Draw(img)
    d.ellipse([1, 2, 14, 13], fill=(60, 50, 45), outline=(35, 28, 22))
    # Ore flecks
    for x, y in [(4, 5), (7, 4), (10, 6), (5, 9), (9, 10), (6, 7)]:
        d.point([(x, y)], fill=(200, 120, 40))
    d.point([(7, 8)], fill=(220, 150, 60))
    return img

# ── raw_fish ──────────────────────────────────────────────────────────────────
# Silver fish body facing right
def make_raw_fish():
    img = new()
    d = ImageDraw.Draw(img)
    # Body ellipse
    d.ellipse([2, 4, 12, 11], fill=(140, 170, 200), outline=(80, 120, 160))
    # Tail triangle (left side)
    d.polygon([(2, 4), (2, 11), (0, 2)],  fill=(100, 140, 175))
    d.polygon([(2, 4), (2, 11), (0, 13)], fill=(100, 140, 175))
    # Belly highlight
    d.ellipse([4, 6, 10, 9], fill=(200, 220, 240))
    # Eye
    d.point([(11, 6)], fill=(20, 20, 20))
    return img

# ── coin ──────────────────────────────────────────────────────────────────────
# Gold circle with shine spot
def make_coin():
    img = new()
    d = ImageDraw.Draw(img)
    d.ellipse([1, 1, 14, 14], fill=(180, 140, 20), outline=(120, 90, 10))
    d.ellipse([3, 3, 12, 12], fill=(210, 170, 40))
    # Shine spot
    d.ellipse([4, 4,  7,  6], fill=(240, 220, 120))
    # Inner ring detail
    d.arc([5, 5, 11, 11], start=30, end=150, fill=(150, 110, 15))
    return img

# ── axe ───────────────────────────────────────────────────────────────────────
# Handle + wide axe head
def make_axe():
    img = new()
    d = ImageDraw.Draw(img)
    # Handle (vertical, left of center)
    d.rectangle([6, 1, 8, 14], fill=(130, 90, 50), outline=(80, 55, 25))
    # Axe head (to the right)
    d.polygon([
        (8,  1), (14, 3),
        (14, 10), (8, 12),
        (8,  1)
    ], fill=(160, 170, 180), outline=(100, 110, 120))
    # Edge bevel
    d.line([(14, 3), (14, 10)], fill=(220, 230, 240), width=1)
    return img

# ── pickaxe ───────────────────────────────────────────────────────────────────
# Angled pick head + handle
def make_pickaxe():
    img = new()
    d = ImageDraw.Draw(img)
    # Handle (diagonal)
    d.line([(7, 14), (11, 8)], fill=(130, 90, 50), width=2)
    # Pick head (horizontal beam)
    d.rectangle([1, 5, 14, 8], fill=(150, 160, 170), outline=(90, 100, 110))
    # Pick tip left (sharpened)
    d.polygon([(1, 5), (1, 8), (0, 6)], fill=(180, 190, 200))
    # Pick tip right
    d.polygon([(14, 5), (14, 8), (15, 4)], fill=(180, 190, 200))
    return img

# ── fishing_rod ───────────────────────────────────────────────────────────────
# Diagonal pole with line + hook
def make_fishing_rod():
    img = new()
    d = ImageDraw.Draw(img)
    # Rod (diagonal: top-right to bottom-left)
    d.line([(13, 1), (3, 11)], fill=(120, 85, 45), width=2)
    # Fishing line (thin, from tip)
    d.line([(13, 1), (14, 7)], fill=(200, 210, 220), width=1)
    # Hook (small curve at end of line)
    d.arc([12, 7, 15, 10], start=270, end=180, fill=(180, 180, 190))
    # Handle grip
    d.ellipse([2, 10, 5, 13], fill=(150, 105, 60), outline=(90, 60, 30))
    return img

# ── shears ────────────────────────────────────────────────────────────────────
# Two crossing blades
def make_shears():
    img = new()
    d = ImageDraw.Draw(img)
    # Left blade (top-left to bottom-right)
    d.line([(2,  2), (14, 14)], fill=(170, 175, 180), width=2)
    # Right blade (top-right to bottom-left)
    d.line([(14, 2), (2, 14)], fill=(170, 175, 180), width=2)
    # Pivot ring
    d.ellipse([6, 6, 9, 9], fill=(130, 135, 140), outline=(90, 95, 100))
    # Blade edges highlight
    d.line([(2, 1), (13, 13)], fill=(210, 215, 220), width=1)
    d.line([(14, 1), (3, 13)], fill=(210, 215, 220), width=1)
    # Handle circles (bottom)
    d.ellipse([0, 11, 4, 15], fill=(140, 90, 50), outline=(90, 55, 25))
    d.ellipse([11, 11, 15, 15], fill=(140, 90, 50), outline=(90, 55, 25))
    return img


if __name__ == "__main__":
    print(f"Writing placeholders to {OUT_DIR.relative_to(Path.cwd()) if OUT_DIR.is_relative_to(Path.cwd()) else OUT_DIR}/")
    save(make_oak_log(),     "oak_log")
    save(make_stone(),       "stone")
    save(make_iron_ore(),    "iron_ore")
    save(make_raw_fish(),    "raw_fish")
    save(make_coin(),        "coin")
    save(make_axe(),         "axe")
    save(make_pickaxe(),     "pickaxe")
    save(make_fishing_rod(), "fishing_rod")
    save(make_shears(),      "shears")
    print("Done.")
