# GrumbleQuest — Game Design Document

**Genre:** Monster-Collecting RPG + Life Simulation  
**Platform:** Raspberry Pi Pico (ST7789 240×240 display) + SDL2 Desktop Simulator  
**Engine:** Custom C11 (zero dependencies except SDL2 for sim)  
**Status:** Playable prototype with core systems implemented

---

## Overview

**GrumbleQuest** is a pixel-art monster-collecting RPG with integrated life-sim mechanics. Players explore a handcrafted top-down overworld, gather resources through 10 distinct skills, raise and evolve companion creatures, battle wild monsters, craft equipment, and build their home base.

The game combines:
- **Monster collection & evolution** (5 species, 4 stages each)
- **Turn-based combat** with type effectiveness and stat modifiers
- **Skilling system** (10 skills: Mining, Fishing, Woodcutting, Combat, Cooking, Magic, Farming, Crafting, Smithing, Herblore)
- **Resource gathering & crafting** (25 item types, 7 equipment pieces)
- **Pet management** (party of 4, storage box of 12, happiness system)
- **Dual perspectives** (top-down overworld + side-view platformer)
- **Persistent save system** with auto-save every 10 seconds

---

## Core Loop

1. **Explore** the 30×20 tile overworld (480×320 px)
2. **Gather resources** by skilling at nodes (trees, rocks, ore, water)
3. **Battle wild monsters** in tall grass encounter zones
4. **Catch and evolve** creatures to build your party
5. **Craft equipment** to boost pet stats and happiness
6. **Return home** to manage inventory, swap party members, and upgrade your base

---

## World Design

### Top-Down Overworld (30×20 tiles, 16px each)

**Key Locations:**
- **Player Mansion** — North end of the map (11×6 tile building), accessible via vertical path. Enter with A button to access base mode.
- **Path Network** — East-west road at y=9, north-south spine at x=15 connecting mansion to crossroads
- **Lake** — Northeast (elliptical water body with sand border, fishing spot)
- **Forest** — Southwest (dense tree cluster with clearings, woodcutting nodes)
- **Rock Quarry** — Southeast (stone and ore mining)
- **Tall Grass Zones** — Three encounter areas for wild battles (northwest, east, south)
- **Resource Nodes** — Trees, rocks, ore veins, water (respawn after 60 ticks ≈ 10 min)

**Terrain Types:**
- Grass (walkable, base terrain)
- Path (walkable, fast travel aesthetic)
- Water (non-walkable, fishing)
- Sand (walkable, decorative)
- Trees (non-walkable, woodcutting resource)
- Rocks (non-walkable, mining resource)
- Ore (non-walkable, rare mining resource)
- Tall Grass (walkable, 1/60 encounter chance per frame)
- Flowers (walkable, decorative)

### Side-View World (30×9 tiles)

Accessible by pressing **SELECT** to toggle view mode. Platformer-style movement with:
- **Ground layer** at y=8
- **Platforms** (4 levels) — jump-through platforms
- **Ore and tree nodes** — resource gathering in side-view
- **Physics** — gravity (0.35 px/frame²), jumping (-5.5 px/frame), max fall speed (5.0 px/frame)

---

## Monster System

### Species (5 total, 4 evolution stages each)

| Species | Type | Base→Final Evolution | Description |
|---------|------|---------------------|-------------|
| **Glub** | Dark | Glub → Glubmaw → Glublord → Abysgore | Shadow blob that grows horns and armor |
| **Bramble** | Grass | Bramble → Thornling → Briarclaw → Verdantwrath | Thorny plant creature |
| **Korvax** | Fire | Korvax → Emberhorn → Infernox → Pyrowrath | Flame-wreathed beast |
| **Wisp** | Air | Wisp → Gustwing → Tempest → Stormlord | Wind elemental |
| **Shadowkin** | Dark | Shadowkin → Darkspawn → Voidwalker → Shadowlord | Pure shadow entity |

**Evolution Triggers:**
- Stage 0 → Stage 1: Level 16–18
- Stage 1 → Stage 2: Level 32–36
- Stage 2 → Stage 3: Level 48–54

**Stats:**
- HP (30–90 base + 4–5 per level)
- Attack (11–14 base + 2–3 per level)
- Defense (8–13 base + 2–3 per level)
- Speed (10–16 base + 2–3 per level)

**Move Pool:** 32 moves, 4 slots per pet
- Physical moves (Tackle, Shadow Bite, Night Slash, Slam)
- Special moves (Dark Pulse, Flamethrower, Solar Beam, Hurricane)
- Status moves (Leer, Growl, Swords Dance, Iron Defense, Recover)

