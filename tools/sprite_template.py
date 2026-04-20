#!/usr/bin/env python3
"""
sprite_template.py — Generate blank/template sprite sheets for manual pixel art editing

Usage:
    python3 tools/sprite_template.py

This creates empty PNG templates in assets/ with grid guides.
After editing the PNGs manually, they're loaded by the game at runtime.
"""

import os
from PIL import Image, ImageDraw

def create_template(path, width, height, cell_w, cell_h, labels=None):
    """Create a template PNG with grid guides and optional cell labels."""
    img = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    # Draw grid lines (light gray, semi-transparent)
    grid_color = (128, 128, 128, 64)
    for x in range(0, width + 1, cell_w):
        draw.line([(x, 0), (x, height)], fill=grid_color, width=1)
    for y in range(0, height + 1, cell_h):
        draw.line([(0, y), (width, y)], fill=grid_color, width=1)
    
    # Add cell labels if provided
    if labels:
        text_color = (255, 255, 255, 128)
        for row, col, text in labels:
            x = col * cell_w + 4
            y = row * cell_h + 4
            draw.text((x, y), text, fill=text_color)
    
    img.save(path)
    print(f"Created template: {path} ({width}×{height})")


def main():
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    assets_dir = os.path.join(root, "assets", "templates")
    os.makedirs(assets_dir, exist_ok=True)
    
    # Isometric tiles: 4×4 grid of 64×48 cells
    tile_labels = [
        # Row 0: water animation frames
        (0, 0, "water0"), (0, 1, "water1"), (0, 2, "water2"), (0, 3, "water3"),
        # Row 1: basic terrain
        (1, 0, "grass"), (1, 1, "path"), (1, 2, "sand"), (1, 3, "tree"),
        # Row 2: resources
        (2, 0, "rock"), (2, 1, "ore"), (2, 2, "flower"), (2, 3, "tgrass"),
        # Row 3: special
        (3, 0, "depleted"), (3, 1, "---"), (3, 2, "---"), (3, 3, "---"),
    ]
    create_template(
        os.path.join(assets_dir, "tiles_template.png"),
        256, 192, 64, 48, tile_labels
    )
    
    # Character sprites: 8×1 grid of 32×32 cells (TD player walk frames)
    char_labels = [
        (0, 0, "d0"), (0, 1, "d1"), (0, 2, "u0"), (0, 3, "u1"),
        (0, 4, "l0"), (0, 5, "l1"), (0, 6, "r0"), (0, 7, "r1"),
    ]
    create_template(
        os.path.join(assets_dir, "chars_template.png"),
        256, 32, 32, 32, char_labels
    )
    
    print("\n✓ Templates created in assets/templates/")
    print("  Edit these with your pixel art tool, then copy to assets/")
    print("  Grid guides are semi-transparent and can be drawn over")


if __name__ == "__main__":
    main()
