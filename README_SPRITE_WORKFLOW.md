# Sprite Editing Quick Start

## I Want To Edit Sprites!

```bash
# 1. Open the sprite PNGs in your pixel art editor
open assets/assets_iso_tiles.png    # Terrain tiles (4×4 grid)
open assets/assets_chars.png        # Character walk animations

# 2. Edit the sprites
# - Stay within the grid cells
# - Use transparency for empty pixels
# - Tiles: 64×48 pixels
# - Characters: 32×32 pixels

# 3. Test immediately (no rebuild needed!)
make dev

# That's it!
```

## Grid Layout

### Tiles (`assets_iso_tiles.png` - 256×192)
```
┌─────────┬─────────┬─────────┬─────────┐
│ water0  │ water1  │ water2  │ water3  │  Row 0: Water animation
├─────────┼─────────┼─────────┼─────────┤
│ grass   │ path    │ sand    │ tree    │  Row 1: Basic terrain
├─────────┼─────────┼─────────┼─────────┤
│ rock    │ ore     │ flower  │ tgrass  │  Row 2: Resources
├─────────┼─────────┼─────────┼─────────┤
│ deplete │ (empty) │ (empty) │ (empty) │  Row 3: Special
└─────────┴─────────┴─────────┴─────────┘
```

Each cell is 64×48 pixels. Just edit in place!

### Characters (`assets_chars.png` - 256×32)
```
┌────┬────┬────┬────┬────┬────┬────┬────┐
│ d0 │ d1 │ u0 │ u1 │ l0 │ l1 │ r0 │ r1 │  Walk frames
└────┴────┴────┴────┴────┴────┴────┴────┘
```

Each cell is 32×32 pixels. d=down, u=up, l=left, r=right, 0/1=walk frame.

## Starting from Scratch?

```bash
# Generate blank templates with grid guides
python3 tools/sprite_template.py

# Templates appear in assets/templates/
# Copy to assets/ and start drawing
```

## Tools

- **Aseprite** - Professional pixel art tool (paid, best)
- **GraphicsGale** - Free, Windows/Wine
- **Piskel** - Free, web-based
- **GIMP** - Free, but less pixel-art friendly

## More Info

See [`SPRITES.md`](SPRITES.md) for complete documentation.

---

**TL;DR:** Just edit `assets/*.png` directly. The game will use your changes instantly.