**Happiness System:**
- Range: 0–100
- Decays by 1 every game tick (10 seconds)
- Restored by berries, equipment, and combat bonuses
- High happiness grants combat bonuses: +ATK modifier, evade chance

---

## Combat System

### Turn-Based Battle

**Flow:**
1. Wild encounter triggers in tall grass (1/60 chance per movement frame)
2. Enemy level scales with player combat skill
3. Speed stat determines turn order
4. Player chooses: FIGHT / BAG / SWAP / RUN
5. Damage calculated with attack, defense, power, type multiplier, stat mods
6. Victory awards XP to active pet and combat skill

**Type Effectiveness Chart:**
```
Fire   → Grass (2×)     Grass  → Air      (2×)
Air    → Electric (2×)  Electric → Fire   (2×)
Fire   ← Electric (0.5×)  Grass  ← Fire     (0.5×)
Air    ← Grass (0.5×)     Electric ← Air    (0.5×)
Dark   → Neutral (1×)
```

**Stat Modifiers:**
- Attacks and status moves can apply ±stages (−3 to +3)
- Each stage = ±50% effectiveness
- Capped at 0.25× minimum, 4× maximum

**Damage Formula:**
```c
base_dmg = (atk × power / def / 50 + 2) × type_mult × (0.85–1.15 variance)
final_dmg = apply_stat_mod(base_dmg, atk_mod)
```

**Catch Mechanics:**
- Base rate: 30% + 70% × (1 − enemy_hp / enemy_max_hp)
- Penalty: −15% per evolution stage
- Minimum: 5%
- Uses **Orb** item from inventory

---

## Skills System (10 Total)

| Skill | Nodes | Items Dropped | XP Range |
|-------|-------|---------------|----------|
| **Mining** | Rock, Ore | Stone, Ore, Gem | 30–50 |
| **Fishing** | Water | Fish, Seaweed | 20–30 |
| **Woodcutting** | Tree | Log, Branch | 25–40 |
| **Combat** | Battle | — | 5× enemy level |
| **Cooking** | — | Meals from ingredients | — |
| **Magic** | — | Orbs, spell items | — |
| **Farming** | Farm patches | Berries (8 types) | — |
| **Crafting** | Workbench | Thread, equipment | — |
| **Smithing** | Forge | Iron Collar, Rune Plate | — |
| **Herblore** | — | Potions from herbs | — |

**Skilling Flow:**
1. Face a resource node (tree, rock, ore, water)
2. Press **A** to start action
3. Action completes in 30 frames (1 second at 30 FPS)
4. Skill XP awarded, item added to inventory, node depleted
5. Node respawns after 60 ticks (≈10 min real time)
6. Active pet gains half the skill XP

**XP → Level Formula:**
```c
xp_for_level(n) = n² × 10 + n × 20
```

---

## Inventory & Items (25 Types)

### Resources
- **Ore, Stone, Gem** — mining drops
- **Fish, Seaweed** — fishing drops
- **Log, Branch** — woodcutting drops
- **Herb, Thread, Hide** — crafting materials

### Consumables
- **Meals** — restore HP/hunger
- **Bread, Egg** — starting food
- **Orb** — capture wild monsters

### Berries (8 types)
| Berry | Effect |
|-------|--------|
| Oran | +10 HP, +2 happiness |
| Sitrus | +25 HP, +4 happiness |
| Pecha | Cures burn, +5 happiness |
| Rawst | Cures paralysis, +5 HP, +3 happiness |
| Leppa | +10 PP per move, +2 happiness |
| Wiki | +15 HP, +3 happiness, +1 ATK |
| Mago | +15 HP, +3 happiness, +1 SPD |
| Lum | Cures all status, +10 HP, +6 happiness, +5 PP |

**Farming System:**
- 3 farm patches at home base
- Plant berries, wait for growth ticks
- Harvest when ready

---

## Equipment & Crafting (7 Pieces)

| Equipment | Bonuses | Recipe |
|-----------|---------|--------|
| **Cloth Vest** | +1 DEF, +2 happiness | 2 Thread, 1 Log |
| **Leaf Crown** | +1 ATK, +1 SPD, +4 happiness, +1 happy/tick | 3 Branch, 1 Herb |
| **Iron Collar** | +2 ATK, +2 DEF | 3 Ore, 2 Stone |
| **Berry Charm** | +5 happiness, +2 happy/tick | 2 Herb, 1 Branch, 1 Thread |
| **Silk Cape** | +1 DEF, +2 SPD, +3 happiness, +1 happy/tick | 4 Thread, 2 Branch |
| **Gem Amulet** | +2 ATK, +1 DEF, +1 SPD, +5 happiness, +1 happy/tick | 1 Gem, 2 Thread, 1 Herb |
| **Rune Plate** | +3 ATK, +4 DEF, +2 happiness | 4 Ore, 4 Stone, 1 Gem |

