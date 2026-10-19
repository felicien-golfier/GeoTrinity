# Steps

The teaching steps after landing. One Tutorial Board page per important step. Content arrives from the user later; this is the skeleton.

| # | Step | Teaches | Status |
|---|---|---|---|
| 0 | Landing | Meet the Cat, get control | Drafted (`IntroBoard.md`, page 3) |
| 1 | Move | WASD | Drafted (below) |
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
Action the player does: walk onto a glowing floor marker.
Done when: player overlaps the marker (no timer). Idle too long → Cat nags, never blocks.
On fail: n/a (nothing to fail).
In-game source of the text: Tutorial Board page 4.

## Step 2 — Dash
Teaches: dash crosses a void line; it is not invincibility.
Cat angle: tutorials + "even in these shapes, you stay useless humans" (the joke closes the step).
Action the player does: dash across a line of void to the far platform.
Done when: player stands on the far platform.
On fail: fell in → respawn (`TargetPoint.FallRespawn` rule of `GeoHexArena`); the Cat reacts by fall count (below); retry, never a dead end.
Must say (on the first fall): the dash does not make you invincible; only falling is skipped mid-dash.

Cat reactions (the Cat watches the whole step; no line before an outcome):
| Moment | Cat |
|---|---|
| Arrival at the void, before any attempt | Silent. Whistling: a music-note glyph by the mouth, tail sways. No caption, no instruction: the player must try. |
| Idle at the void (no attempt, or standing still after a respawn) | Punchlines, never blocking, escalating by idle time (4 tiers; seconds tunable): casual whistling quip → "staring at a hole doesn't fill it" → hint that the keyboard has more than four keys → absurd filler ("naming the pixels"). Last tier repeats from a small bank. Lines in Board page 5. |
| Far platform reached with 0 falls | Stunning reaction: jaw drop, hexagon eyes widen, a full spin. One line of disbelief that hints it expected a fall ("...no. Do it again, I wasn't looking."). Closes the step with the existing "useless humans" joke. |
| 1st fall | Fake sympathy + the lesson: "Oh, you fell and died? What a shame... I didn't see it coming." Then the explanation: press Dash before the edge, it crosses the void in 0.2 s, falling is skipped mid-dash, it is not invincibility. The Dash key shown in the line. |
| 2nd fall | Mockery. The Dash key in **bold**. |
| 3rd fall and more | Mockery escalates; the Dash key in **bold**, the dying-again word in **bold**, the text vibrates (shake). Each further fall: same style, new mockery line from a bank; never a hint stronger than the key. |
| Reached after falls | Dry relief, no stunning reaction (it is for the first try only). |
Rules: fall count is per step visit, reset when the step is left. Lines skippable (`Tutorial/CLAUDE.md`, invariants); the player may dash while the Cat talks. Mockery targets the dying, never the player's skill alone and always carries the key (`Cat.md`: insult + useful fact). The Dash key text is read from the real binding, never typed.
Level rule: the void is narrower than the dash (500 cm / 0.2 s, `Abilities/Common/CLAUDE.md`) and wider than a walk-off step; built with the hex-arena hole rule (`Actor/Arena/CLAUDE.md`), not a new fall system.
In-game source of the text: Tutorial Board page 5.

## Step 3 — Shoot
Teaches: basic attack. Unlocks it.
Action: destroy a training dummy (a `GeoDummyArena` enemy, `Actor/Arena/CLAUDE.md`).
Done when: dummy dead. Text: Board page 6.

## Step 4 — Deploy
Teaches: the Healer's deployable, the Healing Zone, and healing with it. Unlocks it.
Scene: a friendly dummy with low health (the ally dummy of step 10) next to the player.
Action: drop the Healing Zone on the friendly dummy until it is full.
Done when: the friendly dummy is back to full health (not only when the zone lands). Text: Board page 7.

