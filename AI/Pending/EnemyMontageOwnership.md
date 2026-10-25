# EnemyMontageOwnership — pending

Goal: a boss pattern's montage is never overridden by the server's replicated ability montage, and patternless enemy abilities (bomb, turret) still replicate theirs without cutting another montage.
Docs written: `Abilities/Base/CLAUDE.md`, `Abilities/Pattern/CLAUDE.md`, `Abilities/Boss/CLAUDE.md`, `AI/MCP/MCP_Animation.md`.

## Code (to implement)
- [x] `Pattern/Pattern.h` — public `bool HasMontage() const { return IsValid(AnimMontage); }`
- [x] `Base/GeoGameplayAbility.h/.cpp` — `virtual bool ShouldPlayAbilityMontage() const { return true; }`; `HandleAnimationMontage` returns first when it is false (before the section and `ASC->PlayMontage` logic)
- [x] `Base/PatternAbility.h/.cpp` — override: false when `PatternToLaunch` is set and `GetDefault<UPattern>(PatternToLaunch)->HasMontage()`
- [x] `GeoGameplayAbility.cpp:73` comment "plays montage on client, timer-only on server" — fix to the real behaviour (every machine, server included, replicates)
- [x] `GeoGameplayAbility.cpp:367-369` comment — add that a pattern ability skips this, the pattern plays its own
- [x] Log a warning in `UPatternAbility` setup when the ability has an `AnimMontage` and its pattern has one too (the ability's is ignored)

## Tests (`AI/CodingStyle.md`, "Tests")
- [x] `Private/Tests/GeoMontageOwnershipTests.cpp` (+ `GeoTestPatternAbility.h`) — low-level: pattern ability with a montage-pattern → `ShouldPlayAbilityMontage()` false; pattern without montage → true; plain `UGeoGameplayAbility` → true. Row for it in `Public/CLAUDE.md` "Private-only".
- [x] Run: `Tools\Run_Tests.bat MontageOwnership` — 1 passed

## Editor / Blueprint (bridge: `AI/Python/Runtime/run_via_bridge.py`, MCP clients down)
- [x] Shim `UGeoAnimBuilderUtil::InsertSlotAfter` (+ `FindAnimGraph`), documented in `GeoTrinityEditor/Public/Tool/CLAUDE.md`
- [x] Build (editor closed), reopen the editor
- [x] Parent AnimBP `/Game/Characters/Anim/SK_GeoAnimBlueprint` (HexBoss child has no graph; graph is Idle → BlendListByBool → `DefaultSlot` → Root): `InsertSlotAfter(parent, DefaultSlot, CastSlot, Cast)` — registers group `Cast`, node layers over the default slot
- [x] Bomb and turret montages (`SK_HexBoss_Montage_Bomb` / `_Turret`): slot track on `CastSlot`, sections Start / Fire kept (renamed in place on `slot_anim_tracks`)
- [ ] Any other patternless enemy ability montage that plays while a pattern runs: same slot
- [ ] Pattern ability BPs that set both `AnimMontage` and a montage on their pattern: clear the ability's copy (the new warning lists them)
- [x] Add the technique (shim, child-AnimBP-has-no-graph) to `AI/MCP/MCP_Animation.md`

## Verify
- [x] Headless test above passes
- [ ] Bridge probe / dedicated-server run: boss pattern montage keeps its compensated start section on a client while a bomb cast plays (PIE only if the user asks)

## Status 2026-10-25
C++, tests and docs verified. Remaining (editor/unverifiable): AnimBP slot insertion + bomb/turret montage tracks, other patternless enemy ability montages, pattern ability BPs with both montages, boss montage start-section probe.