**Crafting Flow:**
1. Open menu → CRAFTING tab
2. Select equipment blueprint
3. Check recipe requirements
4. Craft (consumes ingredients)
5. Equip to active pet

---

## Home Base (MODE_BASE)

**Accessible:** Press A at mansion door (15, 4)

**Features:**
- **Party Management** — swap active pet, view stats
- **Storage Box** — 12 pet capacity
- **Furniture System** (8 slots, future expansion)
  - Bed, Workbench, Forge, Farm patches, Chest, Pet Bed, Pet House
- **Farming** — 3 berry plots with growth timers
- **Exit:** Press B to return to overworld

---

## UI & Modes

### Game Modes

| Mode | Description | Controls |
|------|-------------|----------|
| **CHAR_CREATE** | Character customization (name, skin, hair, outfit) | Arrows/WASD, A to confirm, B to back |
| **TOPDOWN** | Overworld exploration | Arrows/WSQD to move, A to interact, B to cancel, M for menu, V to toggle view |
| **SIDE** | Platformer view | Arrows/WSQD, A/Space to jump |
| **BATTLE** | Turn-based combat | Arrows to select, A to confirm, B to back |
| **MENU** | Inventory/stats/crafting | Arrows/Left-Right tabs, A to select |
| **BASE** | Home management | B to exit |

### HUD Elements

**Bottom Strip (28px):**
- Player HP bar (green)
- Hunger bar (orange, decays 2 points per tick)
- Energy bar (blue, used for skills, regenerates when idle)
- Recent log message

**Top-Left Corner:**
- Active pet name + level
- Pet HP bar (green)

**Top-Right Corner:**
- Current day counter

### Menu Tabs (Press M/Start)

1. **SKILLS** — View all 10 skill levels and XP progress
2. **ITEMS** — 20-slot inventory grid with counts
3. **PARTY** — 4 party members + 12 storage box slots
4. **CRAFTING** — Equipment recipes and crafting interface
5. **(Future) EQUIP** — Manage pet equipment

---

## Controls

### Desktop Simulator (SDL2)

| Action | Keys |
|--------|------|
| Move | Arrow Keys or W/S/Q/D |
| Confirm (A button) | Z, Space, Enter, or A |
| Cancel (B button) | X or B |
| Menu | M (Start) |
| Toggle View | V (Select) |

**Note:** Keyboard A is mapped to the face button A (confirm), not left movement. Use Q for strafe-left.

### Pico Hardware (Planned)

- **Directional:** GPIO 2–5 (UP/DOWN/LEFT/RIGHT)
- **Face buttons:** GPIO 6–7 (A/B)
- **System:** GPIO 8–9 (START/SELECT)
- **Display:** SPI0 (ST7789 240×240 RGB565)

---

## Save System

**Format:** Binary file (`grumble_save.bin` on desktop, flash sector on Pico)  
**Structure:**
- Magic number: `0x4735513E` ("G5Q1")
- Version: 1
- Full `GameState` struct (all progress, inventory, pets, skills, world state)

**Auto-Save:** Every game tick (10 seconds)  
**Load:** On startup; if invalid/missing, enters character creation

**Saved Data:**
- Player stats (HP, hunger, energy, position, customization)
- All 10 skill levels and XP
- Full inventory (20 slots)
- Party (4 pets) + box (12 pets) with stats, moves, happiness, equipment
- World node respawn timers
- Total steps, battles, catches
- Current day

---

## Technical Architecture

### Hardware Abstraction Layer (HAL)

