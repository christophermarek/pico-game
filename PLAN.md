# GrumbleQuest — Implementation Plan

Living doc. Milestone order is a dependency chain — each builds on the last.
Mark milestones `[x]` when shipped.

---

## Milestone 1 — Item data model & backend

**Goal:** Inventory exists in memory and persists. Gathering fills it. No visuals yet.

### Item definitions

```c
typedef enum {
    ITEM_NONE = 0,
    // Resources
    ITEM_OAK_LOG,
    ITEM_STONE,
    ITEM_IRON_ORE,
    ITEM_RAW_FISH,
    ITEM_COIN,          // 10% drop from shearing tall grass
    // Tools (max_stack=1, is_tool=true)
    ITEM_AXE,
    ITEM_PICKAXE,
    ITEM_FISHING_ROD,
    ITEM_SHEARS,
    ITEM_COUNT,
} item_id_t;

typedef struct {
    const char *name;
    uint8_t     icon_idx;   // index into item icon spritesheet
    uint8_t     max_stack;  // 1 = tool (no stack), 99 = resource
    bool        is_tool;
} item_def_t;
```

### Inventory layout

Two zones, stored flat in `GameState`:

| Zone | Slots | Purpose |
|------|-------|---------|
| Hotbar | 8 | Quick-access, always visible on screen |
| Bag | 20 | Overflow, accessible via inventory menu (M6) |

`inventory_add()` fills hotbar first, then bag. Tools always land in their
fixed hotbar slots (slots 5–8 reserved for tools).

Stack cap: **99** per slot for resources. Tools don't stack.

### Node → item mapping

| Node | Required tool | Item dropped | Drop rule |
|------|---------------|--------------|-----------|
| Tree (T_TREE) | Axe | Oak log | 1 per hit |
| Rock (T_ROCK) | Pickaxe | Stone | 1 per hit |
| Ore (T_ORE) | Pickaxe | Iron ore | 1 per hit |
| Water (T_WATER) | Fishing rod | Raw fish | 1 per hit |
| Tall grass (T_TGRASS) | Shears | Coin | 10% chance per hit |

### Node HP (hp-per-node system)

Each node has HP that depletes on each successful gather. When HP reaches 0
the node enters DEPLETED state and starts its respawn timer.

| Node | Max HP | Notes |
|------|--------|-------|
| Tree | 3 | 3 chops to fell |
| Rock | 4 | 4 hits to break |
| Ore | 5 | 5 hits (rarer, more effort) |
| Water | 1 | Single catch, always depletes |
| Tall grass | 2 | 2 snips |

Node HP stored in `world.c` alongside the existing respawn tracker:
```c
typedef struct {
    uint8_t respawn_ticks;   // existing
    uint8_t hp;              // remaining hits before depletion
} node_state_t;

node_state_t nodes[MAP_W * MAP_H];
```
HP resets to max on respawn.

### Start state

Player spawns with all 4 tools pre-loaded in hotbar slots 5–8. No crafting
required. This will be gated behind crafting in a later milestone.

### Files to create/modify

- `game/items.h` — enum + `item_def_t`
- `game/items.c` — metadata table + `inventory_add/remove/count/has_tool`
- `game/state.h` — add `inventory_slot_t inv[28]` (8 hotbar + 20 bag)
- `game/world.h` — add `node_state_t` with `hp` field
- `game/world.c` — init node HP on load/respawn
- `game/skills.c` — tool check, hit node HP, grant item, log message
- `game/save.c` — bump save version; old saves → empty inventory, full node HP

Log messages: `"Got oak log (x1)"` / `"Need an axe."` / `"Found a coin!"` (coin is rare so special copy)

---

## Milestone 2 — Item & tool sprites

**Goal:** Art assets exist in the sheet pipeline; renderer can draw item icons.

### Work
- Add entries to `assets/sprites/manifest.json`:
  - 16×16 item icons for all `ITEM_COUNT` items
- `make sprites` to regenerate sheets
- Renderer helper: `draw_item_icon(item_id_t id, int x, int y)`

### Asset spec — all 16×16 px, transparent background

| Item | Visual |
|------|--------|
| oak_log | Brown cylinder end-on, light wood-grain rings |
| stone | Grey rounded chunk, slight highlight top-left |
| iron_ore | Dark rock with orange-brown speckle veins |
| raw_fish | Silver fish silhouette, facing right, small eye dot |
| coin | Gold circle, faint cross-hatch, small shine dot top-right |
| axe | Side profile: wooden handle, steel head with edge highlight |
| pickaxe | Side profile: handle, curved pick tip pointing right |
| fishing_rod | Angled pole (diagonal), short line + hook at bottom |
| shears | Open scissor profile, two blades visible |

