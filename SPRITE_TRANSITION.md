# Sprite System Transition Complete ✅

## What Changed

We've **fully transitioned from procedural art generation to sprite-based rendering**. The PNG files in `assets/` are now the source of truth.

## Deleted (40+ KB of procedural code)

- ❌ `tools/bake_iso_assets.py` (34 KB) - Complex procedural art generator
- ❌ `tools/bake_iso_assets_simple.py` (3 KB) - Simplified wrapper
- ❌ `tools/generate_sprites.py` (1.5 KB) - One-time runner
- ❌ `assets/assets_iso_actors.png` - Unused foot sprites
- ❌ All procedural rendering functions and constants

## Kept (Minimal, essential tools)

- ✅ `tools/sprite_template.py` - Generate blank templates (useful for new sprites)
- ✅ `tools/gen_pico_atlases.py` - PNG → C arrays for Pico (required for builds)
- ✅ `assets/assets_iso_tiles.png` (256×192, 4×4 grid)
- ✅ `assets/assets_chars.png` (256×32, 8×1 grid)

## New Workflow

### Edit Sprites
```bash
# 1. Open in your pixel art tool
open assets/assets_iso_tiles.png
open assets/assets_chars.png

# 2. Edit directly

# 3. Test immediately
make dev
```

### Create New Sprites
```bash
# Generate blank templates with grid guides
python3 tools/sprite_template.py

# Copy from assets/templates/ to assets/ and edit
```

### Build for Pico
```bash
cd pico && mkdir build && cd build
cmake -DPICO_SDK_PATH=/path/to/pico-sdk ..
make
# PNGs are automatically converted to C arrays
```

## Benefits

1. **Simpler:** No complex procedural generation to maintain
2. **Faster iteration:** Edit PNGs, run `make dev`, see changes instantly
3. **More flexible:** Full control over every pixel
4. **Smaller codebase:** Removed 40+ KB of procedural art code
5. **Standard workflow:** Uses industry-standard pixel art tools

## File Sizes

| Component | Size |
|-----------|------|
| **PNG files** | 2.4 KB |
| **Pico flash (embedded)** | 224 KB |
| **Code saved** | 40+ KB deleted |
| **Free flash for code** | 1824 KB (89%) |

## Documentation

- [`SPRITES.md`](SPRITES.md) - Complete sprite workflow guide
- [`README_SPRITE_WORKFLOW.md`](README_SPRITE_WORKFLOW.md) - Quick start
- [`tools/README.md`](tools/README.md) - Tool documentation

## Migration Notes

This file documents the transition. You can delete it once you're familiar with the new workflow.

The PNGs in `assets/` are your **source of truth** now. Edit them directly!
