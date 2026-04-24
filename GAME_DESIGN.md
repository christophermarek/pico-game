# GrumbleQuest — Game Design

**Platform:** Raspberry Pi Pico (ST7789 240×240) + SDL2 desktop simulator.
**Scope:** Compact single-player RPG. Target play time ~5–8 hours to
completion; L99 all skills for completionists.

This doc is the design contract. Anything here is committed. Anything on
the cut list stays cut. New features get proposed in PRs against this
doc BEFORE code — if it's not on the spine, it doesn't ship.

---

## Meta goal

> **Build your homestead at the edge of a wild world. Journey outward to
> harder regions for better materials. Fight what lives there. Carry it
> home.**

Two alternating movements:

- **Homestead** — safe, you're building it up.
- **Journey** — dangerous, you're fighting for loot.

They feed each other: better homestead → better gear for journeys →
better loot → better homestead.

## Fantasy

You're the newcomer on the outskirts of a small village. The shopkeeper
takes pity on you, hands you shears, and puts you to work trimming the
grass around his stall. From that first coin you claw your way out —
your own tools, your own roof, and eventually your own expeditions into
the woods and mountains for things the village shopkeeper can't sell
you.

---

## Progression spine

Eight beats. Each is an ordered checkpoint; you cannot reach beat N+1
without finishing N. Each represents ~20–60 minutes of play.

| # | Beat | Player focus | Gates opened |
|---|------|--------------|--------------|
| **1** | **Tutorial** | Shopkeeper gives shears → clear grass around his shop → earn first coins. | Map, coins, shears. |
| **2** | **Oak & stone age** | Buy bronze axe + bronze pickaxe from the shop. Gather oak, stone, copper, tin in the homestead. Woodcut and Mining climb to ~15. | Basic gathering loop. |
| **3** | **Build the homestead** | Workbench → shack → chest → campfire. Smelt bronze at the campfire, craft your own bronze gear. Shack = respawn. | Crafting, smelting skill (L1), storage, HP regen station. |
| **4** | **Forest expedition** | First outer zone. Requires a bronze sword. Harvest pine logs and iron ore between fights with slimes and wolves. | Combat, zone travel, tier-2 materials. |
| **5** | **Forge & iron tier** | Build + place a forge. Smelt iron. Craft iron axe / pickaxe / sword / rod. Smelting climbs to 15+. | Forge, tier-2 tools. |
| **6** | **Mountain foothills** | Second outer zone. Maple trees (WC 30), silver ore (MN 30). Tougher enemies; need a steel sword. | Tier-3 materials. |
| **7** | **Workshop & steel tier** | Build workshop. Craft steel tools and steel rod. Smelting → 30+. | Steel loop. |
| **8** | **Mountain peak + boss** | Third outer zone. Yew (WC 50), gold (MN 50), dark water shark (FS 50). Boss drops the diamond core that gates tier-4 **diamond** tools. Defeat boss → ending. | Tier-4 materials, game completion. |

---

## Skills

**Four skills.** Levels 1–99. OSRS XP curve: precomputed 99-element
table. Formula for reference:

```
xp_for_level(L) = (1/4) * sum_{i=1}^{L-1} floor(i + 300 * 2^(i/7))
```

Reference thresholds the table must hit:

| Level | XP | Level | XP |
|------:|------:|------:|------:|
| 1  | 0        | 40 | 37 224      |
| 10 | 1 154    | 50 | 101 333     |
| 15 | 2 411    | 70 | 737 627     |
| 20 | 4 470    | 85 | 3 258 594   |
| 30 | 13 363   | 99 | 13 034 431  |

| Skill | Source | L1 | L15 | L30 | L50 |
|-------|--------|----|-----|-----|-----|
| Woodcut | Chop trees | Oak | Pine | Maple | Yew |
| Mining | Mine rocks/ore | Stone / copper / tin | Iron ore | Silver ore | Gold ore |
| Fishing | Fish water | Pond | River | Deep | Dark |
| Smelting | Smelt at campfire / forge | Bronze (L1 campfire) | Iron (L15 forge) | Steel (L30 forge) | Diamond (L50 forge) |

**L50 is the design "end" — the boss is beatable at L50 skills with
steel gear.** L51–99 is completionist grinding; the runs after beating
the boss are where the diamond tier lives.

### Per-hit XP

| Tier | WC | MN | FS | Smelt (per ingot) |
|:---:|:--:|:--:|:--:|:----:|
| 1   | 25 | 20 (stone), 15 (copper/tin) | 15 | 5 (bronze)  |
| 2   | 45 | 40 | 35 | 10 (iron)   |
| 3   | 75 | 70 | 65 | 25 (steel)  |
| 4   | 120| 110| 100| 50 (diamond)|