**Interface:** `hal.h`  
**Implementations:**
- `hal_sdl.c` — Desktop simulator (SDL2, 3× scale, 720×720 window + 24px debug strip)
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
│   ├── game/                 # Core game logic
│   │   ├── state.{c,h}       # GameState, inventory, XP
│   │   ├── world.{c,h}       # Map generation, collision
│   │   ├── player.{c,h}      # Movement, actions
│   │   ├── battle.{c,h}      # Combat system
│   │   ├── tick.{c,h}        # Time-based decay/regen
│   │   ├── skills.{c,h}      # Skilling system
│   │   ├── pets.{c,h}        # Monster management
│   │   └── save.{c,h}        # Persistence
│   ├── render/               # Graphics
│   │   ├── renderer.{c,h}    # Camera, frame dispatch
│   │   ├── sprites.{c,h}     # Tile/character drawing
│   │   └── font.{c,h}        # 5×7 pixel font
│   ├── ui/                   # Interface screens
│   │   ├── hud.{c,h}         # Status bars
│   │   ├── menu.{c,h}        # Inventory/skills menu
│   │   ├── battle_ui.{c,h}   # Combat interface
│   │   └── char_create.{c,h} # Character customization
│   └── data/                 # Game data tables
│       ├── species.h         # Monster definitions
│       ├── moves.h           # Move pool + type chart
│       ├── berries.h         # Berry effects
│       └── equipment.h       # Craftable gear
├── tests/
│   ├── hal_stub.{c,h}        # Headless HAL for CI
│   └── test_runner.c         # Integration tests
├── CMakeLists.txt            # Build configuration
├── Makefile                  # Convenience targets
└── GAME_DESIGN.md            # This document
```

### Build System

**CMake + Make wrapper:**
```bash
make build   # Compile sim + tests
make test    # Run test suite
make dev     # Run simulator (blocking)
make clean   # Remove build/
```

**Targets:**
- `grumblequest_sim` — SDL2 desktop build
- `grumblequest_tests` — Headless unit/integration tests
- *(Future)* `grumblequest.uf2` — Pico firmware

**Dependencies:**
- SDL2 (sim only)
- C11 compiler (Clang/GCC)
- CMake 3.13+

---

## Art Style

**Palette:** RGB565 (5:6:5 bit depth, ST7789 native)  
**Resolution:** 240×240 effective (scaled 3× in sim)  
**Tile Size:** 16×16 pixels  
**Style:** Low-res pixel art, hand-drawn aesthetic

**Key Colors:**
- Grass: Dark/light green
- Water: Deep blue → light blue animated waves
- Paths: Brown with light pebbles
- Trees: Dark/mid/light green canopy + brown trunk
- Rocks: Gray with highlights
- Mansion: Stone gray, dark roof panels, gold accents
- UI: Dark purple panels, gold borders, cyan text

**Animations:**
- Water waves (4-frame cycle)
- Walk frames (2-frame bob)
- Monster sprites (bobbing, evolving)
- XP floaters (rising text)
- Skill progress bar

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
- Fixed-point math (avoid float on embedded)
- Static allocations (no heap)
- Tile-based culling (only draw visible region)
- 30 FPS target (33ms frame budget)

---

## Future Expansion

### Planned Features
- **Cooking system** — combine ingredients into meals
- **Magic system** — spell casting with orbs
- **Herblore** — potion crafting
- **Furniture placement** — decorate home interior
- **Multiplayer** — local link cable (UART between Picos)
- **Day/night cycle** — time-of-day visuals + encounters
- **Weather system** — rain/snow affects spawns
- **Boss battles** — rare encounters, unique loot
- **Achievements** — track milestones
- **Music + SFX** — I2S audio output

### Content Additions
- More species (target: 12–15)
- More moves (target: 64)
- More equipment (target: 20+)
- Larger world (procedural generation?)
- Side quests / NPC dialogue

---

## Design Philosophy

**Core Pillars:**

1. **Depth without complexity** — Simple controls, deep systems
2. **Respect player time** — Auto-save, persistent progress, no grinding
3. **Handcrafted charm** — Every tile placed deliberately, no procedural sprawl
4. **Portable nostalgia** — GameBoy-era feel, modern QoL
5. **Embedded-first** — Runs on a $4 chip, scales to desktop

**Inspirations:**
- Pokémon (monster collection, turn-based combat)
- RuneScape (skilling, resource gathering, crafting)
- Stardew Valley (life sim, home building)
- GameBoy Color RPGs (art style, constraints)

---

## Getting Started

### Playing the Simulator

```bash
# Clone repository
git clone <repo-url>
cd PicoGame

# Build
make build

# Run
make dev
```

**First Launch:**
1. Character creation screen appears
2. Name your character (arrows to cycle letters, A to confirm)
3. Choose skin tone, hair color, outfit
4. Press A on "BEGIN ADVENTURE"
5. You spawn south of your mansion
6. Walk north on the path to find your home
7. Press A to interact with resource nodes
8. Walk through tall grass to trigger battles

### Controls Reminder
- **Move:** Arrow keys or W/S/Q/D
- **Interact:** Z, Space, Enter, or A
- **Back/Cancel:** X or B
- **Menu:** M
- **Toggle View:** V

### Testing

```bash
make test
```

Runs 11 test suites covering:
- Config constants
- Color palette
- Data tables (species, moves, equipment, berries)
- State initialization & inventory
- World generation & collision
- Pets & skills
- Game tick mechanics
- Battle damage formulas
- Save/load roundtrip
- Player movement
- Full render pipeline

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

## Contact & Contributing

*(Add your contact info, GitHub repo link, Discord server, etc.)*

---

**Last Updated:** 2026-04-19