---

## Milestone 3 — HUD redesign (overlay, no strip)

**Goal:** HP, energy, hotbar, and status info are rendered as screen overlays.
The game world renders full 240×240 — no reserved strip, no black bars.

### Layout (240×240, all elements float over the world)

```
┌────────────────────────────────────────────────────────┐
│ ♥ ▓▓▓▓▓▓░░  Day 3          log message here     x,y  │  ← top bar overlay (y=4)
│ ⚡ ▓▓▓░░░░                                             │  ← energy row (y=14)
│                                                        │
│                   GAME WORLD                           │
│                                                        │
│                                                        │
│                                                        │
│  [ ] [ ] [ ] [ ] [ ] [🪓] [⛏] [🎣] [✂]              │  ← hotbar overlay (y=218)
└────────────────────────────────────────────────────────┘
```

No background strip is drawn. Elements sit directly on top of the world
pixels with a single-pixel dark drop shadow for readability.

### Top-left stat block (always visible)

| Element | x | y | Size |
|---------|---|---|------|
| Heart icon | 4 | 4 | 8×8 |
| HP bar | 14 | 5 | 60×6 |
| Bolt icon | 4 | 14 | 8×8 |
| Energy bar | 14 | 15 | 60×6 |

**Bar style:**
- 1px outer border (`C_BORDER`, semi-readable over world)
- Fill: HP = red `#C0392B`, energy = cyan `#1A8FA0`
- 1px highlight at top of fill (lighter tint)
- Drop shadow: 1px offset dark rect drawn first at x+1, y+1

### Top-center: log / day

| Element | Position | Notes |
|---------|----------|-------|
| Log message | centered, y=4 | Shows 3 s after any action, then fades |
| "Day N" | centered, y=4 | Default when no log message |

### Top-right: coordinates (debug / always)

- Right-aligned at x=236, y=4
- `x,y` in non-debug mode; full debug string when debug on

### Hotbar (bottom overlay)

8 resource slots + fixed tool indicator. Two visual rows or single row
depending on available space:

**Single-row layout (preferred):**
- 8 slots × 22×22px + 2px gap = 190px total, centered → x_start = 25
- y = 216 (bottom 24px of screen)
- Slots 1–4: resource items (dynamic)
- Slots 5–8: tool items (fixed, always visible even if empty)
- Separator line (1px, dim) between slot 4 and 5

| Element | Detail |
|---------|--------|
| Slot background | Translucent dark: draw `C_BG_DARK` at 50% opacity (every other pixel trick on Pico) |
| Empty slot | 1px `C_BORDER` border, dark fill |
| Filled slot | Icon centered (12×12 scaled-down), count badge bottom-right |
| Selected slot | Bright white 1px border |
| Tool slot (equipped) | Slightly brighter background tint |
| Add animation | 200ms white pulse on the slot |

**Count badge:** white text, 3×5 font, bottom-right of slot. Hidden if count = 1 for tools.

**Translucency trick (Pico-compatible):** instead of alpha blending, draw
a checkerboard of `C_BG_DARK` pixels over the slot background area before
drawing the border and icon. Cheap, looks good on small screens.

### Menu/tab icons

SK/ST icons stay visible at all times. Move from bottom-right strip to
**top-right corner** — small icons at x=210–238, y=4, right of coordinates.
Same bordered style as current, just repositioned.

### Implementation

- `ui/hud.c` — full rewrite; remove `STRIP_Y`/`STRIP_H` refs
- `config.h` — remove `HUD_STRIP_H` (world now full height); add `HOTBAR_Y=216`, `HOTBAR_SLOT_W=22`, `HOTBAR_SLOTS=8`
- `ui/icons.h` — 8×8 pixel bitmaps for heart, bolt (drawn with `hal_draw_pixel` loop)
- Renderer: remove `DISPLAY_H - HUD_STRIP_H` cull limit; world now culls to full `DISPLAY_H`

---

## Milestone 4 — Node gather animations

**Goal:** Each hit on a node has visual feedback. Depleting a node feels satisfying.

### Per-tile animation state

```c
typedef struct {
    uint8_t state;   // ANIM_IDLE, ANIM_HIT, ANIM_DEPLETING, ANIM_DEPLETED, ANIM_REGROWING
    uint8_t frame;
    uint8_t timer;   // counts down in render frames
} tile_anim_t;

tile_anim_t tile_anims[MAP_W * MAP_H];
```

