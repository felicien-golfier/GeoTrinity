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

---