# GrumbleQuest

A monster-collecting RPG with life-sim mechanics for Raspberry Pi Pico and desktop.

![Platform](https://img.shields.io/badge/platform-Raspberry%20Pi%20Pico-red)
![Language](https://img.shields.io/badge/language-C11-blue)
![Display](https://img.shields.io/badge/display-240x240%20RGB565-green)

---

## Quick Start

### Desktop Simulator (SDL2)

```bash
# Build
make build

# Run
make dev

# Run tests
make test
```

### Controls

| Action | Keys |
|--------|------|
| Move | Arrow Keys or **W/S/Q/D** |
| Confirm (A) | **Z**, **Space**, **Enter**, or **A** |
| Cancel (B) | **X** or **B** |
| Menu | **M** |
| Toggle View | **V** |

**Note:** Keyboard `A` = face button A (confirm), use `Q` for left movement.

---

## What is GrumbleQuest?

A pixel-art RPG combining:

- 🐉 **Monster Collection** — Catch, train, and evolve 5 species (4 stages each)
- ⚔️ **Turn-Based Combat** — Type effectiveness, stat modifiers, 32 moves
- 🎯 **10 Skills** — Mining, Fishing, Woodcutting, Combat, Cooking, Magic, Farming, Crafting, Smithing, Herblore
- 🏠 **Home Building** — Manage your mansion, craft equipment, farm berries
- 🗺️ **Dual Perspectives** — Top-down overworld + side-view platformer
- 💾 **Auto-Save** — Progress saved every 10 seconds

---

## Features

### Core Systems
- ✅ Character customization (name, skin, hair, outfit)
- ✅ Resource gathering (trees, rocks, ore, water)
- ✅ Turn-based wild battles with catch mechanics
- ✅ Pet evolution (5 species × 4 stages = 20 forms)
- ✅ Equipment crafting (7 pieces with stat bonuses)
- ✅ Happiness system (affects combat performance)
- ✅ Inventory management (25 item types, 20 slots)
- ✅ Party system (4 active + 12 storage box)
- ✅ Persistent save/load with auto-save
- ✅ HUD with HP/hunger/energy bars
- ✅ Day counter and log system

### World
- 🌍 30×20 tile overworld (480×320 px)
- 🏰 11×6 tile player mansion
- 🌲 Handcrafted biomes (forest, lake, quarry)
- ⚡ Encounter zones (tall grass battles)
- 🔄 Respawning resource nodes

### Technical
- 🎮 Zero-dependency C11 core (HAL abstraction)
- 🖥️ SDL2 desktop simulator (3× scale)
- 📱 Raspberry Pi Pico target (ST7789 240×240 SPI)
- 🧪 Headless test suite (11 suites, all passing)
- 🎨 RGB565 direct rendering (no conversion overhead)
- ⚡ 30 FPS target

---

## Project Structure

```
PicoGame/
├── src/
│   ├── hal.h               # Hardware abstraction interface
│   ├── hal_sdl.c           # SDL2 desktop implementation
│   ├── hal_pico.c          # Pico hardware (WIP)
│   ├── main.c              # Game loop
│   ├── game/               # Core logic (state, world, battle, skills, pets)
│   ├── render/             # Graphics (sprites, font, camera)
│   ├── ui/                 # Screens (menu, HUD, char create, battle UI)
│   └── data/               # Tables (species, moves, berries, equipment)
├── tests/                  # Integration tests + stub HAL
├── CMakeLists.txt
├── Makefile
├── README.md               # This file
└── GAME_DESIGN.md          # Full design document
```

---

## Game Loop

1. **Explore** the overworld
2. **Gather** resources by skilling (Mining, Fishing, Woodcutting)
3. **Battle** wild monsters in tall grass
4. **Catch** creatures with Orbs
5. **Craft** equipment from materials
6. **Evolve** your party
7. **Return home** to manage inventory and pets

---

## Monster Types & Effectiveness

```
Fire → Grass (2×)       Grass → Air (2×)
Air → Electric (2×)     Electric → Fire (2×)
Dark → Neutral (1×)
```

**Species:**
- Glub (Dark) — Shadow blob
- Bramble (Grass) — Thorny plant
- Korvax (Fire) — Flame beast  
- Wisp (Air) — Wind elemental
- Shadowkin (Dark) — Pure shadow

---

## Skills

| Skill | Resource | Items |
|-------|----------|-------|
| Mining | Rock, Ore | Stone, Ore, Gem |
| Fishing | Water | Fish, Seaweed |
| Woodcutting | Tree | Log, Branch |
| Combat | Battles | — |
| Cooking | — | Meals |
| Magic | — | Orbs |
| Farming | Home | Berries |
| Crafting | Workbench | Equipment |
| Smithing | Forge | Iron gear |
| Herblore | — | Potions |

---

## Equipment

| Name | Bonuses | Recipe |
|------|---------|--------|
| Cloth Vest | +1 DEF | 2 Thread, 1 Log |
| Leaf Crown | +1 ATK, +1 SPD, +1 happy/tick | 3 Branch, 1 Herb |
| Iron Collar | +2 ATK, +2 DEF | 3 Ore, 2 Stone |
| Silk Cape | +1 DEF, +2 SPD | 4 Thread, 2 Branch |
| Gem Amulet | +2 ATK, +1 DEF, +1 SPD | 1 Gem, 2 Thread, 1 Herb |
| Rune Plate | +3 ATK, +4 DEF | 4 Ore, 4 Stone, 1 Gem |

---

## Building

### Requirements
- CMake 3.13+
- C11 compiler (GCC/Clang)
- SDL2 (desktop only)

### Makefile Targets

```bash
make build   # Compile simulator + tests
make test    # Run test suite
make dev     # Run simulator (blocking)
make clean   # Remove build directory
```

### Manual Build

```bash
mkdir -p build && cd build
cmake ..
cmake --build . -j8
./grumblequest_sim
```

---

## Testing

```bash
make test
```

**Test Coverage:**
- Config/colors
- Data tables (species, moves, equipment, berries)
- State initialization & inventory
- World generation & collision
- Skills & pets
- Battle damage calculations
- Save/load roundtrip
- Full render pipeline

---

## Hardware Target (Pico)

**BOM:**
- Raspberry Pi Pico (RP2040)
- ST7789 240×240 SPI display
- 8 tactile buttons (or D-pad + 4 face buttons)
- Wiring harness

**Pinout:**
- SPI0 (SCK: GP18, TX: GP19, CS: GP17, DC: GP20, RST: GP21, BL: GP22)
- Buttons: GP2–9 (internal pull-ups)

**Build:** *(Pico SDK integration coming soon)*

---

## Roadmap

### v0.2 (Next)
- [ ] Cooking system
- [ ] Magic spells
- [ ] Herblore potions
- [ ] Furniture placement
- [ ] Music + SFX

### v0.3
- [ ] Day/night cycle
- [ ] Weather system
- [ ] Boss battles
- [ ] More species (12–15 total)
- [ ] More moves (64 total)

### v1.0
- [ ] Pico firmware build
- [ ] Hardware prototype
- [ ] Multiplayer (local UART link)
- [ ] Achievements
- [ ] Full content pass

---

## Documentation

- **Full Design Doc:** [GAME_DESIGN.md](GAME_DESIGN.md)
- **Architecture:** See `src/hal.h` for HAL abstraction
- **Game Data:** See `src/data/*.h` for species/moves/items
- **Build System:** `CMakeLists.txt` + `Makefile`

---

## Performance

- **Target:** 30 FPS (33ms frame budget)
- **Pico:** 133 MHz ARM Cortex-M0+, 264 KB SRAM
- **Display:** RGB565 native (no conversion)
- **Render:** Tile-based culling, direct SPI writes

---

## License

*(Specify your license here)*

---

## Credits

**Author:** *(Your name)*  
**Inspired by:** Pokémon, RuneScape, Stardew Valley, GameBoy Color RPGs

**Built with:**
- SDL2 (desktop sim)
- Raspberry Pi Pico SDK (hardware target)
- CMake
- Pure C11 (zero game-logic dependencies)

---

## Contributing

*(Add contribution guidelines here)*

---

**Status:** Playable prototype with core systems implemented  
**Last Updated:** 2026-04-19