Tuned so a focused grind to L15 is ~30 min, L30 is ~2–3 hrs, L50 is
~8–10 hrs.

---

## Resource tiers

### Woodcut

| Tier | Node | Level required | Drop |
|:----:|------|---------------:|------|
| 1 | Oak tree   | 1  | Oak log   |
| 2 | Pine tree  | 15 | Pine log  |
| 3 | Maple tree | 30 | Maple log |
| 4 | Yew tree   | 50 | Yew log   |

### Mining

| Tier | Node | Level | Drop |
|:----:|------|------:|------|
| 1 | Stone rock | 1  | Stone   |
| 1 | Copper ore | 1  | Copper ore |
| 1 | Tin ore    | 1  | Tin ore    |
| 2 | Iron ore   | 15 | Iron ore   |
| 3 | Silver ore | 30 | Silver ore |
| 4 | Gold ore   | 50 | Gold ore   |

### Fishing

| Tier | Node | Level | Drop |
|:----:|------|------:|------|
| 1 | Pond       | 1  | Raw fish   |
| 2 | River      | 15 | Raw trout  |
| 3 | Deep water | 30 | Raw salmon |
| 4 | Dark water | 50 | Raw shark  |

**Level alone gates access.** A bronze axe can chop a yew tree if
Woodcut is 50. Tool tier is a speed multiplier, not a block.

Missing level fires `"Your Woodcut is too low"` (the level-gate log).

---

## Tool tiers

Four tiers, four tool types (axe, pickaxe, sword, rod). Shears are
unique — one shears, buy once.

| Tier | Tools | Harvest / combat speed | Gate |
|:----:|-------|-----------------------:|------|
| 1 | Bronze  | 1.0× | Beat 3 (campfire smelts bronze) |
| 2 | Iron    | 1.5× | Beat 5 (forge smelts iron) |
| 3 | Steel   | 2.0× | Beat 7 (workshop crafts, forge smelts steel) |
| 4 | Diamond | 3.0× | Beat 8 (boss drops diamond core) |

Speed multiplier: base action frames / tier multiplier. Base gathering
is 30 frames; a diamond axe chops an oak in ~10 frames instead.

Shopkeeper sells a **starter** bronze axe / pickaxe / shears / bronze
rod at beat 2, so the player can gather before the campfire is built.
From beat 3 onward, all tool tiers are crafted, not bought.

---

## Zones

Three maps. `assets/maps/{homestead,forest,mountain}.bin`.

### Zone: Homestead

- **What's here:** shopkeeper NPC, buildable plots, tier-1 resources
  (oak, stone, copper, tin, pond).
- **Danger:** none. No enemies.
- **Resource respawn:** **never.** Homestead trees and ore are
  one-shot. The homestead is for getting started and building up, not
  grinding. Grinding happens in outer zones.
- **Exits:** to Forest (unlocked after beat 3).

### Zone: Forest

- **What's here:** tier-1 + tier-2 resources. Slimes, wolves.
- **Resource respawn:** **1 in-game day (~2 min wall-clock)** after
  depletion.
- **Exits:** back to Homestead; to Mountain (unlocked after beat 5).

### Zone: Mountain

- **What's here:** tier-2 + tier-3 + tier-4 resources. Tougher enemies.
  Boss arena at the peak.
- **Resource respawn:** **1 in-game day** after depletion.
- **Exits:** back to Forest.

---

## Structures

Placed at the homestead only. Outer zones are not buildable.

| Structure | Beat | Function | Cost |
|-----------|:----:|----------|------|
| **Workbench** | 3 | Craft tools, weapons, placeable structures. | 10 oak + 5 stone |
| **Shack**     | 3 | Respawn point. First home. | 20 oak + 15 stone |
| **Chest**     | 3 | Storage: 20 slots per chest. **Multiple chests allowed.** | 10 oak + 2 stone |
| **Campfire**  | 3 | (1) Smelts bronze (copper + tin → bronze ingot). (2) Cooks raw food. (3) Standing within 2 tiles passively regens HP and energy. | 8 oak + 3 stone |
| **Forge**     | 5 | Smelts iron / steel / diamond ingots. | 25 stone + 5 iron ore |
| **Workshop**  | 7 | Craft steel + diamond gear. Replaces workbench for tier-3+. | 20 stone + 15 iron ingot + 10 maple |

All structures persist in save. Placing one converts the underlying
walkable floor into the structure's occupied footprint. Structures are
non-walkable.

### Chest

