#!/usr/bin/env python3
"""
gen_spritesheets.py — assemble sprite sheets from individual PNG files.

Each sprite lives at assets/sprites/<name>.png at its natural cell size.
The manifest at assets/sprites/manifest.json describes every sheet:

  {
    "sheets": [
      {
        "output": "assets/assets_iso_tiles.png",
        "cell_w": 64,
        "cell_h": 48,
        "grid": [
          ["tiles/water_0", "tiles/water_1", ...],
          ["tiles/grass",   null, ...]
        ]
      },
      ...
    ]
  }

  "grid" is a 2-D array (rows × cols).  Each entry is either:
    - a path relative to assets/sprites/ (without the .png extension), or
    - null  →  that cell is left transparent.

Usage:
    python3 tools/gen_spritesheets.py [project_root] [--out-dir DIR]

project_root defaults to the directory containing this script's parent.
Sheets are written to project_root/DIR/<output> (default DIR=build), so the
generated PNGs live alongside build artefacts instead of being committed to
assets/. The manifest's "output" should be a plain basename.

Exits with code 1 if any required sprite file is missing.
"""

import argparse
import json
import sys
from pathlib import Path
from PIL import Image


def load_sprite(path: Path, cell_w: int, cell_h: int) -> Image.Image:
    img = Image.open(path).convert("RGBA")
    if img.size != (cell_w, cell_h):
        print(f"  WARNING: {path.name} is {img.size}, expected ({cell_w}×{cell_h}) — resizing")
        img = img.resize((cell_w, cell_h), Image.NEAREST)
    return img


def build_sheet(sheet: dict, sprites_dir: Path, out_dir: Path, root: Path) -> None:
    output   = out_dir / sheet["output"]
    cell_w   = sheet["cell_w"]
    cell_h   = sheet["cell_h"]
    grid     = sheet["grid"]

    rows = len(grid)
    cols = max(len(row) for row in grid)

    canvas = Image.new("RGBA", (cols * cell_w, rows * cell_h), (0, 0, 0, 0))

    missing = []
    for r, row in enumerate(grid):
        for c, entry in enumerate(row):
            if entry is None:
                continue
            sprite_path = sprites_dir / f"{entry}.png"
            if not sprite_path.exists():
                missing.append(str(sprite_path))
                continue
            sprite = load_sprite(sprite_path, cell_w, cell_h)
            canvas.paste(sprite, (c * cell_w, r * cell_h))

    if missing:
        for m in missing:
            print(f"  ERROR: missing sprite: {m}")
        sys.exit(1)

    output.parent.mkdir(parents=True, exist_ok=True)
    canvas.save(output)
    size_kb = output.stat().st_size / 1024
    print(f"  {output.relative_to(root)}  ({cols}×{rows} cells @ {cell_w}×{cell_h})  {size_kb:.1f} KB")


def main() -> None:
    script_dir = Path(__file__).resolve().parent
    default_root = script_dir.parent

    parser = argparse.ArgumentParser()
    parser.add_argument("root", nargs="?", default=str(default_root))
    parser.add_argument("--out-dir", default="build")
    args = parser.parse_args()

    root = Path(args.root).resolve()
    out_dir = (root / args.out_dir).resolve()

    sprites_dir  = root / "assets" / "sprites"
    manifest_path = sprites_dir / "manifest.json"

    if not manifest_path.exists():
        print(f"ERROR: manifest not found at {manifest_path}")
        sys.exit(1)

    with open(manifest_path) as f:
        manifest = json.load(f)

    out_dir.mkdir(parents=True, exist_ok=True)
    print(f"Building sprite sheets from {sprites_dir.relative_to(root)}/ → {out_dir.relative_to(root)}/")
    for sheet in manifest["sheets"]:
        build_sheet(sheet, sprites_dir, out_dir, root)
    print("Done.")


if __name__ == "__main__":
    main()
