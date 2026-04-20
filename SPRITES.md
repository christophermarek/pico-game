# Sprite Workflow

GrumbleQuest uses PNG sprite sheets that you edit directly. The game loads them at runtime.

## Quick Start

```bash
# 1. Edit sprites directly in your pixel art tool
open assets/assets_iso_tiles.png
open assets/assets_chars.png

# 2. Test immediately (no rebuild needed!)
make dev
```

That's it! Changes are reflected instantly.

## Sprite Sheet Organization

### Isometric Tiles (`assets_iso_tiles.png`)
**Size:** 256×192 (4×4 grid of 64×48 cells)

```
Row 0: water0  water1  water2  water3
Row 1: grass   path    sand    tree
Row 2: rock    ore     flower  tgrass
Row 3: deplete ---     ---     ---
```

- **Water:** 4 animation frames (auto-cycles every 4 game ticks)
- **Overlays:** tree, rock, ore, flower render on grass base
- **Depleted:** shown over harvested resource nodes

### Character Sprites (`assets_chars.png`)
**Size:** 256×32 (8×1 grid of 32×32 cells)

```
Row 0: down0 down1 up0 up1 left0 left1 right0 right1
```

Top-down player walk animation (2 frames per direction).

## Creating New Sprites from Scratch

```bash
# Generate blank templates with grid guides
python3 tools/sprite_template.py

# Templates appear in assets/templates/
# Copy to assets/ and start drawing
```

## Editing Tips

1. **Grid Alignment:** Each tile must stay within its cell boundaries
2. **Transparency:** Use alpha channel for empty pixels
3. **Instant Feedback:** Just run `make dev` after editing
4. **Backup:** Copy `assets/` before major changes
5. **Format:** PNG with alpha channel, any color depth

## File Sizes

| File | Dimensions | PNG | Flash (Pico) |
|------|------------|-----|--------------|
| `assets_iso_tiles.png` | 256×192 | ~2 KB | 192 KB |
| `assets_chars.png` | 256×32 | ~393 B | 32 KB |
| **Total** | | ~2.4 KB | **224 KB** |

Pico has 2 MB flash - sprites use ~11%, leaving 1824 KB for code.

## Adding New Tiles

To add a new terrain tile:

1. **Edit PNG:** Open `assets_iso_tiles.png`, add sprite to unused cell (e.g., Row 3, Col 1-3)

2. **Add Constant:** In `src/config.h`:
   ```c
   #define T_NEWTILE 10
   ```

3. **Map Sprite:** In `src/render/iso_spritesheet.c`, in `iso_tile_frame()`:
   ```c
   case T_NEWTILE: col = 1; row = 3; break;
   ```

4. **Add to Validation:** In `iso_tile_id_on_sheet()`:
   ```c
   case T_NEWTILE:
   ```

5. **Use in World:** In `src/game/world.c`, use `T_NEWTILE` in map generation

## Build System

### Desktop (SDL)
The game loads PNGs at runtime using `stb_image.h`. No build step needed.

### Pico (Hardware)
PNGs are converted to C arrays during the build:

```bash
# Automatically runs during Pico build
python3 tools/gen_pico_atlases.py .
```

This creates:
- `src/pico_assets/atlas_iso_tiles.c` (192 KB)
- `src/pico_assets/atlas_chars.c` (32 KB)

The Pico firmware includes these arrays in flash.

## Tools

- **`tools/sprite_template.py`** - Generate blank templates with grid guides (useful!)
- **`tools/gen_pico_atlases.py`** - Convert PNGs → C arrays for Pico (needed for builds)

## Workflow

1. Edit `assets/*.png` in your pixel art editor (Aseprite, GraphicsGale, Piskel, etc.)
2. Test on desktop: `make dev` (instant feedback)
3. Commit PNGs to git
4. Pico build auto-converts PNGs to C arrays

**The PNGs are your source of truth.** Edit them directly!
