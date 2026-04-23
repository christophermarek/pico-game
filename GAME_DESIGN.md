# GrumbleQuest — Game Design Document

**Genre:** Resource-Gathering RPG  
**Platform:** Raspberry Pi Pico (ST7789 240×240 display) + SDL2 Desktop Simulator  
**Engine:** Custom C11 (zero dependencies except SDL2 for sim)  
**Status:** Playable prototype with core systems implemented

---

## Overview

**GrumbleQuest** is a pixel-art top-down RPG focused on exploration and resource gathering. Players explore a handcrafted isometric overworld, harvest resources through 3 skills, and manage their inventory.

The game combines:
- **Skilling system** (3 skills: Mining, Fishing, Woodcutting)
- **Resource gathering** (8 item types)
- **Persistent save system** with auto-save every minute

---

## Core Loop

1. **Explore** the 30×20 tile overworld
2. **Gather resources** by skilling at nodes (trees, rocks, ore, water)
3. **Level up skills** through XP earned from each action
4. **Manage inventory** via the in-game menu

---

## World Design

### Top-Down Overworld (30×20 tiles, 16px each)

**Key Locations:**
- **Path Network** — East-west road at y=12, north-south spine at x=15
- **Lake** — Northeast (elliptical water body with sand border, fishing spot)
- **Forest** — Southwest (dense tree cluster with clearings, woodcutting nodes)
- **Rock Quarry** — Southeast (stone and ore mining)
- **Tall Grass Zones** — Three decorative meadow areas (northwest, east, south)
- **Resource Nodes** — Trees, rocks, ore veins, water (respawn after 60 ticks ≈ 10 min)

**Terrain Types:**
- Grass (walkable, base terrain)
- Path (walkable, fast travel aesthetic)
- Water (non-walkable, fishing)
- Sand (walkable, decorative)
- Trees (non-walkable, woodcutting resource)
- Rocks (non-walkable, mining resource)
- Ore (non-walkable, rare mining resource)
- Tall Grass (walkable, decorative)
- Flowers (walkable, decorative)

---

## Skills System (3 Total)

| Skill | Nodes | Items Dropped | XP Range |
|-------|-------|---------------|----------|
| **Mining** | Rock, Ore | Stone, Ore, Gem | 30–50 |
| **Fishing** | Water | Fish, Seaweed | 20–30 |
| **Woodcutting** | Tree | Log, Branch | 25–40 |

**Skilling Flow:**
1. Face a resource node (tree, rock, ore, water)
2. Press **A** to start action
3. Action completes in 30 frames (1 second at 30 FPS)
4. Skill XP awarded, item added to inventory, node depleted
5. Node respawns after 60 ticks (≈10 min real time)

**XP → Level Formula:**
```c
xp_for_level(n) = n² × 10 + n × 20
```

---

## Inventory & Items (8 Types)

| Item | Source |
|------|--------|
| Ore | Mining (rock/ore nodes) |
| Stone | Mining (rock nodes) |
| Gem | Mining (ore nodes, rare) |
| Fish | Fishing |
| Seaweed | Fishing (rare) |
| Log | Woodcutting |
| Branch | Woodcutting (rare) |
| Bread | Starting inventory |

- 20 inventory slots, items stack up to 255
- Start with 3 Bread

---

## UI & Modes

### Game Modes

| Mode | Description | Controls |
|------|-------------|----------|
| **TOPDOWN** | Overworld exploration | Arrows/WSQD to move, A to interact, B to run, M/Tab for menu |
| **MENU** | Inventory/skills | Arrows/Left-Right tabs, B to close |

### HUD Elements

**Bottom Strip (18px):**
- Player HP bar (green)
- Energy bar (blue, drains while running, regenerates when idle)
- Recent log message or "Day X" when idle

**Tab shortcut icons (right side):** BG (Items), SK (Skills)

### Menu Tabs (Press M/Tab)

1. **SKILLS** — View all 4 skill levels and XP progress bar
2. **ITEMS** — 20-slot inventory grid with item names and counts

---

## Controls

### Desktop Simulator (SDL2)

| Action | Keys |
|--------|------|
| Move | Arrow Keys or W/S/Q/D |
| Confirm / Interact (A) | Z, Space, Enter, or A |
| Cancel / Run (B) | X or B |
| Menu open / Tab cycle | M or Tab |
| Rotate camera | `[` / `]` |

**Note:** Keyboard A is mapped to the face button A (confirm), not left movement. Use Q for strafe-left. Hold B to run (drains energy).

### Pico Hardware (Planned)

