# Tools

## Sprite Tools

### `sprite_template.py`
Generates blank PNG templates with grid guides for creating new sprites from scratch.

```bash
python3 tools/sprite_template.py
```

Creates templates in `assets/templates/`:
- `tiles_template.png` - 4×4 grid for terrain tiles
- `chars_template.png` - 8×1 grid for character animations

Copy to `assets/` and edit.

### `gen_pico_atlases.py`
Converts PNG sprite sheets to C arrays for Pico builds. **Automatically runs during Pico builds** - you don't need to run this manually.

```bash
python3 tools/gen_pico_atlases.py .
```

Reads from `assets/*.png` and generates:
- `src/pico_assets/atlas_iso_tiles.c` (192 KB)
- `src/pico_assets/atlas_chars.c` (32 KB)
- `src/pico_assets/pico_atlases.h`

## Workflow

1. Edit sprites: Open `assets/*.png` in your pixel art tool
2. Test on desktop: `make dev` (loads PNGs at runtime)
3. Build for Pico: `cd pico && cmake .. && make` (auto-converts PNGs to C arrays)

The PNG files are your source of truth!
