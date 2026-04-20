# GrumbleQuest

A resource-gathering RPG for Raspberry Pi Pico and desktop.

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
| Run (B) | Hold **X** or **B** |
| Menu | **M** or **Tab** |
| Rotate Camera | **[** / **]** |

**Note:** Keyboard `A` = face button A (confirm), use `Q` for left movement.

---

## What is GrumbleQuest?

A minimalist pixel-art RPG focusing on exploration and resource gathering:

- 🎯 **4 Skills** — Mining, Fishing, Woodcutting, Cooking
- 🗺️ **Top-Down Overworld** — Handcrafted 30×20 tile map with isometric rendering
- 🔄 **Resource Nodes** — Trees, rocks, ore, water (respawn after ~10 minutes)
- 📦 **Inventory** — 20 slots, 12 item types
- 💾 **Auto-Save** — Progress saved every ~1 minute
- 📊 **XP System** — Level up skills through gathering actions

---

## Features

### Currently Implemented
- ✅ Resource gathering (Mining, Fishing, Woodcutting)
- ✅ Skill leveling with XP progression
- ✅ Inventory management (20 slots)
- ✅ Handcrafted overworld map with collision
- ✅ Isometric camera with 4 rotation bearings
- ✅ Node depletion and respawn system
- ✅ Running (hold B to run, drains energy)
- ✅ HUD with HP/energy bars and message log
- ✅ Menu system (Skills tab, Items tab)
- ✅ Persistent save/load
- ✅ Day counter

### Future Features
- 🏠 Home base interior with furniture placement
- 🌾 Farming system with berry growth timers
- ⚔️ Equipment crafting (7 pieces)
- 🍖 Hunger bar and cooking system
- ✨ Magic and Herblore skills
- 🔨 Smithing at forge
- 🎮 Side-view platformer mode
- 🐉 Pet/monster collection system
- ⚔️ Turn-based battle system
- 🎨 Character creation and customization
- 🎵 Music + SFX

---

## Project Structure

```
PicoGame/
├── src/
│   ├── hal.h               # Hardware abstraction interface
│   ├── hal_sdl.c           # SDL2 desktop implementation
│   ├── hal_pico.c          # Raspberry Pi Pico implementation
│   ├── main.c              # Game loop
│   ├── game/               # Core logic (state, world, player, skills, save)
│   ├── render/             # Graphics (sprites, font, camera, iso spritesheet)
│   └── ui/                 # Screens (menu, HUD)
├── assets/                 # Source PNG spritesheets (edit directly!)
├── tools/
│   ├── sprite_template.py  # Generate blank sprite templates
│   └── gen_pico_atlases.py # Convert PNGs to C arrays for Pico
├── pico/
│   └── CMakeLists.txt      # Pico SDK build
├── tests/                  # Integration tests + stub HAL
├── CMakeLists.txt          # SDL simulator + test build
└── README.md
```

---

## Game Loop

1. **Explore** the top-down overworld
2. **Face resource nodes** (trees, rocks, ore, water)
3. **Press A** to start skilling action
4. **Gain XP** and collect items when action completes
5. **Level up skills** through repeated gathering
6. **Manage inventory** via the menu (M or Tab)

---

## Skills

| Skill | Resource | Items | XP Range |
|-------|----------|-------|----------|
| Mining | Rock, Ore | Stone, Ore, Gem | 30–50 |
| Fishing | Water | Fish, Seaweed | 20–30 |
| Woodcutting | Tree | Log, Branch | 25–40 |
| Cooking | — | (future: meals) | — |

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

---

## Testing

```bash
make test
```

**Test Coverage:**
- Config constants
- State initialization & inventory
- World generation & collision
- Skills system
- Save/load roundtrip
- Player movement
- Full render pipeline

---

## Hardware Target (Pico)

**BOM:**
- Raspberry Pi Pico (RP2040)
- ST7789 240×240 SPI display
- 8 tactile buttons

**Pinout:**
- SPI0 (SCK: GP18, TX: GP19, CS: GP17, DC: GP20, RST: GP21, BL: GP22)
- Buttons: GP2–9 (internal pull-ups)

**Build:**

```bash
cd pico
mkdir build && cd build
cmake -DPICO_SDK_PATH=/path/to/pico-sdk ..
make -j4
# Flash grumblequest.uf2 to the Pico
```

---

## Documentation

- **Full Design Doc:** [GAME_DESIGN.md](GAME_DESIGN.md)
- **Sprite Sheets:** [assets_iso_sheets.md](assets_iso_sheets.md)
- **Architecture:** See `src/hal.h` for HAL abstraction

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
**Inspired by:** RuneScape, Stardew Valley, GameBoy Color RPGs

**Built with:**
- SDL2 (desktop sim)
- Raspberry Pi Pico SDK (hardware target)
- CMake
- Pure C11 (zero game-logic dependencies)

---

**Status:** Playable prototype with core gathering systems implemented  
**Last Updated:** 2026-04-20