- **Capacity:** 20 slots per chest. Distinct `Inventory` struct per
  chest instance.
- **Multiple chests:** allowed. No hard cap for now; we'll tune if save
  size becomes a problem.
- **Interaction:** A-press on a chest opens a 2-pane UI — the player's
  28-slot inventory on top, the chest's 20 slots below. D-pad moves
  cursor; A transfers the selected item one-at-a-time.
- **Save:** serialise each chest's inventory in the save blob.

### Campfire

- **Smelting:** tier-1 only (1 copper + 1 tin → 1 bronze ingot, 5 XP).
- **Cooking:** raw fish → cooked fish. Recipe list below.
- **Regen:** while the player is within 2 tiles, +1 HP AND +1 energy
  per game tick (every 2 s). Proximity-triggered, no "sitting" state.
- **Multiple campfires:** allowed (same rules as chests).

### Forge

- **Smelting only.** No combat, no healing.
- Tier-2 and up: iron / steel / diamond.
- Smelting XP flows through the Smelting skill.

### Workshop

- Tier-3+ tool/weapon crafting.
- Can also craft tier-1/2 as a superset of the workbench (workbench
  becomes obsolete but doesn't disappear — the player chose to build
  it).

---

## Food & consumption

Edible items can be consumed directly from the BAG menu (A on a food
item with an inventory cursor — tools still set active_slot, foods
consume).

| Item | On eat | Stack |
|------|--------|------:|
| Raw fish    | −10 HP, −5 energy  | 99 |
| Raw trout   | −15 HP, −5 energy  | 99 |
| Raw salmon  | −20 HP, −10 energy | 99 |
| Raw shark   | −30 HP, −10 energy | 99 |
| Cooked fish   | +20 HP, +10 energy | 99 |
| Cooked trout  | +30 HP, +15 energy | 99 |
| Cooked salmon | +40 HP, +20 energy | 99 |
| Cooked shark  | full heal (HP + energy) | 99 |

**Design intent:** raw = punishment if eaten by accident; cooking is
the reward. Cooking is done at the campfire (Beat 3+). Cooking is NOT
a skill — it's a recipe. Level-gating cooking doesn't add interesting
decisions; we save skill slots for Smelting.

---

## Crafting & recipes

Three crafting stations:

- **Workbench** (Beat 3) — tools (bronze/iron), weapons, placeables.
- **Forge** (Beat 5) — smelts iron/steel/diamond ingots.
- **Workshop** (Beat 7) — steel + diamond tools, weapons.

Campfire is NOT a craft station; it's a smelter (bronze only) and a
cooker.

| Recipe | Station | Inputs | Output | Smelt lv |
|--------|---------|--------|--------|:--:|
| Bronze ingot | Campfire | 1 copper + 1 tin | 1 bronze ingot | 1 |
| Iron ingot   | Forge    | 1 iron ore      | 1 iron ingot   | 15 |
| Steel ingot  | Forge    | 1 iron ingot + 1 stone | 1 steel ingot | 30 |
| Diamond ingot| Forge    | 1 gold ore + 1 diamond core | 1 diamond ingot | 50 |
| Bronze tool  | Workbench | 1 bronze ingot + 2 oak log | tool/weapon | — |
| Iron tool    | Workbench | 1 iron ingot + 2 oak log | tool/weapon | — |
| Steel tool   | Workshop  | 1 steel ingot + 2 oak log | tool/weapon | — |
| Diamond tool | Workshop  | 1 diamond ingot + 2 yew log | tool/weapon | — |
| Cooked fish/trout/salmon/shark | Campfire | 1 raw counterpart | 1 cooked | — |
| Workbench    | (bare hands on grass) | 10 oak + 5 stone | Workbench | — |
| Shack        | Workbench | 20 oak + 15 stone | Shack | — |
| Chest        | Workbench | 10 oak + 2 stone | Chest | — |
| Campfire     | Workbench | 8 oak + 3 stone | Campfire | — |
| Forge        | Workbench | 25 stone + 5 iron ore | Forge | — |
| Workshop     | Workbench | 20 stone + 15 iron ingot + 10 maple | Workshop | — |

Shopkeeper inventory (for gating beat 2 before any crafting):

- Bronze axe, bronze pickaxe, shears.
- Bronze rod.
- No higher-tier items ever sold.

---

## Combat

One weapon type: the sword (four tiers).

- **Action:** A-press swings. Same raycast/swing primitive the
  gathering tools use — on overlap, damage the entity.
- **Enemies:**
  - **Slime** (forest, easy) — slow, low HP, low damage.
  - **Wolf** (forest + mountain, mid) — chases player.
  - **Troll** (mountain, heavy) — slow, high HP, high damage.
  - **Boss** (mountain peak) — scripted unique enemy.
