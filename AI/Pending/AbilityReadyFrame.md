# AbilityReadyFrame — pending

Goal: a slot / passive tile frame lights only when the ability has something ready to use; each ability decides through one virtual, no new replication. Same idea for the player card's class gauge.
Docs written: `Abilities/CLAUDE.md`, `Abilities/Base/CLAUDE.md`, `Abilities/Common/CLAUDE.md`, `Abilities/Circle/CLAUDE.md`, `Abilities/Square/CLAUDE.md`, `Abilities/Triangle/CLAUDE.md`, `Abilities/Gem/CLAUDE.md`, `GeoTrinityUI/.../HUD/CLAUDE.md`, `HUD/AbilityBar/CLAUDE.md`, `HUD/Team/CLAUDE.md`.

## Code
- [x] `Abilities/Base/GeoGameplayAbility.h/.cpp` — `IsReady(ASC)`, `GetClassGauge(ASC, Fill, bReady)`
- [x] `Abilities/Common/GeoDeployAbility` — `GetCurrentStacks(ASC)` / `GetMaxStacks(ASC)` read the given ASC (CDO-safe; call sites in HUD, character sheet, satellite ring updated); `IsReady` = stacks > 0
- [x] `Abilities/Circle/GeoSweetSpotChargePassiveAbility` — `IsReady`, `GetClassGauge`
- [x] `Abilities/Square/GeoShieldBurstPassiveAbility` — `IsReady` via `UShieldBurstPassiveComponent::GetGaugeRatio()`
- [x] `Abilities/Square/GeoSacrificeDetonateAbility` — `SacrificeForFullGauge` (200); `IsReady` = armed; `GetClassGauge` fill = value / 200, ready = armed
- [x] `Abilities/Triangle/GeoRecallTurretAbility` — `GetClassGauge`
- [x] `GeoTrinityUI/HUD/GeoHUD` — `IsAbilityReady(Tag)`
- [x] `GeoTrinityUI/HUD/AbilityBar/GeoAbilitySlotWidget.cpp` — frame = `Fill <= 0 && HUD->IsAbilityReady(Tag)` (in `SetCooldownFill`, which every tick branch calls)
- [x] `GeoTrinityUI/HUD/Team/GeoPlayerCardWidget` — loops granted specs; casts, `SacrificeForFullGauge` and includes dropped
- [x] BP override check: no asset in `Content/` references `SacrificeForFullGauge` → default 200 kept, nothing to copy

## Tests
- [x] `Source/GeoTrinity/Private/Tests/GeoAbilityReadyTests.cpp` — `GeoTrinity.AbilityReady.Rules` (Bond, HealReturn, Wake ± cooldown tag, sweet-spot 0 / 150 / 300, detonate unarmed / armed at 100)
- [ ] Build: `set GEO_NO_PAUSE=1 && Tools\Build_Editor.bat` (editor closed)
- [ ] Run: `set GEO_NO_PAUSE=1 && Tools\Run_Tests.bat GeoTrinity.AbilityReady` (needs a full editor restart for the new test to register)

## Verify
- [ ] Tests green (parsed report); in-game look only if the user asks (PIE)
