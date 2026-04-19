# PNG Sprite Sheets (Top-down Iso)

The SDL simulator loads these files at runtime (each path is tried as `./<path>` then `../<path>` when the working directory is `build/`):

- `assets/assets_iso_actors.png`
- `assets/assets_iso_tiles.png`
- `assets/assets_iso_mansion.png`

## Regenerating

The bake script writes all three PNGs into the **`assets/`** directory:

```bash
pip install pillow   # once
python3 tools/bake_iso_assets.py
```

## Requirements

- **Simulator and unit tests** load these PNGs via **stb_image** ([`third_party/stb_image.h`](third_party/stb_image.h)). If a file is missing or unreadable, iso sprites are simply not drawn — for the mansion, the procedural C fallback in `spr_mansion_td` is used instead.
- **Pico hardware** still has `hal_image_load_rgba` as a stub, so the procedural fallback runs on device.

## terrain sheet

- File: [`assets/assets_iso_tiles.png`](assets/assets_iso_tiles.png)
- Size: `768×48` (one row of 12 cells × `64×48`)
- Anchor: cell centre `(cx, cy)` in screen space (offset `ox=-32`, `oy=-24`)
- Columns 0–3: `T_WATER` animation frames (phase `0..3`, same as `(tick/4)%4` in game)
- Columns 4–11: `T_GRASS`, `T_PATH`, `T_SAND`, `T_TREE`, `T_ROCK`, `T_ORE`, `T_FLOWER`, `T_TGRASS`

`T_HOME` / `T_HOME_SOLID` under the mansion footprint render as grass; a path strip (column `MANSION_DOOR_TX`) runs up to the door tile (see `world.c`).

## mansion sheet

- File: [`assets/assets_iso_mansion.png`](assets/assets_iso_mansion.png)
- Size: `768×128` (four `192×128` frames, one per camera bearing 0–3)
- Anchor: pixel `(96, 56)` within each frame maps to the footprint centre `(MV_CX, MV_CY)` in world space. `iso_draw_mansion()` receives the screen projection of that point and shifts by `(-96, -56)` to position the frame.
- Bearing 0 → column 0; bearing 1 → column 192; bearing 2 → column 384; bearing 3 → column 576
- Content: plinth, lit/shaded walls, windows, door (south wall, bearings 0 and 3), pyramidal roof, chimney with smoke
- Fallback: if the PNG is absent, `spr_mansion_td` falls through to the procedural C renderer

## actors sheet

- File: [`assets/assets_iso_actors.png`](assets/assets_iso_actors.png)
- Size: `128×64`
- Cell size: `32×32` source pixels; drawn at **75%** (`ISO_ACTOR_SN/SD` in `iso_spritesheet.c`)
- Layout:
  - Row 0: player frames — down, up, left, right
  - Row 1: monster evo 0, evo 1, evo 2, (unused)

## Notes

- PNG alpha is respected (transparent pixels are skipped).
- Art in all sheets is generated to match the procedural rendering in `sprites.c` / `bake_iso_assets.py`.