- **Player HP:** existing bar. Death → respawn at shack with full HP,
  no item loss. (MVP rule; revisit if trivial.)
- **Enemy HP:** per-entity. Tick-based attacks. Player damage scales
  with sword tier.

Combat is deliberately shallow. Journeys feel risky because *one wolf
will kill you at L1*, not because combat is mechanically deep.

---

## Item catalogue

Final item list. Keep this tight — every extra item costs a sprite
slot.

**Tools/weapons (16):** 4 tiers × { axe, pickaxe, sword, rod } = 16.
**Special:** shears, bronze core? (no), diamond core.
**Resources (9):** oak log, pine log, maple log, yew log, stone, copper
ore, tin ore, iron ore, silver ore, gold ore.
**Ingots (4):** bronze ingot, iron ingot, steel ingot, diamond ingot.
**Fish — raw (4):** raw fish, raw trout, raw salmon, raw shark.
**Fish — cooked (4):** cooked fish, cooked trout, cooked salmon,
cooked shark.
**Currency/misc (2):** coin, diamond core.

Total: 16 + 1 + 1 + 10 + 4 + 4 + 4 + 2 = **42 items.** Fits in
`item_id_t` (uint8) with lots of headroom.

---

## Systems this spine forces

| System | First beat | Notes |
|--------|:--:|-------|
| OSRS XP table | 2 | Precomputed 99-element array. |
| Tiered `NodeAction` with `level_required` | 2 | Extends existing `actions.c` table. Tool tier becomes a speed multiplier. |
| Smelting skill | 3 | Fourth skill. XP from ingot smelting. |
| Copper + tin + bronze smelting | 3 | New ores, new ingot type. |
| Structure placement + save | 3 | `Structure` list in `GameState`, serialised. |
| Crafting UI | 3 | Menu tab; recipes greyed when inputs missing. |
| NPC + dialog box | 1 | Shopkeeper. |
| Shop UI | 1 | Buy menu, coin economy. |
| Chest UI (two-pane transfer) | 3 | 20-slot inventory per chest. |
| Zone travel (world loader) | 4 | Swap active `World`, per-zone save state. |
| Per-zone respawn flag | 4 | Homestead = never. Outer zones = 1 day. |
| Combat entities + AI + damage | 4 | Enemy struct, tick-based attack, swing-overlap damage. |
| Forge + smelt recipes | 5 | Interact opens smelt menu. Smelting XP. |
| Campfire smelt (bronze) + cook (fish) + regen | 3 | Interact opens action menu; proximity regen per frame. |
| Food consumption from BAG | 3 | A on edible in menu → apply effect, remove 1. |
| Workshop tier-3 crafting | 7 | Same crafting UI with recipes filtered by station. |
| Boss fight + ending screen | 8 | Scripted enemy; win → credits. |

---

## Cut list (do NOT do these)

If you want one of these, it must replace something on the spine. No
additive scope.

- Farming / crops / seasons
- Multiple save slots
- Day/night cycle beyond the tick counter
- Weather
- Romance / social sim
- Tool tiers beyond four (no "mithril after diamond")
- Stone house / shack upgrades
- Cooking as a separate skill (recipe only)
- Quests beyond the shopkeeper tutorial
- Full village population (1–3 NPCs total)
- More than three zones
- Multiplayer

---

## Open questions (non-blocking)

- **Enemy drops.** Tentative: slime 1–3 coins, wolf 2–5 coins + hide,
  troll 5–10 coins + bone. Hide/bone as craftable materials? Probably
  not — cuts scope. Coins only.
- **Boss drops 3 diamond cores.** Enough for all three diamond
  weapons. Boss respawns? **No.** One-shot. Completionist grind is
  farming tier-4 resources, not refighting the boss.
- **Chest cap.** No hard limit for now. Revisit if save blob blows
  past a reasonable size.
- **Shopkeeper second-tier consumables (bandages, bread)?** Not in
  MVP — the campfire + cooked fish covers consumables.

---

## Implementation roadmap (derived — not the design)

The design above is the contract. Order we build:

1. **OSRS XP table + tiered `NodeAction`.** Extends `actions.c`. Level
   gates access; tool tier becomes speed multiplier. Copper/tin ore
   tiles added to map.
2. **NPC + dialog box + shopkeeper + shop UI + coin purchase.** Beat 1
   playable end to end.
