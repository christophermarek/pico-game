# GrumbleQuest

A resource-gathering RPG for Raspberry Pi Pico and desktop.

![Platform](https://img.shields.io/badge/platform-Raspberry%20Pi%20Pico-red)
![Language](https://img.shields.io/badge/language-C11-blue)
![Display](https://img.shields.io/badge/display-240x240%20RGB565-green)

## Quick start — desktop simulator

```bash
make build   # compile sim + tests
make dev     # run sim
make test    # run tests
make editor  # browser map editor at http://localhost:8765/tools/map_editor.html
```

### Controls

| Action          | Keys                         |
|-----------------|------------------------------|
| Move            | Arrow keys or **W/S/Q/D**    |
| Confirm (A)     | **Z**, **Space**, **Enter**, or **A** |
| Run (B)         | hold **X** or **B**          |
| Menu            | **M** or **Tab**             |
| Rotate camera   | **[** / **]**                |

Keyboard `A` is the face button A — use `Q` for left strafe.

## Gameplay

Explore a 30×20 handcrafted overworld rendered isometrically, gather
resources at nodes (trees, rocks, ore, water), and level four skills
(Mining, Fishing, Woodcutting). Nodes respawn after 60 ticks
(~10 min). Gathering awards XP only — the item/inventory system will be
added back once item sprites exist.
Save writes every ~1 min of real time.

## Project layout

```
PicoGame/
├── src/
│   ├── hal.h               # HAL interface — the only thing game code calls
│   ├── hal_sdl.c           # SDL2 desktop implementation
│   ├── hal_pico.c          # Raspberry Pi Pico implementation
│   ├── config.h            # Tuning constants + tile IDs
│   ├── colors.h            # RGB565 palette
│   ├── main.c              # Game loop
│   ├── game/               # state, world, player, skills, tick, save
│   ├── render/             # renderer, iso_spritesheet, font
│   └── ui/                 # hud, menu
├── assets/                 # Source sprites + assembled sheets
├── tools/                  # gen_spritesheets, gen_pico_atlases, map_editor
├── pico/CMakeLists.txt     # Pico SDK build
├── tests/                  # Headless integration tests
└── CMakeLists.txt          # Simulator + tests
```

## Pico hardware

**BOM:** RP2040, ST7789 240×240 SPI display, 8 tactile buttons.

**Wiring** (see `src/pico_config.h`):
- SPI0 SCK GP18, MOSI GP19, CS GP17, DC GP20, RST GP21, BL GP22
- Buttons GP2–GP9 with internal pull-ups

**Build:**
```bash
export PICO_SDK_PATH=/path/to/pico-sdk
cd pico && mkdir build && cd build
cmake ..
make -j$(nproc)
# copy grumblequest.uf2 to the Pico in BOOTSEL mode
```

## Documentation

- [`GAME_DESIGN.md`](GAME_DESIGN.md) — full design doc, skills, future plans
- [`SPRITES.md`](SPRITES.md) — sprite authoring and regeneration workflow
- `src/hal.h` — HAL abstraction surface

## Performance target

- 30 FPS (33 ms frame budget)
- Pico: 133 MHz Cortex-M0+, 264 KB SRAM
- Display: RGB565 native — no colour conversion
- Tile-based culling, static allocations, no heap in game logic
