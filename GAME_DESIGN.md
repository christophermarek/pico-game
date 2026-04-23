# GrumbleQuest — Game Design Document

**Genre:** Resource-gathering RPG
**Platform:** Raspberry Pi Pico (ST7789 240×240 display) + SDL2 desktop simulator
**Engine:** Custom C11 (zero dependencies except SDL2 for the sim)

## Overview

A pixel-art top-down RPG focused on exploration and skill progression. The
player moves around a handcrafted 30×20 isometric overworld and gathers at
resource nodes to level three skills. Scope is deliberately small — this is a
prototype, and the doc focuses on what exists, not what's dreamed.

## Core loop

1. Explore the 30×20 tile overworld.
2. Face a resource node (tree, rock, ore, water) and press **A** to start a
   skilling action.
3. After ~1 second the action completes, awarding XP in the relevant skill.
4. The node depletes for ~10 min, then respawns.

## World

30×20 tiles, 16 px each. Rendered isometrically (2:1 diamond) with four
90°-step camera bearings. The map is loaded from `assets/maps/map.bin`.

**Tile types:**

| ID | Tile | Walkable | Role |
|----|------|----------|------|
| 0 | Grass | ✓ | Base terrain |
| 1 | Path | ✓ | Aesthetic road |
| 2 | Water | — | Fishing node |
| 3 | Sand | ✓ | Decorative |
| 4 | Tree | — | Woodcutting node (circle collider) |
| 5 | Rock | — | Mining node |
| 6 | Flower | ✓ | Decorative |
| 7 | Tall grass | ✓ | Decorative |
| 8 | Ore | — | Mining node (rare) |

## Skills

| Skill | Nodes | XP range |
|-------|-------|----------|
| Mining | Rock, Ore | 30–50 |
| Fishing | Water | 20–30 |
| Woodcutting | Tree | 25–40 |

Action duration: 30 frames (1 s at 30 FPS). Node respawn: 60 game ticks
(~10 min). Level cap: 99.

```c
xp_for_level(n) = n² × 10 + n × 20
```

## UI & controls

### Modes

| Mode | Description |
|------|-------------|
| TOPDOWN | Overworld exploration |
| MENU | Skills / Settings tabs |

### HUD (bottom 18 px)

- HP bar (green)
- Energy bar (blue; drains while running, regenerates when idle)
- Recent log message or "Day X" when idle
- Tile coordinates + tab shortcut icons on the right

### Controls (SDL)

| Action | Keys |
|--------|------|
| Move | Arrow keys / W/S/Q/D |
| Confirm (A) | Z, Space, Enter |
| Run (B) | hold X |
| Menu | M / Tab |
| Rotate camera | `[` / `]` |

Keyboard A is the face-button A (not left movement) — use Q for strafe-left.

### Pico controls

Active-low buttons with internal pull-ups:
GP2–GP5 direction, GP6 A, GP7 B, GP8 Start.

## Save system

Binary format with magic `0x4735513E`, version bump on any `SaveData`
layout change. Writes to `grumble_save.bin` on desktop, to the last flash
sector on Pico (length-prefixed). Auto-save every 6 game ticks (~1 min).

Persisted: player position, stats, skill XP/levels, world tick count, day,
total steps, debug-mode preference. Transient fields (log, menu cursor,
skilling progress, frame counter, debug telemetry) are not saved.

## Architecture

### HAL

All platform-specific code lives behind [`src/hal.h`](src/hal.h). Game code
calls only HAL functions; adding a new platform means writing one
`hal_*.c`.

| Impl | Target |
|------|--------|
| `hal_sdl.c` | Desktop sim (SDL2, 3× scaled window) |
| `hal_pico.c` | RP2040 + ST7789V SPI display |
| `hal_stub.c` | Headless tests (RAM framebuffer, no SDL) |

### Project layout

```
PicoGame/
├── src/
│   ├── hal.h / hal_sdl.c / hal_pico.c   # HAL interface + platforms
│   ├── config.h                         # Constants + tile IDs
│   ├── colors.h                         # RGB565 palette
│   ├── main.c                           # Game loop
│   ├── game/                            # state, world, player, skills, tick, save
│   ├── render/                          # renderer, iso_spritesheet, font
│   └── ui/                              # hud, menu
├── assets/sprites/                      # Source PNGs (see SPRITES.md)
├── assets/maps/map.bin                  # Overworld map
├── tests/                               # Headless HAL + integration tests
├── tools/                               # gen_spritesheets, gen_pico_atlases, map_editor
├── pico/CMakeLists.txt                  # Pico hardware build
└── CMakeLists.txt                       # Simulator + test build
```

### Build

```bash
make build     # sim + tests
make test      # run ctest
make dev       # build + run sim
make editor    # browser map editor at http://localhost:8765/...
make clean
```

## Performance

- RGB565 framebuffer (no colour conversion)
- Static allocations only — no heap in game code
- Tile culling: only draw cells within the screen cull zone
- Target 30 FPS (33 ms frame budget)
- Pico: RP2040 dual-core @ 133 MHz, 264 KB SRAM, 2 MB flash

## Future (rough direction)

Everything below is a sketch — no code exists yet. Listed so contributors
can see where the scope might grow, not as committed spec.

- **Items & inventory** — Gathering should drop items; inventory UI, stacking
- **Home base interior** — Separate scene with farm plots, workbench, chest
- **Side-view platformer mode** — Toggle via Select, physics-based scrolling
- **Crafting** — Recipes at workbench, equipment bonuses
- **Farming** — Timed berry growth
- **Combat** — Turn-based wild encounters in tall grass
- **Pets / monsters** — Catch, raise, party of companions
- **Cooking, magic, smithing, herblore** — More skills
- **Character creation** — Skin/hair/outfit on first launch
- **Music + SFX** — I2S audio on Pico, SDL_mixer on desktop
- **Day/night, weather, achievements**

## Design philosophy

1. **Depth without complexity** — Simple controls, deep systems.
2. **Respect player time** — Auto-save, persistent progress, no grinding.
3. **Handcrafted charm** — Every tile placed deliberately, no procedural sprawl.
4. **Embedded-first** — Runs on a $4 chip, scales up to desktop.

**Inspirations:** RuneScape (skilling), Stardew Valley (life sim),
GameBoy-era RPGs (constraints).
