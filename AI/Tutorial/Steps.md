# Steps

The teaching steps after landing. One Tutorial Board page per important step. Content arrives from the user later; this is the skeleton.

| # | Step | Teaches | Status |
|---|---|---|---|
| 0 | Landing | Meet the Cat | Drafted (`IntroBoard.md`, page 3); same room and encounter as step 1 |
| 1 | Move | WASD, get control | Drafted (below) |
| 2 | Dash | Dash over a void line; dash ≠ invincible | Drafted (below) |
| 3 | Shoot | Basic attack on a dummy | Drafted (below) |
| 4 | Deploy | Class deployable | Drafted (below) |
| 5 | Special | Special acting on the deployable | Drafted (below) |
| 6 | Three dummies | Combine everything | Drafted (below) |
| 7 | Zone colors | Heal / shield / damage zones by color | Drafted (below) |
| 8 | Lethal wave | Lethal color; you die once | Drafted (below) |
| 9 | Safe spots | Move into the safe spot of lethal waves | Drafted (below) |
| 10 | Miniboss | Combined fight | Drafted (below) |
| 11 | Gems | Loot chips, fill sockets | Drafted (below) |
| 12 | Forge | Craft your first chip | Drafted (below) |
| 13 | Training Room | Tutorial over; free map | Sketched (below) |

## Class
The whole tutorial is played as the Healer (Circle), forced: no class choice. The other classes are discovered after, in the Training Room (step 13). No class adaptation anywhere; the Board scenes from page 3 show the Circle alone. Healing is a taught verb: the Healing Zone (step 4) and healing the ally dummies (step 10).

