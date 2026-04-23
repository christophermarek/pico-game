# Sprite Workflow

GrumbleQuest edits **individual** sprite PNGs under `assets/sprites/`.
`make build` (or `make dev`) runs `tools/gen_spritesheets.py` to assemble
them into two sheets under `build/` — `build/assets_iso_tiles.png` and
`build/assets_chars.png` — which the runtime loads.

```bash
# Edit a tile
open assets/sprites/tiles/tree.png

# Build & run — sheets are re-assembled automatically
make dev
```

## Layout

### `assets/sprites/tiles/` → `build/assets_iso_tiles.png` (256×192, 64×48 cells)

```
Row 0: water_0  water_1  water_2  water_3
Row 1: grass    path     sand     tree
Row 2: rock     ore      flower   tgrass
Row 3: depleted
```

Water is a 4-frame animation cycling every 15 frames (~0.5 s at 30 FPS).
Overlays (`tree`, `rock`, `ore`, `flower`, `tgrass`) render on top of a grass
(or path, for ore) floor tile.

### `assets/sprites/chars/` → `build/assets_chars.png` (256×32, 32×32 cells)

```
Row 0: player_down_0  player_down_1
       player_up_0    player_up_1
       player_left_0  player_left_1
       player_right_0 player_right_1
```

Stored at 2× game pixels, drawn at 1:1 via nearest-neighbour downsample.

### Manifest

`assets/sprites/manifest.json` maps cells → sprite paths. Add rows or change
cell dimensions here; `gen_spritesheets.py` reads this on every build.

## Adding a new tile

1. Drop the PNG in `assets/sprites/tiles/my_tile.png` at 64×48.
2. Add it to `manifest.json` in an empty cell.
3. `#define T_MYTILE N` in `src/config.h`.
4. Route the id in `iso_tile_frame()` and `iso_tile_id_on_sheet()` inside
   `src/render/iso_spritesheet.c`.
5. Use `T_MYTILE` in the map.

## Pico build

`tools/gen_pico_atlases.py` converts the assembled sheets into C arrays
(`src/pico_assets/atlas_*.c`) linked into flash. The CMake Pico build runs
this automatically when the PNGs change.

## Blank templates

```bash
python3 tools/sprite_template.py
```

Produces `assets/templates/*.png` with a semi-transparent grid overlay — a
starting point for drawing new sheets from scratch.