`world_tick_anims()` called every render frame.

### States

| State | Trigger | Visual | Duration |
|-------|---------|--------|----------|
| IDLE | default | normal sprite | — |
| HIT | each successful gather | sprite flashes white + ±1px X nudge | 6 frames (200ms) |
| DEPLETING | HP > 0 after hit | crack overlay darkens progressively | persistent until HP=0 |
| DEPLETED | HP hits 0 | 4-frame break animation, then final depleted sprite | ~12 frames then hold |
| REGROWING | respawn starts | 4-frame grow-back | ~12 frames |

### Hit feedback (every gather, not just last hit)
- Sprite nudges +2px in facing direction, snaps back over 6 frames
- Brief white tint (fill all non-transparent pixels white for 2 frames)
- Damage crack overlay: semi-transparent crack sprite composited on tile
  - 3 crack stages: light (HP at 2/3), medium (HP at 1/3), heavy (HP at last)

### Break animation frames (4 frames each, full iso tile size)

| Node | F0 | F1 | F2 | F3 (held) |
|------|----|----|----|----|
| Tree | heavy crack, trunk split | tree tilts 20° | tree down, leaves scatter | stump only |
| Rock | rock splits | chunks spread | debris | rubble pile |
| Ore | sparks | vein shatters | ore chunks fly | empty dark vein |
| Tall grass | grass bends | blades scatter | — | bare dirt patch |
| Water | splash | large ripple | ripple fades | calm water (normal tile) |

### Implementation
- `render/renderer.c` — wobble offset + white tint during HIT, frame index during DEPLETED
- `render/crack_overlay.h` — 3 small crack sprites (8×8) tiled over node
- `game/skills.c` — set HIT anim on each gather, DEPLETED when HP=0
- Remove existing static depleted overlay; replaced by DEPLETED anim state

---

## Milestone 5 — Item-drop animation

**Goal:** Item flies from node to hotbar slot when gathered.

### Transient entity system

```c
typedef struct {
    float     wx, wy;       // iso world pixel origin (node center)
    int       sx, sy;       // screen-space target (hotbar slot center)
    item_id_t item;
    uint8_t   timer;        // 0→ITEM_FLY_FRAMES (20 frames, ~667ms)
    bool      active;
} item_fly_t;

#define MAX_ITEM_FLIES 4
item_fly_t item_flies[MAX_ITEM_FLIES];   // in GameState
```

**Arc math:** at `t = timer / ITEM_FLY_FRAMES`:
- screen x = lerp(start_x, slot_x, t)
- screen y = lerp(start_y, slot_y, t) − sin(t × π) × 30   (peak lift 30px)

### Visuals
- 10×10 item icon follows arc (scaled down from 16×16)
- 4-frame poof particle at node on spawn (bright yellow/white, 8×8, expands outward)
- Slot flash (white border pulse 200ms) when item lands

---

## Milestone 6 — Inventory menu (bag view)

**Goal:** Full inventory accessible from menu. See all 28 slots.

### Layout
- New tab in `ui/menu.c`: `MTAB_INVENTORY` (replaces or joins existing SK/ST tabs)
- 4-column grid:
  - Row 0 header: "Hotbar" label
  - Rows 1–2: slots 1–8 (hotbar)
  - Row header: "Bag"
  - Rows 3–7: slots 9–28 (bag)
- D-pad cursor; item name + count shown below grid
- Tool section highlighted with a subtle tint

---

## Parking lot (not in scope yet)

- **Tool tiers** — Bronze → iron → steel axe (deferred)
- **Tool durability** — breaks after N uses (deferred)
- **Drop items** — place item on ground (deferred, M6+)
- **Crafting** — tools and later recipes at a workbench
- **Coin uses** — shop NPC, economy (coin exists now, spending deferred)
- **Weight/encumbrance** — no

---

## Scope summary

| Milestone | Blocking? | Estimated effort | Status |
|-----------|-----------|-----------------|--------|
| M1 — Item data + backend | blocks all | 1–2 sessions | [x] |
| M2 — Item sprites | blocks M3 hotbar, M5 | 0.5 + art time | [x] |
| M3 — HUD overlay redesign | independent of M1 | 1–2 sessions | [x] |
| M4 — Node animations | after M1 | 1–2 + art time | [ ] |
| M5 — Item-drop animation | after M1 + M2 | 1 session | [ ] |
| M6 — Inventory menu | after M1 + M2 | 1 session | [ ] |

M1 + M3 can both start immediately and are independent of each other.
Art (item icons, animation frames) is the biggest time unknown.
