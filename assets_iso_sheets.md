# PNG Sprite Sheets (Top-down Iso)

The SDL simulator loads these files at runtime (each path is tried as `./<path>` then `../<path>` when the working directory is `build/`):

- `assets/assets_iso_tiles.png`
- `assets/assets_chars.png`

## Editing Sprites

Edit `assets/*.png` directly with your pixel art tool (Aseprite, GraphicsGale, Piskel, etc.). Changes are reflected immediately when you run the game (no rebuild needed).

```bash
# Edit sprites directly
open assets/assets_iso_tiles.png
open assets/assets_chars.png

# Test your changes
make dev

# Generate blank templates (optional)
python3 tools/sprite_template.py  # Creates assets/templates/*.png
```

See [`SPRITES.md`](SPRITES.md) for the full sprite workflow guide.

## Requirements

- **Simulator and unit tests** load these PNGs via **stb_image** ([`third_party/stb_image.h`](third_party/stb_image.h)). If a file is missing or unreadable, iso sprites are simply not drawn.
- **Pico hardware** embeds the PNG data as C arrays via `tools/gen_pico_atlases.py`.

## terrain sheet

- File: [`assets/assets_iso_tiles.png`](assets/assets_iso_tiles.png)
- Size: `256×192` (4×4 grid of 64×48 cells)
- Anchor: cell centre `(cx, cy)` in screen space (offset `ox=-32`, `oy=-24`)
- Layout:
  - Row 0: `T_WATER` animation frames (0-3)
  - Row 1: `T_GRASS`, `T_PATH`, `T_SAND`, `T_TREE`
  - Row 2: `T_ROCK`, `T_ORE`, `T_FLOWER`, `T_TGRASS`
  - Row 3: Depleted-node overlay (col 0), unused (cols 1-3)

## chars sheet

- File: [`assets/assets_chars.png`](assets/assets_chars.png)
- Size: `256×32`
- Stored at 2× game pixels (SPRITE_SCALE=2), rendered at 1:1 via `draw_png_frame_scaled(..., 1, 2)`
- Layout:
  - Row 0 (y=0): Top-down player character sprites (8 frames: 4 dirs × 2 walk frames)

## Notes

- PNG alpha is respected (transparent pixels are skipped).
- Sprites are hand-edited in `assets/*.png` - these are the source of truth.
- Desktop loads PNGs at runtime; Pico embeds them as C arrays during build.
