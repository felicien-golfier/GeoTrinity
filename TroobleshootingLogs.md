**Enter here all your issues and fixes you got when setup this project, upgrade Unreal engine etc..**

# How to send a cmd through network in editor : 
ServerExec slomo 0.5
---

# Project Setup & Engine

## UE 5.7.2 — 30/01/2026
commit `562880e0ffc015ddce292007cde1ac44ac83389e`
- Had to update manually `Source/GeoTrinity.Target.cs` and `Source/GeoTrinityEditor.Target.cs`
- BitFlags now require the full Enum path (e.g. `/Script/GeoTrinity.ETeam`)
- **Rider load failure**: had to install the new Visual Studio with .Net 8.0 manually
- Forgot to download the PDB for Unreal in the launcher

**After each pull (from Oliver):**
Open a command prompt and run:
```
"C:\Program Files\Epic Games\UE_5.x\Engine\Build\BatchFiles\Build.bat" GeoTrinityEditor Win64 Development "C:\Path\To\Project.uproject"
```

---

# GAS — Gameplay Cues

## Passive ability won't auto start if ClientOnly OR ClientPredicted (That is the default value)

## ExecCalc attribute capture: Source vs Target
When adding a new ExecCalc that captures a multiplier attribute (e.g. `HealMultiplier`), capture from **Target** if the GE is applied by a non-character source (deployable, pickup). Capture from **Source** only when the caster/attacker is the source and owns the attribute (e.g. `DamageMultiplier` on the attacker).

If `FindCaptureSpecByDefinition` returns null with `bOnlyIncludeValidCapture=true`, the capture spec exists but `HasValidCapture()` is false — the source ASC doesn't own the attribute set that contains that attribute.

## Gameplay Cue not showing when fired from `OnFireTargetDataReceived` — 29/04/2026

**Symptom:** A deployable's explosion Gameplay Cue plays correctly on proximity overlap but is invisible when the same `Recall(true, ...)` call is made from `OnFireTargetDataReceived` (e.g. `GeoDetonateAllMinesAbility`).

