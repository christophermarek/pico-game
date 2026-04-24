# GrumbleQuest — Next Steps

Completed in last session: M1–M6 (items, inventory, HUD overlay, node animations, item fly, bag menu).
Branch merged: `claude/recursing-taussig-0c14c3` → main (PR #5).

---

## High priority

### Replace placeholder item sprites
All 9 item icons are placeholders in `assets/sprites/items/`.
Real 16×16 pixel-art sprites needed for: oak_log, stone, iron_ore, raw_fish, coin, axe, pickaxe, fishing_rod, shears.
Drop replacements over the existing files and re-run `make sprites`.

### Add a BG tab icon to the HUD
Items that land in bag slots (8–27) have no visual landing feedback — the fly arc disappears off-screen.
Add a "BG" tab icon in `src/ui/hud.c` (alongside SK/ST) and route bag-slot item flies to it.
Also add `bag_flash` counter to `GameState` for landing pulse.

### Day/night cycle + energy regen pacing — tuned
`TICK_MS = 2000`, `TICKS_PER_DAY = 60`, `NODE_RESPAWN_TICKS = 30`,
`SAVE_EVERY_TICKS = 30`. A full day is 120 s (2 min); energy refills in ~200 s
of idle time; depleted nodes respawn in 60 s. Revisit during real playtesting.

---

## Medium priority

### Swing-and-collide action model (partially in place)
Current targeting uses a **3-stage pipeline** (see player.c
find_action_target): collision probe → raycast → own-tile fallback. The
raycast is the single-ray degenerate of a swing arc and already handles
reach (`TARGET_REACH_PX`) and camera-direction resolution via the
inverse iso transform.

For full combat/AOE swing the raycast step becomes a swept hit region:
- Replace the single ray with an arc of rays (or a swept bounding box)
  sampled across ~8 angles during the swing animation.
- Each frame of the swing emits `on_hit(target)` for any entity/tile
  entering the hit region that hasn't been hit this swing.
- Per-swing `hit_this_swing[]` bitset (one bit per entity) prevents
  double-hits while the arc continues through a target.
- `NodeAction` gains an optional `on_hit` callback; the current
  `skill_complete_action` path becomes `on_complete`.

Entities (enemies, interactable NPCs, dropped items) plug into the
same scan by implementing the "does world-point `(x, y)` overlap me?"
primitive. Until then, tiles are the only hit targets.

### Crafting system
Tools are pre-loaded at game start. Gate them behind a crafting table node.
Design: workbench tile → interact opens crafting menu → recipe list → consume materials → produce tool.
Save the crafted-tools state in `SaveData`.

### Coin economy / shop NPC
`ITEM_COIN` exists and is earnable but has no use. Add a shop NPC tile or node type.
Simple: interact opens a buy menu, deduct coins, add item.

### Tool tiers
Bronze → Iron → Steel axe/pickaxe. Higher tier = fewer hits required (reduce `ACTION_TICKS`).
Add `active_tool_tier` to `GameState` or derive from equipped tool slot.

### Map editor or official map
Current map is hand-crafted binary (`assets/maps/map.bin`). Either:
- Document the `.map` format (already in `world.c` comments) and build a tile-paint tool, OR
- Commit the map source (e.g. a CSV or text format) and generate `.bin` at build time.

---

## Low priority / polish

### Break animation frames
M4 implemented crack overlays but skipped the 4-frame break animation (tree falls, rock shatters).
Requires new sprite frames in the iso sheet and `ANIM_DEPLETED` state in the world.

### Regrowing animation
When `node_respawn` expires, the tile snaps back. A short grow-back animation would feel better.

### Drop item on ground
Allow player to drop an item (deselect from inventory → place as world entity).

### Pico map-not-found diagnostic
`while(1){}` halt gives no feedback on hardware. Add LED blink or display error string.

### Selected hotbar slot (active slot) — done
`state.active_slot` tracks the last-used tool. HUD hotbar and BAG-tab grid
both highlight it with `C_BORDER_ACT`. Explicit set via A on a hotbar slot in
the BAG menu; auto-set when a skill action starts.

---

## Known design decisions (locked)

- Inventory: hotbar 0–7, bag 8–27, tools fixed at slots 4–7
- No alpha blending on Pico — use checkerboard translucency everywhere
- No `hal_draw_pixel` — only `hal_fill_rect`
- `world_init` returns false (game aborts) if `map.bin` not found — no procedural fallback
- Save version: 11
