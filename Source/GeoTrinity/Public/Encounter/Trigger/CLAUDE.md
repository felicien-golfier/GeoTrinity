# Encounter/Trigger

Stock triggers (base: `../GeoEncounterTrigger.h`). Each is a C++ class and a base for Blueprint children. Payload `Count` is the trigger's own running number since `OnArm`.

| File | Fires when | Payload |
|---|---|---|
| `GeoVolumeEnteredTrigger.h` | A player overlaps the referenced box (also from `OnArm` for each player already inside) | `Actor` = player, `Count` = entries so far |
| `GeoActorFilter.h` | `FGeoActorFilter { Class, Tag }`: which characters a trigger watches; `Matches(Actor)` |
| `GeoActorDiedTrigger.h` | A character matching `Watched` (`FGeoActorFilter`; `APlayableCharacter` = player deaths) dies | `Actor` = the dead, `Count` = deaths so far, `Value` = watched still alive |
| `GeoHealthThresholdTrigger.h` | A character matching `Watched` (`FGeoActorFilter`, spawned ones included) reaches the `Ratio` side chosen (`Above`: ≥, `Below`: ≤), also from `OnArm` when already there | `Actor` = the actor, `Count` = crossings since arm, `Value` = ratio |
| `GeoEventTrigger.h` | `UGeoEncounterSubsystem` raises its `Tag` | The raised payload, untouched |
| `GeoCounterTrigger.h` | The named step counter of its encounter changes | `Count` = new value |
| `GeoPlayerIdleTrigger.h` | Every `Delay` s the local player stays within `Radius` of where it stood still (Cat nags, idle tiers) | `Actor` = player, `Count` = idle tier |
| `GeoGemProfileTrigger.h` | The first local player's gem profile does `Event`: `Equipped` (`OnGemEquipped`, a gem slotted; never what was slotted before arming) or `Crafted` (`OnGemCrafted`) | `Count` = occurrences so far, `Value` = quantity (`Crafted`) |
| `GeoAbilityActivatedTrigger.h` | The first local player's ASC activates an ability carrying `AbilityTag` (the ASC's ability-activated delegate, never a call planted in the ability) | `Actor` = player, `Count` = activations so far |

- A trigger chooses what it watches (which box, which actors, which tag); it reports every occurrence with its count, and the rule's `Condition` decides which ones count (`Count == 3`, `Count` multiple of 3).
- No trigger carries a delay or fires on a step starting: what happens as a step starts, after a wait or not, is the step's `OnEnter` with a `GeoDelayAction` (`Action/CLAUDE.md`). A wait that ends a step: `OnEnter` ends on a counter add that a rule completes on.
- Watching: by filter only, never a level reference: every watched character is spawned (tutorial dummies, `Encounter/Action/CLAUDE.md`) or a player. `Tag` (actor tag, None = any) tells apart spawned variants sharing one class (`Characters/CLAUDE.md`, "Enemy setup"); set by the spawner's `Setup.Tag`.
- Deaths: `AGeoGameState::OnCharacterDied` (`GameClasses/Match/CLAUDE.md`), one binding for every character, spawned or placed; the filter runs at death time, so no spawn tracking. A character that never dies (`ResetToFullLifeWhenReachingZero`) never fires.
- Health: per watched actor, its ASC's native attribute delegates for Health and MaxHealth (`GetGameplayAttributeValueChangeDelegate`, ASC from `GeoASLib::GetGeoAscFromActor`, so a player's lives on its PlayerState), bound with the actor as payload; ratio over the live `MaxHealth`. Side state is per actor. Watched set: the live matching characters at `OnArm`, then each one spawned while armed (`UWorld::AddOnActorSpawnedHandler`, removed in `OnDisarm`). A spawned actor is matched and watched on the next tick (`SetTimerForNextTick`), never in the handler: the handler runs before `FinishSpawning`, so its setup tag and starting health are not there yet; by the next tick `BeginPlay` has set both, so a rule on a spawned actor's health may sit in the step that spawns it. A ratio with MaxHealth 0, or a dead character (a body still in the world, killed at full health), is never on any side. Fires again only after leaving and re-reaching its side (sitting at full does not fire every heal tick).
- Idle: polled every `PollSeconds` (no movement delegate exists); a move past `Radius` or a death (a respawn teleports, so it reads as a move too) puts the tier back to 0; tier N fires `N × Delay` s of stillness after the last move.
- Gem trigger: the profile lives on the local player (`Gem/CLAUDE.md`); the solo tutorial host runs the encounter in the same process, so the trigger binds that profile directly.
- Ability trigger: counts activations since `OnArm`, so an ability used in an earlier step never counts. It only reports; the step keeps the fact in a counter (`SpecialUsed`) because the rule that reads it sits elsewhere in the step (`Tutorial/Steps.md`, step 5).
- A new trigger (menu page opened) is a Blueprint or C++ child written with the feature that needs it, with its own row here.
