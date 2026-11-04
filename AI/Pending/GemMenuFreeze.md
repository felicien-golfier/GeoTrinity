# GemMenuFreeze — pending

Goal: every Gems menu action (slot, ALL, unequip, echo; probably craft / break) must feel instant; today a slot click freezes ~450 ms, also on the listen server.
Priority: 0
Docs written: `Source/GeoTrinityUI/Public/HUD/Menu/Gems/CLAUDE.md` (`CommitSlotting`, in-place stack rows)

## Report (measured 2026-11-04, Development editor, listen-server host, one slot click)
- `UGeoGemSubsystem::CommitChanges`: `Profile->Save` 0.5 ms, `SendLoadoutsToServer` **428–452 ms**, `OnProfileCommitted` 0.3 ms.
- Menu side after the in-place change (`CommitSlotting`): sockets 0.4, unequip button 0.3, stacks 0.3, detail 0.3, totals 1.1, page broadcast 0.2 ms. UI is not the cost.
- `SendLoadoutsToServer` = 3 `ServerSetLoadout` RPCs (one per class), run locally on the host. The one for the current class (`UpdateLoadoutAndResetAttributes`, when `!IsMatchInProgress` and the pawn is alive) calls `APlayableCharacter::ResetAttributes` → default attributes effect + `UGeoGemComponent::ApplyGems` (remove / reapply stats effect, clear + `GiveAbility` + `TryActivateAbility` of every Core passive) + health / ammo refill. Cause inside it not pinned yet: finer timing is in the build, one slot click away.
- Open question: a remote client pays the same on the server (RPC), so the host case is the proof, not the exception.

## Read map
- `Source/GeoTrinity/Private/Gem/GeoGemSubsystem.cpp` — `CommitChanges` (save, send, broadcast)
- `Source/GeoTrinity/Private/Gem/GeoGemComponent.cpp` — `SendLoadoutsToServer`, `UpdateLoadoutAndResetAttributes`, `ApplyGems`
- `Source/GeoTrinity/Private/Characters/PlayableCharacter.cpp` — `ResetAttributes`, `GiveLife`
- `Source/GeoTrinity/Private/GameClasses/Match/GeoPlayerState.cpp` — other `UpdateLoadoutAndResetAttributes` caller (line ~65)
- `Source/GeoTrinityUI/Private/HUD/Menu/Gems/GeoGemLoadoutWidget.cpp` — `CommitSlotting`, `UpdateStacks`
- `Source/GeoTrinityUI/Private/HUD/Menu/Gems/GeoGemForgeWidget.cpp` — Forge still rebuilds on every craft / break (`Refresh`)
- `Source/GeoTrinity/Public/Gem/CLAUDE.md` — commit / server apply rules

## Code (to implement)
- [ ] Read the `GemTiming` lines of one slot click (`Saved/Logs/GeoTrinity.log`) → name the slow part of `ApplyGems` / `ResetAttributes`; fix that part, not around it.
- [ ] Send only the edited class's loadout (and only when it changed) instead of all three classes per commit.
- [ ] Do not run the full `ResetAttributes` (health / ammo refill, passive re-grant) for a loadout edit that only changes gems; update the stats effect / Core passives that changed. Keep behaviour: refill only where a spawn needs it.
- [ ] Forge: craft / break / break-all update in place like `CommitSlotting` (tier break rows, gem rows); tier tabs and rate table untouched.
- [ ] Remove every `TEMP-TIMING` block: `GeoGemSubsystem.cpp` (`CommitChanges`), `GeoGemComponent.cpp` (`ApplyGems`), `PlayableCharacter.cpp` (`ResetAttributes`), `GeoGemLoadoutWidget.cpp` (`CommitSlotting`).
- [ ] Update `Gem/CLAUDE.md` (commit path) and `Gems/CLAUDE.md` (Forge) to the final behaviour.

## Verify
- [ ] Test (`GeoTrinity.Gems`, lightest layer that proves it): committing a loadout edit sends one class's loadout and leaves health / ammo and Core passive handles untouched when only the gems of the shown class changed.
- [ ] Quick suite (`GeoTrinity.` minus bot sessions); tutorial gem steps (`EquipChip` / `ForgeChip`) are at risk: run the covering bot sessions from `Tests/Tutorial/CLAUDE.md` Coverage table.
- [ ] User: slot / ALL / unequip / craft / break feel instant, host and client.
