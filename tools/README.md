# Tools

### `gen_spritesheets.py`

Assembles individual per-tile / per-frame PNGs under `assets/sprites/` into
the two sheets the runtime loads (`build/assets_iso_tiles.png`,
`build/assets_chars.png`). Driven by `assets/sprites/manifest.json`.

Runs automatically from `make build` / `make dev` whenever a source sprite
changes. Manual run:

```bash
python3 tools/gen_spritesheets.py [project_root] [--out-dir build]
```

### `gen_pico_atlases.py`

Converts the assembled sheets into C arrays (`src/pico_assets/atlas_*.c`)
that the Pico hardware build links into flash. Runs automatically from the
Pico CMake build.

```bash
python3 tools/gen_pico_atlases.py <project_root>
```

### `map_editor.html`

Browser map editor for `assets/maps/map.bin`. Start via `make editor`.

## Workflow

1. Edit a sprite: `open assets/sprites/tiles/tree.png`
2. `make dev` — sheets re-assemble, game reloads.
3. For Pico: `cd pico && cmake .. && make` — atlases re-generated into flash.

See [../SPRITES.md](../SPRITES.md) for the full sprite workflow.
