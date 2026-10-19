# Tutorial — pending

Goal: an MMO-parody tutorial narrated by a geometric Cat, framed by a tabletop-RPG-gets-absorbed lore, designed with the user in shareable artifacts before any code.
Docs written: `AI/Tutorial/CLAUDE.md`, `Tone.md`, `Lore.md`, `Cat.md`, `IntroBoard.md`, `Steps.md`. No code folder touched yet.

## Design (now)
- [x] Tutorial Lore artifact (shareable)
- [x] Tutorial Board artifact: pages 1-3
- [ ] User edits text in the artifacts; Claude reads them back and syncs the docs
- [x] Board pages 4 (Move) and 5 (Dash) + `Steps.md` steps 1-2
- [x] Board pages 6-10 (Shoot, Deploy, Special, Three dummies, Miniboss placeholder) + `Steps.md` steps 3-7
- [x] Board pages 10-12 (zone colors, lethal wave, safe spots), miniboss moved to page 13 + real-world closing joke
- [x] Miniboss brief drafted (page 13, `Steps.md` step 10)
- [ ] Decide: lateral wave every 3 dummies *killed* or *spawned*; miniboss HP and the 3 points
- [ ] Dummy spawn: landing damage hit (reuse the pillar's damage zone, `Actor/Deployable`) then a growing zone at its place, removed when it dies; growth rate/time to fill the map to tune (what happens when the map is full: to decide)
- [ ] Ally dummy low-health callout ("I need healing" + geometric ninja-head icon): trigger threshold, icon asset (`GeoIcon` polylines), shown on the ally's head
- [ ] Death-triggered Cat advice: one tip per death for the miniboss, in order, nothing during the fight
- [ ] Scripted miniboss loot: 1 of each chip (bypass the drop table roll); shard grant = one chip's Forge cost (`Gem/CLAUDE.md`); forced Forge step; first-open of the gem sheet and the Forge
- [x] Board page 16 (Training Room) + `Steps.md` step 13
- [ ] Design the Training Room map (class select, training dummies, zones, teleport to lobby; no boss portals, bosses are in the lobby): own `CLAUDE.md` and Pending file once designed
- [ ] Decide: where the "back to the real world" joke lands (miniboss kill vs end of Forge); N chips to slot
- [ ] Miniboss implementation: 3-point movement, projectile + dummy summon patterns, kill-converts-to-ally dummy (health, healable, cover, shoots the boss)
- [ ] Check the real palette hues (`GameDataSettings::ColorPalette`) and redraw the zone colors on pages 10-12 (hues are placeholders)
- [x] Decided: tutorial is Healer (Circle) only; pages and scenes updated (no per-class text)
- [x] Decided: the tutorial forces the Circle; other classes are discovered in the Training Room
- [ ] Tutorial level forces the Circle class (no class selection before the Training Room)
- [ ] Check the story: pages 1-2 absorb all three friends, page 3 lands only the Circle (Cat line to explain where the others are, or leave it as a mystery)
- [ ] User reviews pages 4-10 text; Dash key label on page 5 is a placeholder (check the real binding)
- [x] Step 2 Cat reactions: lines written in the Tutorial Board page 5 (whistle, 4 idle tiers, first-try stunned, 1st fall + lesson, 2nd fall bold key, 3rd+ bold + shake bank of 3, after-falls, closing); the board renders `**bold**` and `~~shake~~`
- [ ] User validates the page 5 text; extend the 3rd+ bank to ≥ 4 lines; tune the idle tier delays (seconds)
- [ ] User sends the next tutorial steps -> add pages + `Steps.md` rows
- [ ] Decide: language (EN draft, FR?), tabletop game name, Cat palette slot, is the Cat the final boss

## Placement and flow (decided)
- The tutorial is built on the Encounter system (`AI/Pending/Encounter.md`, done first): no tutorial director class. One `AGeoEncounter` per area/step group; each board step = one encounter step (`OnEnter` actions, rules with triggers, `bCompletesStep`).
- Tutorial level block in `DraftMap`, further from the hub than the current room (X≈-6000, which becomes the Training Room: rename its tag `Arena.Tutorial` → `Arena.TrainingRoom`, fix `tutorial_room.py`). Own `Arena.*` tags, camera volumes and bounds points; rooms linked by barriers opened with `SetBarrierClosedAction`.
- Played solo on a listen server (server-only encounters, no client sync of Cat lines needed).
- Specific triggers/actions to write as Encounter children: ability used, ability unlock/lock + ability page, dialogue (Cat line, `[on death]` flag, skippable; widget in `GeoTrinityUI` through `HUD/Interface/`), grant loot/shards, open gem sheet/Forge, chip slotted/forged, ally-dummy low-health.
- [ ] Open decisions: entry (menu button vs hub teleporter), forced Circle scoped to the tutorial area (`StartingClassOverride` is per level today), temporary profile vs real profile for scripted loot/shards

## Code (to implement, after the design is validated and the user says go)
- [ ] Write the `CLAUDE.md` of each code folder touched (UI intro screens in `GeoTrinityUI`, level for the landing, dialogue system) before coding
- [ ] Intro screens: layered images + drift data (`AI/Tutorial/IntroBoard.md`)
- [ ] Dialogue box with skippable Cat lines; line text supports a **bold** run and a **shake** run (rich-text styles), key names resolved from the input binding
- [ ] Step 2 flow: fall counter fed by the `FallRespawn` event, outcome (first-try reach / fall N / reach after falls) selects the Cat line + animation; whistle idle state before the first outcome
- [ ] Cat animations for step 2: whistle (note glyph + tail sway), stunned (jaw drop, wide eyes, spin)
- [ ] Tests per `AI/CodingStyle.md` ("Tests")

- [ ] Friendly (ally) dummy with low health for steps 4-5 (same actor as the miniboss ally dummy); step 5 adds an enemy dummy; done checks: full health, enemy dead
- [ ] Ability locking per tutorial step + auto-open of the ability page on unlock
- [ ] Three dummy behaviours: chaser, shooter, mover (data on dummy enemies)

- [ ] Zone room: scripted zones by color, forced-lethal wave, waves with safe-spot holes (reuse the existing hazard patterns, `Abilities/Pattern/CLAUDE.md`)

## Editor / Blueprint
- [ ] Landing level asset + Cat hologram mesh (shapes in `Cat.md`)
- [ ] Layer art for pages 1-2
- [ ] Tutorial level: move marker (step 1), void line + far platform (step 2), void width between a walk-off and 500 cm

## Verify
- [ ] Headless tests only unless the user asks for PIE