- **Directional:** GPIO 2–5 (UP/DOWN/LEFT/RIGHT)
- **Face buttons:** GPIO 6–7 (A/B)
- **System:** GPIO 8–9 (START/SELECT)
- **Display:** SPI0 (ST7789 240×240 RGB565)

---

## Save System

**Format:** Binary file (`grumble_save.bin` on desktop, flash sector on Pico)  
**Version:** 6

**Auto-Save:** Every 6 game ticks (~1 minute of real time)  
**Load:** On startup; if invalid/missing, starts a fresh game

**Saved Data:**
- Player stats (HP, energy, position)
- All 4 skill levels and XP
- Full inventory (20 slots)
- World node respawn timers
- Total steps
- Current day

---

## Technical Architecture

### Hardware Abstraction Layer (HAL)

**Interface:** `hal.h`  
**Implementations:**
- `hal_sdl.c` — Desktop simulator (SDL2, 3× scale, 720×720 window)
- `hal_pico.c` — Raspberry Pi Pico (ST7789 SPI display, GPIO buttons)
- `hal_stub.c` — Headless test harness (RAM framebuffer, no SDL)

**Abstraction:**
- Display (240×240 RGB565 framebuffer)
- Input (8 buttons: up/down/left/right/A/B/start/select)
- Time (millisecond ticks, sleep)
- Persistence (save/load to file or flash)

### Project Structure

```
PicoGame/
├── src/
│   ├── hal.h                 # Hardware abstraction interface
│   ├── hal_sdl.c             # SDL2 simulator implementation
│   ├── hal_pico.c            # Pico hardware implementation
│   ├── config.h              # Game constants and tuning
│   ├── colors.h              # RGB565 color palette
│   ├── main.c                # Game loop
│   ├── game/                 # state, world, player, skills, tick, save
│   ├── render/               # renderer, iso_spritesheet, font
│   └── ui/                   # hud, menu
├── tests/                    # Headless HAL + integration tests
├── tools/                    # sprite & atlas generators, map editor
├── pico/CMakeLists.txt       # Pico hardware build
├── CMakeLists.txt            # Simulator + test build
└── Makefile                  # Convenience targets
```

### Build System

```bash
make build   # Compile sim + tests
make test    # Run test suite
make dev     # Run simulator (blocking)
make clean   # Remove build/
```

---

## Art Style

**Palette:** RGB565 (5:6:5 bit depth, ST7789 native)  
**Resolution:** 240×240 effective (scaled 3× in sim)  
**Tile Size:** 16×16 pixels  
**Style:** Low-res pixel art, isometric top-down

**Animations:**
- Water waves (4-frame cycle)
- Walk animation (2-frame bob)
- Skill progress bar (shown while gathering)

---

## Performance & Constraints

### Target Specs
- **CPU:** RP2040 dual-core ARM Cortex-M0+ @ 133 MHz
- **RAM:** 264 KB SRAM
- **Flash:** 2 MB (code + save data)
- **Display:** ST7789V 240×240 SPI @ 62.5 MHz

### Optimizations
- RGB565 framebuffer (no conversion overhead)
- Direct SPI writes (DMA on Pico)
- Static allocations (no heap)
- Tile-based culling (only draw visible region)
- 30 FPS target (33ms frame budget)

---

## Future Expansion

### Planned Features
- **Home Base / Mansion** — Enter the player's mansion as a dedicated room (see design notes below)
- **Side-View Platformer** — Toggle between top-down and a scrolling platformer with physics (see design notes below)
- **Crafting System** — Equipment recipes crafted at workbench (see design notes below)
- **Farming** — Berry plots at home base with growth timers (see design notes below)
- **Hunger system** — Hunger decay per tick, starvation HP loss, food restoration
- **Cooking system** — Combine ingredients into meals at the cooking station
- **Magic system** — Spell casting with spell items
- **Smithing** — Iron and rune equipment at the forge
- **Herblore** — Potion crafting from herbs
- **Floating XP drops** — Rising "+42 XP" text shown on skill actions
- **Character creation** — Name entry + skin tone, hair colour, and outfit selection on first launch
- **Furniture placement** — Decorate home interior (Bed, Workbench, Forge, Farm, Chest)
- **Turn-based battle system** — Wild encounters in tall grass, type effectiveness, stat modifiers
- **Pet / Monster System** — Catch and raise companion creatures with evolution, happiness, and party management
- **Multiplayer** — Local link cable (UART between Picos)
- **Day/night cycle** — Time-of-day visuals + encounters
- **Weather system** — Rain/snow affects spawns
- **Achievements** — Track milestones
- **Music + SFX** — I2S audio output

### Home Base / Mansion (Future)

Player's home accessible by pressing A at the door tile:

