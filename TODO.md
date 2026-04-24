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

### Swing-and-collide action model (design note)
Current model: press A → `find_action_target` picks one tile, a 30-frame
timer runs, `skill_complete_action` fires once at the end. Simple, works
for single-target gathering.

Swing-and-collide would replace the timer with a physical arc:
- `action_ticks_left` still drives animation phase, but the tool's
  on-screen bounding box is the hit-region each frame.
- Each frame during the swing: test the bbox against every adjacent
  tile center; any tile overlapped during the arc that hasn't already
  been hit this swing gets a hit callback.
- A per-swing `hit_this_swing[MAP_CELLS/8]` bitset avoids double-hits.
- `NodeAction` gets an optional `on_hit` callback separate from
  `on_complete` so the grant-item / XP path runs at impact rather than
  at timer end.

Trade-offs: enables AOE gathering and combat naturally, feels more
responsive, but adds per-frame collision work (cheap — max 9 tiles to
check) and requires tool bbox data per swing frame. Revisit when combat
lands — gathering alone doesn't need it.

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
