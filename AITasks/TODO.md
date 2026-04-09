# TODO

Tasks marked `[x]` without a recurrence tag are permanently done.
Tasks tagged `[recur:daily]` are reset to `[ ]` each day by this automated agent.

## Tasks

- [ ] [recur:daily] For every function modified today and every public function that has no comment or a malformed comment: add or fix its Unreal-style JavaDoc header. Rules below.

  **Scope**
  - Run `git diff --name-only HEAD` to get today's modified files. Only open and process `.h` and `.cpp` files from that list — do not scan the whole codebase.
  - Within those files: add comments to public functions that have none, fix comments that are malformed or contradict the current implementation, and update any comment whose intent changed today.
  - Do not touch headers that are already correct.
  - Only comment where the method is publicly declared — not in the implementation.

  **Comment format**

  Simple functions — inline description only (no parameter block needed):
  ```cpp
  /** Checks whether the deployable has expired. */
  bool IsExpired() const;
  ```

  Complex functions — multi-line block:
  ```cpp
  /**
   * Steeps a cup of tea.
   * Does not apply milk. Caller is responsible for milk application.
   *
   * @param Vessel        The cup to steep into. Must not be null.
   * @param Duration      Steep time in seconds. Range: [30, 300].
   * @param Strength      Target strength (0 = weak, 1 = strong).
   * @return              True if the tea was successfully steeped.
   * @warning             Vessel temperature must be above 90°C before calling.
   */
  bool SteepTea(UCup* Vessel, float Duration, float Strength);
  ```

  **Rules**
  - Comments document **intent**, not implementation. Code documents implementation.
  - Parameter block required when parameters have non-obvious units, ranges, or constraints.
  - `@return` only when the return value is not already fully described by the function purpose.
  - `@warning`, `@note`, `@see`, `@deprecated` each on their own line after the main block.
  - Class comments must state: what problem the class solves, and why it was created.

  **Report**
  After completing the task, optionally append a short timestamped comment (usually 1–5 lines, it can be longer if needed.) directly below the task bullet. Flag anything worth surfacing: incoherent naming, a function whose comment contradicts its implementation, suspicious patterns, or anything that looks like a latent bug. Skip the report if nothing notable was found.
  Format: `<!-- [YYYY-MM-DD] <findings> -->`

- [x] Blueprint exposure audit — add BlueprintReadOnly/BlueprintPure/BlueprintNativeEvent specifiers across all headers. No behavior changes except OnHealthChanged and OnDeployableExpired become BlueprintNativeEvent (requires _Implementation rename in .cpp). Full change list below.

### Blueprint Exposure — Change List

**`GeoGameplayAbility.h`**
- `AnimMontage` → add `BlueprintReadOnly`
- `EffectDataAssets` → add `BlueprintReadOnly`
- `EffectDataInstances` → add `BlueprintReadOnly`

**`GeoMoiraBeamAbility.h`**
- `DamageEffect`, `HealEffect`, `SpeedBuffEffect` → add `BlueprintReadOnly`
- `BeamLength`, `InitialDuration`, `DurationPerAbsorbedZone`, `RadiusGrowthPerZone`, `BeamZoneDrainPercentagePerSecond` → add `BlueprintReadOnly`
- `RemainingDuration` → add `UPROPERTY(BlueprintReadOnly)`
- `AccumulatedRadiusBonus` → add `UPROPERTY(BlueprintReadOnly)`

**`GeoHealingAuraAbility.h`**
- `HealPerSecond` → add `BlueprintReadOnly`

**`GeoRecallTurretAbility.h`**
- `BlinkBonusEffect`, `RecallGameplayCueTag`, `OverlapAttitude` → add `BlueprintReadOnly`

**`GeoReloadAbility.h`**
- `AmmoRestoreEffect`, `BuffPickupClass`, `MinSpawnRadius`, `MaxSpawnRadius` → add `BlueprintReadOnly`

**`GeoDeployAbility.h`**
- `MinDeployDistance`, `MaxDeployDistance`, `Params`, `DeployableActorClass` → add `BlueprintReadOnly`

**`GeoProjectile.h`** (private members need `meta = (AllowPrivateAccess = true)`)
- `LifeSpanInSec`, `DistanceSpan`, `OverlapAttitude` → add `BlueprintReadOnly`
- `ImpactEffect`, `ImpactSound`, `LoopingSound` → add `BlueprintReadOnly`
- `Sphere` → add `BlueprintReadOnly`
- `EffectDataArray` → add `UPROPERTY(BlueprintReadOnly)`
- `Payload` → add `UPROPERTY(BlueprintReadOnly)`
- `SetDistanceSpan` → add `UFUNCTION(BlueprintCallable)`

**`GeoTurret.h`**
- `TurretProjectileClass`, `FireInterval` → add `BlueprintReadOnly`

**`GeoBuffPickup.h`** (private members need `meta = (AllowPrivateAccess = true)`)
- `BuffMeshAssets`, `RotationSpeed`, `LaunchCurve`, `MinScale`, `MaxScale`, `OverlapAttitude` → add `BlueprintReadOnly`

**`GeoDeployableBase.h`**
- `FDeployableDataParams::BlinkDuration`, `LifeDrainMaxDuration`, `Size` → add `BlueprintReadOnly`
- `bUseRegularDrain` → add `UPROPERTY(BlueprintReadOnly)` (bare bool → needs UPROPERTY)
- `DrainMagnitudePerSecond` → add `UPROPERTY(BlueprintReadOnly)` (bare float → needs UPROPERTY)
- `GetDurationPercent()` → add `UFUNCTION(BlueprintPure)`
- `IsExpired()` → add `UFUNCTION(BlueprintPure)`
- `IsBlinking()` → add `UFUNCTION(BlueprintPure)`
- `OnDeployableExpired()` → change to `UFUNCTION(BlueprintNativeEvent)`; rename `.cpp` body to `OnDeployableExpired_Implementation()`

**`GeoInteractableActor.h` + `.cpp` + subclasses**
- `OnHealthChanged` → add `BlueprintNativeEvent`; rename to `_Implementation` in: `GeoInteractableActor.cpp`, `GeoDeployableBase.cpp`, `EnemyCharacter.cpp`
- `OnMaxHealthChanged` → same as above

**`Pattern.h`**
- `AnimMontage` → add `BlueprintReadOnly`
- `OnPatternEnd` → add `UPROPERTY(BlueprintAssignable)`

**`GeoDeployableManagerComponent.h`**
- `MaxDeployables` → add `BlueprintReadOnly`
- `CanDeploy()`, `GetDeployedCount()`, `GetMaxDeployables()`, `GetDeployRatio()` → add `UFUNCTION(BlueprintPure)`

**`PlayableCharacter.h`**
- `GaugeChargingSpeedCurve`, `ClassData` → add `BlueprintReadOnly`

**`EnemyCharacter.h`**
- `StateTree`, `ResetToFullLifeWhenReachingZero` → add `BlueprintReadOnly`

**Verification**: Build must compile cleanly. Open BP_HealingZone, BP_Turret, BP_BuffPickup, a Circle ability BP, BP_PlayableCharacter — confirm exposed vars appear in Details and are accessible in BP graphs. Confirm OnDeployableExpired and OnHealthChanged are overridable in BP subclasses.
