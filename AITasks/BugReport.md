# Bug Report — GeoTrinity Codebase Audit
**Date:** 2026-04-15  
**Files read:** all `.h` and `.cpp` under `Source/GeoTrinity/Public/` and `Source/GeoTrinity/Private/`

---

## [x] BUG-01 — Crash: `OnFireTargetDataReceived` dereferences TargetData after `ensureMsgf` without early return
**File:** `Private/AbilitySystem/Abilities/GeoGameplayAbility.cpp:332-337`  
**Severity:** High — server crash

```cpp
FGeoAbilityTargetData const* AbilityTargetData = static_cast<FGeoAbilityTargetData const*>(DataHandle.Get(0));
ensureMsgf(AbilityTargetData, TEXT("No FGeoAbilityTargetData found ..."));
StoredPayload.Seed = AbilityTargetData->Seed;   // crashes if null
```

`ensureMsgf` does not halt execution. If `DataHandle.Get(0)` returns null (e.g. the client sent no data, type mismatch, packet dropped before ability ends), the next line crashes. Per CLAUDE.md the ensure must be followed by an `if (!AbilityTargetData) return;`.

**Fix:** add `if (!AbilityTargetData) return;` immediately after the ensure.

---

## [x] BUG-02 — Crash: `GeoAutomaticFireAbility::OnFireTargetDataReceived` has no null guard on TargetData
**File:** `Private/AbilitySystem/Abilities/Damaging/GeoAutomaticFireAbility.cpp:161-166`  
**Severity:** High — server crash

```cpp
FGeoAbilityTargetData const* TargetData = static_cast<FGeoAbilityTargetData const*>(DataHandle.Get(0));

StoredPayload.Origin = TargetData->Origin;   // no null check, no ensure
StoredPayload.Yaw    = TargetData->Yaw;
```

Unlike the base class (which at least has an `ensureMsgf`), this override has neither an assert nor a guard. Any case where `DataHandle.Get(0)` returns null crashes the server immediately.

**Fix:** add `ensureMsgf(TargetData, ...) + if (!TargetData) return;` before the dereference.

---

## [x] BUG-03 — Crash: `GeoProjectileAbility::OnFireTargetDataReceived` re-reads TargetData without a null check
**File:** `Private/AbilitySystem/Abilities/Damaging/GeoProjectileAbility.cpp:45-49`  
**Severity:** High — server crash

```cpp
Super::OnFireTargetDataReceived(DataHandle, ApplicationTag);   // reads DataHandle safely

FGeoAbilityTargetData const* AbilityTargetData =
    static_cast<FGeoAbilityTargetData const*>(DataHandle.Get(0));   // second read, no guard

SpawnProjectilesUsingTarget(AbilityTargetData->Yaw, ...);   // crashes if null
```

The super call already extracted what it needs, but this override performs a separate `Get(0)` and immediately dereferences without checking.

**Fix:** `ensureMsgf(AbilityTargetData, ...) + if (!AbilityTargetData) return;`

---

## [x] BUG-04 — Logic Error: `GetDebuffDuration` returns debuff **damage**, not duration (copy-paste)
**File:** `Private/AbilitySystem/Lib/GeoAbilitySystemLibrary.cpp:351-354`  
**Severity:** Medium — silent wrong value

```cpp
float UGeoAbilitySystemLibrary::GetDebuffDuration(FGameplayEffectContextHandle const& effectContextHandle)
{
    ...
    return pGeoContext ? pGeoContext->GetDebuffDamage() : 0.f;   // should be GetDebuffDuration()
}
```

Every caller of `GetDebuffDuration` receives the damage value instead of the duration. Debuff timing will be completely wrong anywhere this is used.

**Fix:** change `GetDebuffDamage()` to `GetDebuffDuration()`.

---

## [x] BUG-05 — Logic Error: `ensureMsgf(true, ...)` never fires — condition is inverted
**File:** `Private/AbilitySystem/Lib/GeoAbilitySystemLibrary.cpp:563`  
**Severity:** Medium — silent failure, missing error signal

```cpp
// Inside GetEffectDataArray(FGameplayTag) — reached when no ability was found:
ensureMsgf(true, TEXT("No Ability found for AbilityTag %s"), *AbilityTag.ToString());
```

`ensureMsgf(true, ...)` is a no-op — it always succeeds. The intended signal (no ability found for the tag) is never surfaced, silently returning an empty array.

**Fix:** `ensureMsgf(false, TEXT("No Ability found for AbilityTag %s"), *AbilityTag.ToString());`

---

## [x] BUG-06 — Logic Error: Same `ensureMsgf(true, ...)` in `GetAndCheckSection`
**File:** `Private/AbilitySystem/Lib/GeoAbilitySystemLibrary.cpp:762`  
**Severity:** Medium — missing error signal

```cpp
if (SectionIndex == INDEX_NONE)
{
    ensureMsgf(true, TEXT("Section %s not found in AnimMontage %s"), ...);   // never fires
    UE_LOG(LogPattern, Error, TEXT("Section %s not found ..."));
}
```

The `UE_LOG` fires correctly, but the ensure (which would break the debugger and generate a report) never triggers. Same pattern as BUG-05.

**Fix:** `ensureMsgf(false, ...)`.

---

## [x] BUG-07 — Crash: `FullySpawnProjectile` passes null to `FinishSpawnProjectile` when spawn fails
**File:** `Private/AbilitySystem/Lib/GeoAbilitySystemLibrary.cpp:573-578`  
**Severity:** Medium — crash when spawn fails

```cpp
AGeoProjectile* Projectile = StartSpawnProjectile(...);   // can return nullptr
FinishSpawnProjectile(World, Projectile, ...);            // Projectile->GetClass() crashes on null
```

`StartSpawnProjectile` has two early-return paths that return `nullptr` (invalid World/class, or pool returning null). `FinishSpawnProjectile` immediately calls `Projectile->GetClass()` with no null check.

**Fix:** guard before the call: `if (!Projectile) return nullptr;`

---

## [x] BUG-08 — Wrong Instigator on poolable projectiles
**File:** `Private/AbilitySystem/Lib/GeoAbilitySystemLibrary.cpp:598`  
**Severity:** Low — Actor's `GetInstigator()` returns wrong pawn for pooled projectiles

```cpp
// Poolable path:
Projectile = UGeoActorPoolingSubsystem::Get(World)->RequestActor(
    ProjectileClass, SpawnTransform, Payload.Owner,
    Cast<APawn>(Payload.Owner),        // <-- should be Payload.Instigator
    false);

// Non-poolable path (correct):
Projectile = World->SpawnActorDeferred<AGeoProjectile>(
    ProjectileClass, SpawnTransform, Payload.Owner,
    Cast<APawn>(Payload.Instigator),   // correct
    ...);
```

For pooled projectiles, `Actor->GetInstigator()` returns the owner cast to APawn instead of the actual instigator. Current game code reads `Payload.Instigator` directly (not `GetInstigator()`), so most logic is unaffected — but anything that calls `GetInstigator()` on a pooled projectile gets a wrong actor.

**Fix:** `Cast<APawn>(Payload.Instigator)` in the poolable `RequestActor` call.