## Step 1 — Move
Teaches: WASD moves your shape.
Cat angle: tutorials ("press W to walk"), premium-feature parody.
Action the player does: walk onto the room's exit teleporter.
Shape: steps 0 and 1 are one room and one encounter (the Landing room): the Landing lines then the Move lines, said in a row. Movement is locked until the Cat says the WASD line (the Move lesson's first line), then free for the rest of the talk (`GeoSetMovementLockedAction`, `Encounter/Action/CLAUDE.md`); the lines stay skippable. After the last Move line the room's exit pad opens (it is the target: no floor marker); the pad appearing is the cue to step on it.
Done when: player steps on the open pad (it teleports them to the Dash room; no timer). Idle once the pad is open → Cat nags, never blocks.
Hand-off: the closing line of this step ("You reached a portal by walking. Truly the peak of the genre. Next lesson: gravity.") is not said here but is the first line of the Dash room, on arrival, together with the Dash unlock (step 2).
On fail: n/a (nothing to fail).
Cat staging (the 3D Cat, `AGeoCat`, ordered by the Cat actions, `Encounter/Action/Cat/CLAUDE.md`):
| Moment | Cat |
|---|---|
| Room start, movement locked | Shown, then reassembles from the splat (`AM_Cat_Reassemble`) before the first line; the first line waits for it |
| Landing lines | Idle, facing the player; one line said while it walks its path (`Tut_Landing_CatPath_*`, a line where the cat moves) |
| WASD line (movement unlocked) | Roams the room until the last line |
| Last line ("See the round portal…") | The pad opens; the Cat walks to it and portal-jumps into it, gone while the line still shows |
In-game source of the text: Tutorial Board pages 3 and 4.

## Step 2 — Dash
Teaches: dash crosses a void line; it is not invincibility.
Cat angle: tutorials + "even in these shapes, you stay useless humans" (the joke closes the step).
Action the player does: dash across a line of void to the far platform.
Done when: player stands on the far platform.
On fail: fell in → death, respawn at the room's `TargetPoint.Entrance` after `DeathTime` (GameState policy); the Cat reacts by fall count (below); retry, never a dead end.
Must say (on the first fall): the dash does not make you invincible; only falling is skipped mid-dash.

Unlock: Dash is locked until the player enters this room; the arrival grants it (`GeoUnlockAbilityAction`, `Unlock rule` below), so the Landing room needs no dash block.
Cat reactions (the Cat watches the whole step; no line before an outcome). On arrival the Cat first closes step 1 (its "reached a portal by walking" line, above); the "arrival at the void" silence starts after it:
| Moment | Cat |
|---|---|
| Arrival at the void, before any attempt | Silent. Whistling: a music-note glyph by the mouth, tail sways. No caption, no instruction: the player must try. |
| Idle at the void (no attempt, or standing still after a respawn) | Punchlines, never blocking, escalating by idle time (4 tiers; seconds tunable): casual whistling quip → "staring at a hole doesn't fill it" → hint that the keyboard has more than four keys → absurd filler ("naming the pixels"). Last tier repeats from a small bank. Lines in Board page 5. |
| Far platform reached with 0 falls | Stunning reaction: jaw drop, hexagon eyes widen, a full spin. One line of disbelief that hints it expected a fall ("...no. Do it again, I wasn't looking."). Closes the step with the existing "useless humans" joke. |
| 1st fall | Fake sympathy + the lesson: "Oh, you fell and died? What a shame... I didn't see it coming." Then the explanation: press Dash before the edge, it crosses the void in 0.2 s, falling is skipped mid-dash, it is not invincibility. The Dash key shown in the line. |
| 2nd fall | Mockery. The Dash key in **bold**. |
| 3rd fall and more | Mockery escalates; the Dash key in **bold**; the dying-again word shakes (`~~`), the only shaking run (the markup never nests). Each further fall: same style, new mockery line from a bank; never a hint stronger than the key. |
| Reached after falls | Dry relief, no stunning reaction (it is for the first try only). |
Rules: fall count is per step visit, reset when the step is left. Lines skippable (`Tutorial/CLAUDE.md`, invariants); the player may dash while the Cat talks. Mockery targets the dying, never the player's skill alone and always carries the key (`Cat.md`: insult + useful fact). Key names (Dash and every other) are read from the real binding through the HUD's key-label system (the ability bar's live key label, `Source/GeoTrinityUI/Public/HUD/AbilityBar/CLAUDE.md`), never typed.
Level rule: the void is narrower than the dash (500 cm / 0.2 s, `Abilities/Common/CLAUDE.md`) and wider than a walk-off step; a real hole in the floor, a fall dies on the world `KillZ` (`Characters/CLAUDE.md`, "Void"); no fall system of its own, the hex-arena tile rule is not used. The void is map geometry: `tutorial_map.py` builds the Dash room as two floors (near, far platform) with the hole between, plus the platform volume; the encounters script only reads them.
In-game source of the text: Tutorial Board page 5.

## Step 3 — Shoot
Teaches: basic attack. Unlocks it.
Action: destroy a training dummy (a `GeoDummyArena` enemy, `Actor/Arena/CLAUDE.md`).
First dummy: alone, standing at the room's centre (`TargetPoint.Tutorial.ShootFirst`); nothing else in the room, no card yet. Its death montage plays out (`FirstDummyDeathSeconds`, 2 s, placeholder), and only then the Cat's quest message, the card and the nine dummies appear together.
Quest gag (joke target: endless kill quests, `Tone.md`): once the first dummy has died and the delay is over, a quest card (UI, not a voice) shows "Cull the Dummies, KILLED n/100" and a 30:00 countdown as if the quest ended then (decoration only: the cut below comes first, and 00:00 does nothing); no respawn timer shown. Nine dummies spawn at once, in 3 groups of 3, and each one respawns 5 s (tunable) after its own death, at the spot it died. The Cat quips at 3, 6 and 9 kills.
Layout (so the Healer beam, a forward rectangle, never reaches more than one group): each group is 3 dummies in a line on its own ray from the room's entry spot, the 3 rays wide apart (≥ 60°), a group's dummies close enough to share one beam, groups far enough that one beam never covers two. Layout only, no cap on the beam. The Cat cuts it at the first of: 12 kills, 45 s after the card. The cut: every dummy bursts at once, the counter jumps to 100/100, the Cat keeps the reward (joke only, no currency exists), then leads to step 4. Dummies are harmless.
Done when: the quest is cut (never when 100 is reached). Text: Board page 6.

## Step 4 — Deploy
Teaches: the Healer's deployable, the Healing Zone, and healing with it. Unlocks it.
Scene: a friendly dummy with low health (the ally dummy of step 10) next to the player.
Action: drop the Healing Zone on the friendly dummy until it is full.
Done when: the friendly dummy is back to full health (not only when the zone lands). Text: Board page 7.
Hurt ally (steps 4 and 5): spawned by the step (`GeoSpawnEnemiesAction`, `BP_Dummy`), its setup = player team, `StartHealthRatio` 0.1, its own `Tag`. Nothing else sets its health. The Deploy room's first encounter step, `Setup`, only spawns (the ally passes through full health while it starts, so the heal step watches it after). The Special room has no `Setup`: its fight step spawns both dummies itself, a spawned actor's health being read on the next tick.

## Step 5 — Special
Teaches: the Special (the Healer's beam), which interacts with the deployable. Unlocks it. The Cat stays vague on how; the mechanics are on the ability page (single source, never restated in Cat lines).
Scene: two dummies, a hurt friendly one and an enemy one, in their own room behind the step 4 barrier: a second ally (spawned, like every tutorial dummy), starting hurt like the first (step 4's ally stays full behind).
Action: kill the enemy dummy and fully heal the friendly one with the beam.
Real deployable rules, no tutorial exception: 3 charges, slow refill (`Abilities/Common/CLAUDE.md`, "Charges"). The Cat tells the player to spend them wisely.
Done when: the enemy dummy is dead AND the friendly dummy is at full health, in either order, AND the Special was activated at least once during the step. Each goal counts once (`bRepeat` false): an ally that dips and is healed back never counts twice. Which ability did the killing or healing is not checked (not observable on a death); the usage check is what makes the lesson real.
Special never used when both goals are met: not done. Both goals are tallied in one counter (enemy dead +1, ally full +1, Special used +10): 2 = the sting, 12 = done. The Cat stings (a "cinglante" line, `Cat.md` voice: insult + useful fact, the Special key printed with `{Key:<InputTag>}`, never typed: you did all that without the one thing you were taught), both dummies respawn (the fight step re-enters itself: leaving kills the survivors, entering spawns them again), retry. Used once = the step ends normally, however it was done after. Text: Board page 8, the sting is in-game only.

## Step 6 — Three dummies
Teaches: combining shoot + deploy + special + dash.
Three dummies arrive: Chaser (runs at the player), Shooter (fires bullets), Mover (patrols, harmless).
Done when: all three dead. Then → step 7 (zone colors). On wipe: respawn, retry. Text: Board page 9.

## Step 7 — Zone colors
Teaches: a zone's color says what it does (`EGeoColor`: Heal, Shield, Damage; hues from the palette, never invented here). A wind-up zone wears the color of the effect it announces (`Tool/Visual/CLAUDE.md`).
Cat angle: customer service; "the color is the whole game".
Action: stand in each zone color as zones appear.
Done when: player has stood in every color shown. Text: Board page 10.
Shape: one color at a time (heal, shield, damage), each its own encounter step: entering spawns that color's zone on its fixed spot (`GeoSpawnZoneAction`, the color's `TargetPoint.Tutorial.Zone<Color>`; no wind-up); a placed box over the spot completes the step. The box is the square inscribed in the zone's circle, so being in the box is being in the zone. The step enters with the spawn, so the spot is never empty while it is watched.

## Step 8 — Lethal wave
Teaches: the lethal color kills, and you will die.
Inevitable by design: the telegraph covers the whole floor and lands faster than a dash can leave the room.
Cat angle: the Cat *plays* with the player. Cheerful warning as the wave telegraphs ("Big wave is coming! Avoid it!"), then fake sympathy on death ("Sorry, I forgot to leave you room to survive..."), then "get used to it, dying is part of the job". Blames the wave, not the player.
Done when: player dies once (forced). Respawn free. Text: Board page 11. Step 9 starts only once the player is back (`DeathTime`), through the same wait step as a step-9 death.

## Step 9 — Safe spots
Teaches: read the hole in the lethal color and move into it before it lands.
The Cat adds the safe spots nicely ("don't tell the other cats"). Several waves; spots move each wave and shrink in number.
Done when: N waves survived in a row (N = the waves listed). On death: the series stops, gentle comment once the player is back, then the series restarts from wave 1 after `DeathTime`. Text: Board page 12.
Shape: step `SafeSpots` runs the waves in order (one ability each, its spots and count), each awaited, then marks the series done; a player death leaves for `SafeSpotsWait` (a `DeathTime` timer), which goes back to `SafeSpots`. Leaving cancels the running wave. Counters reset on each entry, so nothing counts across a death. The room's exit pad opens from the finishing rule (after its Cat line), never from `OnExit`: a death leaves the step too.

## Step 10 — Miniboss
Teaches: the combined fight; reuses every earlier step (dash, deployable, Special, zone colors, lethal wave + safe lines).
- Miniboss: 500 HP. Moves between 3 map points, each a `TargetPoint` placed in the level (movable in the editor); fires projectiles; then summons dummies that fire projectiles.
- Each dummy lands with a damage hit at its spawn (telegraphed in the Damage color, like the pillars); then a zone grows at its place until it is killed. Low damage, but it never stops growing until it reaches its max size (pressure, not burst); at max it stays and keeps ticking its low damage until the dummy dies.
- A killed dummy turns ally and its zone is gone: has health, healable, blocks bullets (cover), shoots the boss. As the Healer the player heals it (healing the dummies is a core verb of the fight). Strategy on offer: kill the dummies, use the allies for damage. It is optional (a tutorial): the player may kill the miniboss directly; "done" is the miniboss dead by any means.
- When an ally dummy is low on health, the Cat calls it out (allies are spawned, many: a `GeoHealthThresholdTrigger` with `WatchedClass` = the ally class, `Below`, watches every one, spawned ones included): three Cat lines, one per firing (`bRepeat`, the trigger's `Count` 1 / 2 / 3, silent after): (1) the Cat reminisces about an old friend who jumped around barking "I need healing", named Genji: "'I need healing.' Reminds me of my old friend Genji. Jumped around the whole map, barking it at anyone who'd listen. Nobody ever came."; (2) it pities the lone dummy: "Poor dummy, yawning 'I need healing' with no one around to help. If ONLY we had a healer in the team..."; (3) the cry, "I need healing. **I NEED HEALING.** ~~I NEED HEALING~~". Same Cat reaction as to a death. No UI bubble or icon: the Cat stays the only speaker, and the name is a wink, never a copy of the hero shooter's art (`Tone.md`, limits). Text: in-game source (the Board's page 13 notes still describe the old icon callout).
- The Cat gives strategy advice only on the player's death (one tip per death, in order: dummies + growing zone, allies, lateral wave). Silent otherwise. Each tip ends with a sarcastic jab (the tip stays plain, the insult wraps it, `Tone.md`).
- Every N dummies *spawned* a lateral lethal wave crosses the room. N = `CyclesRequired` of the `STTask_SendEventAfterNCycles` after the summon in the miniboss State Tree (tunable in the editor).
- Its safe spots are not random: each living dummy, summoned or ally, makes a safe lane through its own place, there from the wave's start. A dummy that dies loses its lane at once, even mid-wave. So dummies are worth keeping alive (and healing), which pulls against killing the summoned ones for their growing zone. No dummy alive = no safe spot (the wave is then lethal everywhere; the Cat's third tip covers it). Only this wave: step 9's waves keep their fixed layout.
- A player death is a wipe (solo): the arena resets the fight. Summoned dummies and allies are the arena's adds, so the reset removes them (zones die with their dummy); the State Tree restarts, so the wave count does too. Each attempt starts clean.
Done when: the miniboss is dead. The kill grants the normal class XP of its `BossType` on every run, plus its `BossLoot`. On the kill: the Cat's joke that it is time to come back to the real world (the joke lands here; the loot and Forge steps follow). Text: Board page 13.
Shape: the room is a real boss fight (an `AGeoArena`, `BossType` MiniBoss, no leaderboard; aggro and wipe are the arena's, `Actor/Arena/CLAUDE.md`), not a dummy arena. The boss's tree: `Start` (its salve at every player begins and keeps firing on its own) → `Move` (straight walk to another `TargetPoint.Tutorial.MinibossPoint`, no navmesh, `STTask_MoveToTargetPoint`) → `Summon` → `CountSummons` (every `CyclesRequired` entries sends the wave event) → `Wave` (only when the event came) → `Move`. The lateral wave starts on the room's `TargetPoint.Tutorial.WaveStart` and crosses along +X whatever the boss faces (`TravelYaw`). The summoned dummy shoots the players (same salve, its own ASC); the ally it turns into shoots the boss (`bAimAtArenaBoss`, bullets hostile only). The encounter only talks (intro, one tip per death, ally call-outs, kill line) and opens the exit pad. Built by `tutorial_miniboss.py` (assets) and `tutorial_encounters.py` (room, arena, steps).

## Step 11 — Gems
Teaches: gems and sockets (`Gem/CLAUDE.md`). The miniboss drops one copy of each Chip (scripted, not rolled; the 7 chips of `Data/gems.csv`).
Done when: a chip is slotted during this step (an event, not a state: chips already slotted, e.g. on a replay, do not count). The menu guide leads the player there (Esc menu → Character → Gems), the path highlighted and nothing else clickable: a guide the player must follow, never a menu opened for them (to be designed, `AI/Pending/MenuGuide.md`). Cat stays vague on stats (the gem page says them). Text: Board page 14.

## Step 12 — Forge
Teaches: the Forge turns shards into a chip.
Grant: exactly the shard cost of one chip, no more. Forced: the player must forge their first chip.
Real profile, every run (`Flow.md`, "Replay and loot"): the grant is repeated on a replay.
The menu guide leads to the Forge page (as step 11).
Done when: a chip is forged. End of the tutorial; leads to the Training Room (step 13). Text: Board page 15.

## Step 13 — Training Room
The tutorial is over; the player is brought to the Training Room (in the shared map, `Flow.md`): not the lobby. No objective, nothing forced.
- Class selection (the three classes; the tutorial was Healer-only).
- Training dummies and zones to experiment with.
- No boss portals: the real bosses are directly accessible from the lobby.
- A teleport to the lobby (where the bosses are).
Text: Board page 16 (tutorial's last page).

## Tuning
- Every tutorial number lives in one curve table, `CT_Tutorial` (`Content/AbilitySystem/Data/CurveTables/`, next to `CT_BossHex` etc.): one row per knob, read through `FScalableFloat` like the other kits. Placeholder values until the user tunes them.
- Rows: `MinibossHealth` (500), `DummyHealth`, `AllyDummyHealth`, `DummyLandingDamage`, `DummyZoneDamage`, `DummyZoneGrowthTime`, `DummyZoneMaxRadius`, plus `MinibossProjectileDamage`, `MinibossProjectileSpeed` (the miniboss's salves) and `AllyProjectileDamage` (the ally dummies' salves at the boss).
- Not in the table: the lateral wave interval N (`CyclesRequired` in the miniboss State Tree), the 3 miniboss points (level `TargetPoint`s), the hurt ally's 10% start (its `DefaultAttributes` effect) and the ally low-health ratio (`Ratio` on the placed trigger).

## Unlock rule (steps 3-5)
- Locked = not granted: the tutorial map starts the Circle without its dash, basic attack, deployable and Special; the slot appears on the ability bar when its step grants it. Dash is locked too, granted on arrival in the Dash room (step 2); the Landing room has no dash.
- Each grant ends on the ability page: the Character sheet (`HUD/Menu/Character/CLAUDE.md`) with that ability's detail drawer open; the player closes it. Generic, not per step.
- Cat first, page after: the unlock's Cat line ("An ability page is about to pop up. Read it. Or don't, I'm a cat, not a cop.") is said and gone before the page opens, so it never appears out of nowhere. Order (a rule's actions or the step's `OnEnter`): the room's lines, the unlock line, the grant, then the page; the slot and the page arrive together, after the Cat. The game holds while the menu is open (`Flow.md`, "Menus pause"), so the next line or timer runs once it is closed.

## Step template
```
Step N — <name>
Teaches: one verb / one rule
Cat angle: which joke target (Tone.md) wraps it
Action the player does: ...
Done when: observable condition
On fail: Cat reaction
In-game source of the text: <file/asset>
```

## Rules
- A step teaches one thing.
- "Done when" is observable by gameplay code, never by a timer alone.
- Failure is allowed, commented on, never a dead end.