- Dedicated `MODE_BASE` screen with interior layout
- **Farming** — 3 berry plots with grow-tick timers; plant, wait, harvest
- **Furniture** — Place up to 8 pieces (Bed, Workbench, Forge, Farm Patch, Chest) via cursor
- Sprites in `assets/assets_base.png` (32px cells, SPRITE_SCALE=2)
- Mansion building sprite drawn on the overworld at the north end of the vertical path

### Side-View Platformer (Future)

Toggled via Select (V on desktop):

- `MODE_SIDE`, 30×9 tile scrolling platformer
- Physics: gravity (0.35 px/frame²), jump (−5.5 px/frame), max fall (5.0 px/frame)
- One-way jump-through platforms (4 levels)
- Ceiling collision when rising
- Walk and jump animation
- Ore/Tree nodes accessible via A button press

### Crafting System (Future)

Equipment crafted at the workbench in home base via the CRAFT menu tab:

| Equipment | Bonuses | Recipe |
|-----------|---------|--------|
| Cloth Vest | +1 DEF | 2 Thread, 1 Log |
| Leaf Crown | +1 ATK, +1 SPD | 3 Branch, 1 Herb |
| Iron Collar | +2 ATK, +2 DEF | 3 Ore, 2 Stone |
| Berry Charm | — | 2 Herb, 1 Branch, 1 Thread |
| Silk Cape | +1 DEF, +2 SPD | 4 Thread, 2 Branch |
| Gem Amulet | +2 ATK, +1 DEF, +1 SPD | 1 Gem, 2 Thread, 1 Herb |
| Rune Plate | +3 ATK, +4 DEF | 4 Ore, 4 Stone, 1 Gem |

Requires additional items: Thread, Hide, Herb, Potion (new drops/skills)

### Farming System (Future)

- 3 `FarmPatch` slots in `GameState` (berry_id, grow_ticks, ready flag)
- Plant berry seeds at home base, wait for growth ticks, harvest
- 8 berry types: Oran, Sitrus, Pecha, Rawst, Leppa, Wiki, Mago, Lum
- Berries restore HP or cure status effects

### Battle System (Future)

- Wild encounters in tall grass (1/60 chance per movement frame)
- Turn-based: FIGHT / BAG / RUN; speed stat determines order
- Type chart: Fire → Grass (2×), Grass → Air, Air → Electric, Electric → Fire, Dark neutral
- Damage formula: `(atk × power / def / 50 + 2) × type_mult × (0.85–1.15 variance)`

### Pet / Monster System (Future)

- 5 species (Glub, Bramble, Korvax, Wisp, Shadowkin), each with 4 evolution stages
- Catch via Orb item during battles; party of 4 + box of 12
- Happiness system, equipment bonuses, pet trailing on overworld

### Character Creation & Customisation (Future)

- First-launch screen: name entry, skin tone (4), hair colour (6), outfit colour (6)
- `PlayerCustom` struct: name[12], skin_idx, hair_idx, outfit_idx
- Palette arrays: `C_SKIN_TONES[4]`, `C_HAIR_COLS[6]`, `C_OUTFIT_COLS[6]`

---

## Design Philosophy

**Core Pillars:**

1. **Depth without complexity** — Simple controls, deep systems
2. **Respect player time** — Auto-save, persistent progress, no grinding
3. **Handcrafted charm** — Every tile placed deliberately, no procedural sprawl
4. **Portable nostalgia** — GameBoy-era feel, modern QoL
5. **Embedded-first** — Runs on a $4 chip, scales to desktop

**Inspirations:**
- RuneScape (skilling, resource gathering)
- Stardew Valley (life sim, home building)
- GameBoy Color RPGs (art style, constraints)

---

## Getting Started

### Playing the Simulator

```bash
git clone <repo-url>
cd PicoGame
make build
make dev
```

**First Launch:**
1. You spawn in the top-down overworld
2. Press A facing a Tree, Rock, Ore, or Water tile to start gathering
3. Resources are added to your inventory; skills gain XP
4. Open the menu with M or Tab to view skills and inventory

### Controls Reminder
- **Move:** Arrow keys or W/S/Q/D
- **Interact:** Z, Space, Enter, or A
- **Run:** Hold B (drains energy)
- **Menu:** M or Tab
- **Rotate camera:** `[` / `]`

### Testing

```bash
make test
```

Runs test suites covering: config constants, state init, world generation, skills, game tick, save/load, player movement, render pipeline.

---

## License & Credits

**License:** *(Specify your license here)*  
**Author:** *(Your name/handle)*  
**Version:** 0.1.0 (Prototype)

**Special Thanks:**
- SDL2 team
- Raspberry Pi Foundation
- Pixel art community

---

**Last Updated:** 2026-04-20