**Root cause:** GAS prediction-key collision.
When `Fire()` calls `Super::Fire()` → `ServerSetReplicatedTargetData(key=K)`, the server receives the target data and calls `OnFireTargetDataReceived` with scoped prediction key K still active. When it fires `ExecuteGameplayCue`, GAS calls `NetMulticast_InvokeGameplayCueExecuted` with key K. On the **owning client**, `NetMulticast_InvokeGameplayCueExecuted_Implementation` checks `!PredictionKey.IsLocalClientKey()` → `false` (key K *is* the local client's key) → **the cue is skipped**. GAS assumes the client already played it predicted in `Fire()`, but `Fire()` never fired it, so the owning client sees nothing.

Proximity overlap avoids this entirely: `OnCapsuleBeginOverlap` fires independently on every machine, so each client calls `ExecuteGameplayCue` locally with no prediction key, no multicast, no skip.

**Fix:** Move all logic into `Fire()` and drop `OnFireTargetDataReceived` entirely (same pattern as `GeoRecallTurretAbility`). Because `Super::Fire()` is never called, no prediction key is ever established. The `ExecuteGameplayCue` inside `Mine->Recall()` multicasts with an empty key — empty keys are never treated as local-client keys, so the multicast is not skipped on any machine.

```cpp
void UMyAbility::Fire(FGeoAbilityTargetData const& AbilityTargetData)
{
    // ... resolve Pawn and DeployableManager ...
    for (AGeoMine* Mine : ...)
        Mine->Recall(true, Multiplier);
    EndAbility(true, false);
    // No Super::Fire() — no prediction key, no multicast skip
}
```

Note: `ExecuteRecallCue()` is a public method on `AGeoDeployableBase` that fires only the Gameplay Cue (no game logic, safe to call on any machine) — useful when you need the cue without the full `Recall` side effects.

## Gameplay Cue not showing when fired at the same frame on the same ASC — 30/04/2026

Only 2 Gameplay Cues can be multicasted over the network each frame for a single ASC. If you try to detonate mines with the GC on the SourceASC, then only 2 will be shown, always. even if client plays it.

# GAS — Ability Lifecycle

## `Spec.IsActive()` is a replicated value, not local instance state — 19/06/2026

Held ability (Moira beam) stuck "in use" / slot stuck grayed on death. `FGameplayAbilitySpec::IsActive()` is `ActiveCount > 0`, a server-replicated counter — NOT the local instance's `bIsActive`. `CancelAllAbilities()` gates on `if (Spec.IsActive())`, so when the server's cancel zeroes `ActiveCount` before the client's `OnRep_IsDead -> Death() -> CancelAllAbilities()` runs, the client skips the spec and its local predicted instance keeps ticking.

**Rule:** to tear down your own instance, drive off `Instance->IsActive()` over `Spec.GetAbilityInstances()`, never `Spec.IsActive()`. Fix: `UGeoAbilitySystemComponent::EndActiveAbilitiesLocally()`, called from `Death()`.

# Misc

## Deployables null on client

**Root cause:** `AGeoDeployableBase::BeginPlay` calls `RegisterDeployable(this)` on both server and client — this is intentional, the client needs its own local copy for `GetDeployRatio()` and `OnDeployCountChanged`. However `Deployables` is also `UPROPERTY(Replicated)`, so when the server rep update arrives it overwrites the client's correctly-populated local array. UE resolves actor pointers at rep-apply time; if the actor isn't ready yet the slot is null and never fixed up.

**Fix:** Remove `Replicated` from `Deployables` entirely. The client already builds its own correct copy via `BeginPlay` — replicating the array is redundant and actively harmful.

**Why the OnRep trick works temporarily:** Switching to `ReplicatedUsing` adds a callback that sweeps the array after each rep update, patching up pointers that have since become valid. It masks the symptom but the race is still there.


## Lifebar not visible on the enemies for the host only in ListenServer

IsLocallyControlled() is true for host AI too, so we need to use also IsPlayerControlled 
ie SetCombattantWidgetVisible(!(IsPlayerControlled() && IsLocallyControlled()));

## Tab / C dead after the pause menu, packaged build only (2026-10-15)

**Symptom:** after time in the pause menu (gems), Tab (character sheet) and C (stats detail) never fire again; everything else works. Never in editor.

**Root cause:** `AGeoPlayerController` held its mapping contexts and those actions as `TSoftObjectPtr`. Opening the menu removes the gameplay context, the only hard reference to it and to Tab / C's actions → GC unloads them (build only; the editor keeps assets loaded). Closing reloads new copies, but `UEnhancedInputComponent` bindings hold the old action by `TWeakObjectPtr` → dead binding. Pawn actions survive: hard `TObjectPtr`.

**Fix:** input assets the controller binds or swaps are `TObjectPtr`. Any input action or context bound in code and not always held by an active context needs a hard reference.

---

# AI — StateTree

## Failing Enter Condition falls back to Root, not to a sibling — 23/07/2026

A direct `GotoState → SweepBeam` whose enter condition fails bubbles to Root (stuck at Dormant waiting for the init event). The state's own transitions never run (state never entered), and adding a sibling doesn't help — sibling fallthrough only happens when a transition targets the **parent**.

**Fix:** Condition the transition *into* the state, not the state. On the source: `→ SweepBeam` if `CycleCount >= 3`, else `→ MoveBackToChase`; remove SweepBeam's enter condition.

---

# Rewind debugger is not working for listen server in PIE

---

# Code pitfalls (moved out of the CLAUDE.md files, 2026-10-12)

The folder `CLAUDE.md`s state the way to do things; the failures behind those rules live here.

## Abilities / GAS
- **Deploy charges refilled by hand**: re-arming off the cooldown tag's removed edge broke on clients (the server re-arm inside the removal callback coalesced remove+add into one replicated delta, so the client tag never reached 0; a client cannot re-arm either, no scoped prediction key, `CommitAbilityCooldown` fails `HasAuthorityOrPredictionKey`). Use the Cooldown GE's own stack expiry. `ShouldIgnoreCooldowns` instead of `CheckCooldown() = true` would stop the GE applying.
- **Dash with `LaunchCharacter` or a hand timer**: CMC correction artifacts; a client-only wall-clock timer fired early, forced `MOVE_Walking` mid-dash and cut it short. The root-motion task owns the timeout.
- **Passive that is not ServerOnly**: a client `CancelAllAbilities()` on revive kills the server's freshly reactivated passive.
- **Core passive cancellable on the owning client** (Relay, 2026-10-14): on revive the server re-grants the Core and its activation RPC activates the client copy; the client's own `ReviveLogic` (`OnRep_IsDead`, another actor channel, either order) then ran `CancelAllAbilities` and ended that copy. Nothing re-activates a ServerInitiated ability on a client, so the client dashed unsnapped while the server snapped (correction). Fixed by `UGeoCorePassiveAbility::CanBeCanceled` = false.
- **Slicing a derived `FPatternData`**: `Make(Derived)` without the template arg resolves to the base type, `GetPtr<Derived>()` then returns null everywhere (looks like "InstancedStruct arrives empty").
- **`FMath::Rand*` in abilities**: client and server diverge; use `FRandomStream(StoredPayload.Seed)`. `Seed % Num` goes negative, use `RandHelper`.
- **`GetAvatarActor()` in abilities**: payload is client-set and may differ from ActorInfo; use `StoredPayload`.
- **`Fire()` as a no-op**: it sends the RPC that triggers `OnFireTargetDataReceived`.
- **Pattern tick kicked from `StartPattern`**: `FTimerManager::IsTimerActive` is true while a timer's own callback runs, so the tick is routed to `TickDuringInit`.
- **Spiral pooled re-entrancy**: `SetActorLocation` resolves overlaps synchronously; a projectile can die, return to the pool and null its `Projectiles[i]` mid-iteration. Without the post-move re-check, `GetActorState()` (false for dead and for not-yet-started) resurrects a pooled projectile: released-twice ensure, pattern never ends.
- **ConeSpray salve stamped with tick time**: advanced every projectile by `StartDelay`, per-machine non-deterministic.
- **Sacrifice pair without `bActivateOnFreshPressOnly`**: both abilities chain-activate while the button is held.
- **Tile bomb detached in `RecallEffect`**: it blows up wherever the carrier ends up instead of planting.
- **Gameplay cue fired from `OnFireTargetDataReceived`**: the server multicast carries the scoped prediction key K; the owning client sees `IsLocalClientKey()` and skips it. See the 29/04/2026 entry above.

## Actors / projectiles
- **`bIsEnding` not set in an `EndProjectileLife` override**: a projectile overlapping two targets in one frame releases to the pool twice; two spawns share it and it teleports / recolours mid-flight.
- **`EditCondition` on inherited struct values**: the engine resolves the operand only against the declaring struct (`FProperty::GetOwnerStruct`); member functions fail the same way (`FindUField<UFunction>`; UHT rejects `UFUNCTION` in a `USTRUCT`). Unresolved evaluates to **true** with one `LogEditCondition: Error`. Hence `FExternalProjectileParamsCustomization`.
- **`CallInEditor` button on a class edited in Class Defaults**: `AddCallInEditorMethods` strips `RF_ArchetypeObject`, so the click silently calls nothing; the CDO's `BulletVFX` is an unregistered template. Preview goes through `GetArchetypeInstances()`.
- **Niagara User params on an inherited component** never populate; keep colours and radius C++-driven. A hidden pooled Niagara keeps simulating (non-solo systems are ticked by the world manager): `DeactivateImmediate` in `End()`, `Activate(true)` after `ApplyProjectileParams` in `Init()`.
- **Spawn-site param overrides**: pooled projectiles are not replicated, so every machine applies them; on a replicated projectile the override is authority-only (`BeginPlay` applies `DefaultParams` on proxies, after the initial replication and its RepNotifies).
- **Shield burst radius**: the spawn-site override is never replicated; without seeding `BounceSnapshot` at spawn and re-applying it in `InitProjectileLife` it renders at Blueprint size until the first bounce.
- **Multicast recall / expiry cues**: one cue per client comes from `OnRep_Active`; a multicast doubles it.
- **Deployables array replicated**: the server update overwrote the client's locally built array with nulls (see "Deployables null on client").
- **Hex arena fall check**: a bare `IsOverAliveTile` kills players standing on a border; a fixed-length radial sample bulges at corners. `RemoveInstance` on dead tiles breaks index stability (zero-scale instead).
- **Hex barrier**: a `SetTimerForNextTick` lerp chain had a non-terminating condition and stacked parallel lerps; drive from `Tick`, enabled in `OnRep_bIsClosed`. Re-running the construction script before `CaptureLayout` loses sculpted deletions.
- **Teleporter**: begin-overlap can fire more than once per arrival (hence `PendingArrivals`); an active root-motion source (dash) recomputes velocity toward its pre-teleport target and yanks the character back (clear `CurrentRootMotion`).
- **Arena victory**: `EndFight` is reached on wipe and victory; without `bBossDefeated` the fresh boss instantly re-aggros players looting the corpse.
- **Match stand-down**: `AGameMode::StartMatch()` early-outs while `HasMatchStarted()` (true in `WaitingPostMatch`), silently blocking every future `TriggerAggro`.
- **Immediate wipe respawn**: the hex arena's fall-death teleport must land before `RespawnGroup`; use the timer.

## Characters / game classes
- **`RemoveActiveEffects` client-side**: locally predicted effects (dash cooldown) linger after death / respawn; keep it server-guarded inside Death / Revive logic.
- **Raw slot-0 material set** discards the MID; go through `SetBodyMaterial`.
- **Facing**: `AAIController::UpdateControlRotation` snaps `ControlRotation` to the focal point each frame (bypassing the turn clamp), and `bSetControlRotationFromPawnOrientation` fights `AGeoCharacter::Tick`; face only via `SetTargetYaw`.
- **Local setup in `BeginPlay`** (PlayerController): a couch-coop joiner's controller begins play before `SetPlayer`, so `GetLocalPlayer()` is null. Use `ReceivedPlayer`.
- **Couch coop**: the Windows default `DeviceMappingPolicy=1` welds the first gamepad to player 1; the policy is read once into a static (config only). The engine never falls back to player 0 for unmapped devices. Each platform user has its own Slate focus: a pad with no focused widget drives UI instead of the game; `NotifyPlayerAdded` is too early (viewport widget registered later). PIE: "Route gamepad to 2nd window" must be off. `IPlatformInputDeviceMapper` needs an explicit `ApplicationCore` dependency.
- **`IsLocallyControlled()` for the host's AI** is true; use `IsPlayerControlled() && IsLocallyControlled()`. `!IsServer()` skips the listen-server host, which renders; use `IsDedicatedServer`.
- **`GeoLib::GetServerTime` for local client timing**: use `GetWorld()->GetTimeSeconds()`.
- **A mouse nudge steals both couch players' aim** when gated on `IsLocalPlayerAvatar`; gate on `IsKeyboardMousePlayer`.

## Systems / AI / UI
- **Pool**: duplicate entries make two `RequestActor` calls pop the same actor; a `Pool.FindOrAdd` reference is invalidated by a rehash if `SpawnActor` inserted.
- **Combat stats**: pushing zeros when a match ends wipes the displayed final figures; sliding windows let sustained fire pile into one value (tumbling instead).
- **`GeoAITask_MoveTo`**: `Super::PerformMove()` may destroy the task synchronously (`Path` nulled), so post-call `Path` blocks no-op; `UAITask_MoveTo::ResetTimers()` clears every timer for the object on replan (use `TickTask`); `AddGameplayCue` never dedups; `ExecuteGameplayCue` carries no `RawMagnitude`.
- **World widgets on the rotating root**: offsets orbit as the actor yaws; attach to `WidgetAnchorComponent`.
- **Overlay owned by the world**: slot widgets read player 1's key bindings for every couch-coop player; own it by the PlayerController.
- **Overlay layout in Blueprint**: the player index is only known at runtime; `ApplyColumnLayout` undoes the previous column first or a third player compounds.
- **Floating numbers hidden server-side** (`bSuppressDrainDamageVisuals`, `bSuppressGameplayCue`): those never reach the client's number spawner; use `bShowDamageNumbers`.
- **Damage number off-screen**: culled and never ticks; return it to the pool by world timer.

## Colour / Niagara
- **`Override` not last in `EGeoColor`**: its ordinal is `SlotCount` and earlier values are live stencil indices. Enum properties serialise by name, so inserting other slots anywhere is safe for saved assets.
- **Config maps** take the single-line `ImportText` path; only arrays and sets use repeated `+Prop=` lines.
- **Niagara param names typed at a call site**: nothing validates them; a typo silently leaves the authored default.
- **Reassigning a Niagara asset every call** resets the system; compare first (`ApplySwappableAsset`).
- **Palette texture** exists only from `BeginPlay`: the outline shows its default in the editor viewport.

## Editor module
- **`UEditorUtilityObject` under `#if WITH_EDITOR` in the runtime module**: UHT still emits class registration and the packaged runtime breaks. Keep them in `GeoTrinityEditor`.
- **Widget tree builders**: `WidgetTree` is protected; commit once per batch (`CommitTree` reconciles the GUID map, self-healing).
- **Customization not registered** in `StartupModule` never runs.

---

# GAS / replication post-mortems (moved from agent memory, 2026-10-12)

## Predicted GE removal leaks tags — cooldown soft-lock
Granted tags reach clients through `FGameplayTagCountContainer`, whose delta is absolute and state-based. A client that predicts a GE raises the tag locally and never lowers it on removal unless the prediction was explicitly rejected (`RemoveActiveGameplayEffectGrantedTagsAndModifiers` gates the `-1` on `IsNetAuthority() || bPredictionRejected`). If the server applies **and** removes the GE inside one replication window its count goes 0 → 1 → 0, no delta is sent, and the client keeps the tag forever: `CheckCooldown` fails locally, nothing reaches the server, the ability soft-locks.
- Seen: `Ability.Cooldown.Dash` stuck on clients when a player died in the same server frame as a dash and `ResetAbilitiesAndEffects()` wiped every effect.
- Cooldowns are the only predicted effects, so any server-only wipe of a player ASC reintroduces it.
- **Rule**: `UGeoAbilitySystemComponent::ResetCooldowns()` from a replicated-state path (see its CLAUDE.md).

## Deployable ASC is Minimal but net-owned by the deployer
`AGeoInteractableActor` sets `EGameplayEffectReplicationMode::Minimal`; deployables are spawned owned by the deploying player (`StartSpawnDeployable`). Epic documents Minimal as unsupported for owned ASCs. `MinimalReplicationGameplayCues` and `MinimalReplicationTags` are `COND_SkipOwner`, so the deploying client gets neither.
- Symptoms on the deployer's client only: `WhileActive` cue never fires (only `OnActive` / `OnRemove` RPCs arrive), `OnRemove` finds nothing to clean; tag checks such as `Status.Sacrificed` on your own wall read false.
- Signature: fine on the listen server and on other players' deployables.
- **Fix**: `Mixed` on that ASC; until then, spawn from `OnActive` and `WhileActive` with an idempotent guard.

## Soft-referenced global assets are collected in packages only
`GARBAGE_COLLECTION_KEEPFLAGS` is `GIsEditor ? RF_Standalone : RF_NoFlags`: `RF_Standalone` protects loaded assets only in the editor. A `TSoftObjectPtr` stores a path and keeps nothing alive; `gc.TimeBetweenPurgingPendingKillObjects=61.1` collects roughly once a minute.
- Seen: `UGameDataSettings::AbilityInfo` (the only hard refs to every ability class) collected mid-session; a CDO cached in a non-`UPROPERTY` `TMap` became a use-after-free → client crash `Array index out of bounds ... inside UGameplayAbility::GetAssetTags()`. Reproduces in a package only; log shows a climbing `FlushAsyncLoading(N): 1 QueuedPackages`.
- **Rules**: anything cached past a frame is reachable by GC (`UPROPERTY`; holder = file-scope `USTRUCT`); global data assets go through `GetLoadedDataAsset()` (`AddToRoot()`). Direct `LoadSynchronous()` sites (`EffectData.cpp`, `GeoCharacter.cpp`, `GeoGameplayAbility.cpp`) are still unrooted and churn.

## A lethal hit ends the match inside the killing blow
Server attribute delegates broadcast synchronously from `SetNumericAttributeBase`: a lethal `SetHealth` in `PostGameplayEffectExecute` runs `AEnemyCharacter::OnHealthChanged` → `OnEnemyDefeated` → match stand-down → every `OnMatchStateChanged` handler *before* the lines after `SetHealth` (`OnDamageDealt`, combat-stats report). Direct-Health GEs change Health before `PostGameplayEffectExecute` is even called, so reordering inside the attribute set never fixes it.
- Seen: the boss's killing blow reached `UGeoCombatStatsSubsystem` after its fight ended, opened an out-of-match session and wiped the meter to that one hit.
- **Rule**: defer teardown with `SetTimerForNextTick`.

# Data / redirects: silent loss

## Enum rename with `ValueChanges` orphans every stored value
`UEnum::GetIndexByNameString` regenerates the renamed enum's namespace only in the branch where `FCoreRedirects::GetValueRedirects` returns null. Any `ValueChanges` on the entry skips the fixup and every stored value falls to `_MAX`: a `TMap` keyed on the enum loads its values but no lookup matches ("all data gone", data intact on disk).
- **Rule**: rename with `(OldName=..., NewName=...)` alone; value renames in a separate pass. Diagnose: `[regex]::Matches($text, "EOldEnum::[A-Za-z]+")` over the `.uasset` bytes. USTRUCT-as-map-value renames and property renames are fine.

## CoreRedirects resolve one hop
`A -> B`, `B -> C` leaves an asset still storing `A` one hop short. For a `TMap` whose *value* is the redirected struct the element type mismatches and the engine discards the **whole map** on load (`LogClass: Warning: ... struct type mismatch`, `Map Element Type mismatch`); the next save writes it empty.
- **Rule**: one entry per old name straight to the final name. After any struct rename `grep -ra "OldStructName" Content/` for the names actually on disk, and check the editor log for `type mismatch` before re-saving any affected asset.
