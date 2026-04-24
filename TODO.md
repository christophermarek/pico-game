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

### Day/night cycle + energy regen pacing
`TICKS_PER_DAY = 20` and `TICK_MS = 10000` → a day is 200 s (3.3 min).
Decide if that's right. Energy only regens on tick — consider smoothing or tuning the rate.

---

## Medium priority

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

### Selected hotbar slot (active slot)
Currently no slot is "selected" — pressing a hotbar key to equip isn't wired.
Add `uint8_t active_slot` to `GameState`, render it with a bright border, use it for tool dispatch in `player_do_action`.

---

## Known design decisions (locked)

- Inventory: hotbar 0–7, bag 8–27, tools fixed at slots 4–7
- No alpha blending on Pico — use checkerboard translucency everywhere
- No `hal_draw_pixel` — only `hal_fill_rect`
- `world_init` returns false (game aborts) if `map.bin` not found — no procedural fallback
- Save version: 11