## Step 5 — Special
Teaches: the Special (the Healer's beam), which interacts with the deployable. Unlocks it. The Cat stays vague on how; the mechanics are on the ability page (single source, never restated in Cat lines).
Scene: two dummies, a hurt friendly one and an enemy one.
Action: kill the enemy dummy and fully heal the friendly one with the beam.
Real deployable rules, no tutorial exception: 3 charges, slow refill (`Abilities/Common/CLAUDE.md`, "Charges"). The Cat tells the player to spend them wisely.
Done when: the enemy dummy is dead AND the friendly dummy is at full health, both with the beam. Text: Board page 8.

## Step 6 — Three dummies
Teaches: combining shoot + deploy + special + dash.
Three dummies arrive: Chaser (runs at the player), Shooter (fires bullets), Mover (patrols, harmless).
Done when: all three dead. Then → step 7 (miniboss). On wipe: respawn, retry. Text: Board page 9.

## Step 7 — Zone colors
Teaches: a zone's color says what it does (`EGeoColor`: Heal, Shield, Damage; hues from the palette, never invented here). A wind-up zone wears the color of the effect it announces (`Tool/Visual/CLAUDE.md`).
Cat angle: customer service; "the color is the whole game".
Action: stand in each zone color as zones appear.
Done when: player has stood in every color shown. Text: Board page 10.

## Step 8 — Lethal wave
Teaches: the lethal color kills, and you will die.
Inevitable by design: the telegraph covers the whole floor and lands faster than a dash can leave the room.
Cat angle: the Cat *plays* with the player. Cheerful warning as the wave telegraphs ("Big wave is coming! Avoid it!"), then fake sympathy on death ("Sorry, I forgot to leave you room to survive..."), then "get used to it, dying is part of the job". Blames the wave, not the player.
Done when: player dies once (forced). Respawn free. Text: Board page 11.

## Step 9 — Safe spots
Teaches: read the hole in the lethal color and move into it before it lands.
The Cat adds the safe spots nicely ("don't tell the other cats"). Several waves; spots move each wave and shrink in number.
Done when: N waves survived (N tunable). On death: respawn, wave restarts, gentle comment. Text: Board page 12.

## Step 10 — Miniboss
Teaches: the combined fight; reuses every earlier step (dash, deployable, Special, zone colors, lethal wave + safe lines).
- Miniboss moves between 3 map points and fires projectiles; then summons dummies that fire projectiles.
- Each dummy lands with a damage hit at its spawn (telegraphed in the Damage color, like the pillars); then a zone grows at its place until it is killed. Low damage, but it never stops growing until the whole map is filled (pressure, not burst).
- A killed dummy turns ally and its zone is gone. As the Healer the player heals it (healing the dummies is a core verb of the fight). Strategy on offer: kill the dummies, use the allies for damage. It is optional (a tutorial): the player may kill the miniboss directly; "done" is the miniboss dead by any means.
- Ally dummies show a short callout when low on health: "I need healing", with a small head icon in front of the text. The icon is a flat geometric ninja head (visor accent) nodding to a certain hero shooter, never a copy of its character art (`Tone.md`, limits). It is a UI callout, not a voice: the Cat stays the only speaker.
- The Cat gives strategy advice only on the player's death (one tip per death, in order: dummies + growing zone, allies, lateral wave). Silent otherwise. Each tip ends with a sarcastic jab (the tip stays plain, the insult wraps it, `Tone.md`).
- A killed dummy turns ally: has health, healable, blocks bullets (cover), shoots the boss.
- Every 3rd dummy (killed; to confirm vs spawned) a lateral lethal wave crosses the room with safe lines to pass through.
Done when: the miniboss is dead. On the kill: the Cat's joke that it is time to come back to the real world (ends the tutorial). Text: Board page 13.

## Step 11 — Gems
Teaches: gems and sockets (`Gem/CLAUDE.md`). The miniboss drops one copy of each Chip (scripted, not rolled; the 7 chips of `Data/gems.csv`).
Done when: the player has slotted chips (N tunable). Gem/sheet UI opens on first drop, like the ability page. Cat stays vague on stats (the gem page says them). Text: Board page 14.

## Step 12 — Forge
Teaches: the Forge turns shards into a chip.
Grant: exactly the shard cost of one chip, no more. Forced: the player must forge their first chip.
Done when: a chip is forged. End of the tutorial; leads to the Training Room (step 13). Text: Board page 15.
Open: the "back to the real world" joke is on the miniboss kill (page 13); it may move to the end of this step.

## Step 13 — Training Room
The tutorial is over; the player is brought to the Training Room: not the lobby. No objective, nothing forced.
- Class selection (the three classes; the tutorial was Healer-only).
- Training dummies and zones to experiment with.
- No boss portals: the real bosses are directly accessible from the lobby.
- A teleport to the lobby (where the bosses are).
Text: Board page 16 (tutorial's last page).

## Unlock rule (steps 3-5)
- Each new ability opens the ability page (the ability-tutorial UI, `AI/Artifacts.md`) the moment it unlocks; this is generic, not per step.
- Abilities are locked until their step; nothing fires early.

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