3. **Crafting UI at a dummy workbench.** Ugly but proves recipe data.
4. **Structure placement + save.** Real workbench, shack, chest,
   campfire. Bronze smelting at campfire. Smelting skill wired. Food
   consumption from BAG. **Beat 3 complete.**
5. **Zone travel infrastructure.** Load two maps, exit tile, per-zone
   save state. Walk between zones.
6. **Combat entities + sword swing + damage + enemy AI.** Beat 4
   playable.
7. **Forge + smelt recipes + per-ingot smelting XP.** Beat 5 complete.
8. **Third zone + mountain enemies + tier-3 resources.** Beat 6
   playable.
9. **Workshop + steel + diamond groundwork.** Beats 7 & 8
   foundations.
10. **Boss + ending screen.** Beat 8 complete. **Ship.**
11. **Production-readiness pass.** Full codebase review before we cut
    any "1.0" tag. See the review checklist below.

Between each milestone: playtest, update this doc if something locked
turned out to be wrong.

---

## Production-readiness review (milestone 11, explicit)

Before tagging 1.0 we do a full pass, not incremental cleanup. The pass
is structured — same checklist every time — so reviewers (human or
agent) run the same playbook and produce comparable reports.

### Scope

Every file under `src/`, `tests/`, `tools/`, `assets/`, plus the
top-level `Makefile`, `CMakeLists.txt`, `pico/CMakeLists.txt`,
`.gitignore`, and the docs. No skipping.

### Three-agent review protocol

Run in parallel, each gets the full diff / file set as context:

1. **Reuse / duplication.** Look for logic that's duplicated across
   files, inline helpers that should use an existing utility, tables
   that should merge, switch statements repeated in different
   callers. Output: file:line list + concrete consolidation.
2. **Code quality.** Dead parameters, stale comments, unused fields,
   stringly-typed values where an enum exists, structs that could be
   smaller, functions that should be static, redundant state. Output:
   file:line + specific fix.
3. **Efficiency + Pico-fitness.** Per-frame float math in hot paths,
   per-frame scans that should be event-driven, unbounded memory,
   large stack locals (Pico has 4 KB), stdio in the game-loop path,
   heap allocations anywhere. Output: file:line + measured or
   estimated cost.

### Artifact & file hygiene (always cut)

Explicitly delete if found:

- Dead commented-out code blocks
- Comments that narrate what the code obviously does
- Unused `static` helpers, unused `#define`s, unused typedefs, unused
  struct fields
- `TODO` / `FIXME` that have no owner and no plan
- Build artefacts under `build/` committed to git
- `.DS_Store`, `__pycache__`, `.pyc`, editor swap files
- `pico_assets/*` stale atlas dumps that no longer match `manifest.json`
- Placeholder-generator scripts (we commit placeholders, not generators)
- Any file the build doesn't reference and isn't listed as an asset

### Correctness checks (the rules)

- Every public function in a `.h` has a one-sentence doc comment or a
  name so obvious the comment would just repeat it.
- Every `.h` is self-contained (compiles standalone) and minimises
  transitive includes.
- `static` everything that isn't called from another `.c`.
- No global mutable state outside `GameState`, `World`, and
  module-local `static` scratch buffers. No new file-scope `static`
  state without a comment explaining why.
- No heap allocation in the game loop. Verify with a grep for
  `malloc`/`calloc`/`realloc`/`strdup`.
- No `fprintf` / `printf` / `stdio` behind `PICO_BUILD`.
- `hal_fill_rect` / `hal_pixel` are the ONLY drawing primitives the
  game touches. Anything else → HAL boundary violation.
- All per-frame float math (hypotf, sinf, cosf, division) is
  accounted for; flag anything that runs every frame without a
  profile justification comment.

### Structure & organisation

- Folders: `src/game/`, `src/render/`, `src/ui/`, `src/` for the HAL
  and main. Nothing else. If a file wants to live elsewhere, argue
  it in the PR.
- Naming: `snake_case` functions, `UPPER_SNAKE` macros, `PascalCase`
  types. No exceptions except legacy field names in existing
  structs (fix in the review pass if spotted).
- No file over 500 lines without a justification comment at the top.
- Headers import only what they need; implementation files close
  their includes on the last line that uses them.

### Deliverables of the review

- Single commit with all the deletions + consolidations.
- Short `REVIEW_YYYY_MM_DD.md` summary (NOT retained after tag — it's
  a changelog for that pass) listing what was found and what was cut.
- Test suite still green; sim still runs identically.
- `GAME_DESIGN.md` updated if any systems turned out to be redundant
  or unused at tag time.

The review is not incremental maintenance. It's a discrete milestone
with its own "done" criteria, and it ships **in its own PR** separate
from any feature work.
