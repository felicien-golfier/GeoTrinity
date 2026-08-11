# Simplification Report

Audit of the GeoTrinity source tree for places the code can be made smaller / simpler, folder by folder.
Every entry is **What** (the observation) / **Why** (why it is worth changing) / **What to change** (the concrete edit).

Rules referenced come from `AI/CodingStyle.md`.

**16 sections, 95 findings.** Three cross-cutting themes and a suggested order of work are at the end — start
there.

---

## Status — what is left (2026-08-11)

Tiers 0a–13 of the order-of-work table are **applied**. The bug-fix tiers (0a–0c) landed in commit `4af2ae7`
"Fix all audits."; every simplification tier landed after it. Per-finding status lives in the table at the end of
this file; this section is the short version of what is still open.

**Not applied — remaining work, in the order it should be done:**

| # | Item | Why it is still open |
|---|---|---|
| 12 | **8.4 + 8.5** — collapse the two attribute-change fan-out mechanisms | Real work: deletes `UGenericCombattantWidget::UnbindStatCallbacks` and two ASC delegates. Touches `GeoHUD`, `GenericCombattantWidget`, `GeoCombattantWidgetComp`. 8.7 (weak-lambda unbind policy) is decided as part of it |
| 14 | **2.1 + 2.2 + 2.3** — `FEffectData` hierarchy collapse | Largest structural win, and the only item that **cannot be verified by a compile**: it renames serialized `UPROPERTY` names, so it needs `[CoreRedirects]` in `DefaultEngine.ini` plus a PIE check on a damage ability, or designer-authored values silently reset to defaults |
| 15 | **2.5** — `AbilityDescriptions.txt` hand-rolled INI section handling | Editor-only path; still parses with `ParseIntoArrayLines` + `StartsWith(TEXT("["))` in two places |
| 15 | **12.1** — `AGeoGameCamera::Tick` is ~137 lines doing five jobs | `GeoGameCamera.cpp` has uncommitted local edits from other work; do it on a clean file so the diff is readable |
| — | **13.7** — three hand-written recursive walkers over the StateTree | `FindStateRecursive` / `RemoveStateRecursive` / `LogStatesRecursive` are still three separate traversals |
| — | **1.9 (second half)** — drop `GeoDeployableBase.h` / `GeoAbilitySystemComponent.h` from the module-wide library header | Deliberately deferred: worth its own commit where the compiler drives which translation units need the include added back |
| — | **1.10** — `ApplyStatusToTarget` / `GetStatusTag` naming drift | Renames public API for consistency only; no behaviour, low value, do it when that header is touched anyway |
| — | **9.4** — `GetFightingArena` and `RespawnAllBosses` are the same traversal twice | Judged low value on re-reading: 8 lines each, and the shared part is only "resolve world, iterate arenas" |

**Needs a runtime check, not a compiler** — applied but unverified in PIE:
- **11.1** pooled-actor hand-out (exhaust the pre-spawned pool)
- **3.2** projectile impact FX on the `Destroyed()` path
- **3.6** unified deployable recall cosmetics
- **3.7** `IsBlinking()` now reads `bBlinking` instead of the timer handle — check a turret recall after the blink expires
- **7.2** / **12.2** — re-read before closing them out; the audit could not confirm from the current source whether the double montage-section validation and the unguarded `LocalCharacter` deref are gone

**Doc drift introduced by this work** (left for the daily agent, per the CLAUDE.md rule that interactive sessions
do not edit subfolder docs): `Source/GeoTrinityUI/Public/HUD/CLAUDE.md` still lists
`IGeoDeployGaugeWidgetInterface`, which finding 4.1 deleted — the deploy gauge now implements the shared
`IGeoChargeGaugeWidgetInterface`.

---

## 1. `AbilitySystem/Lib` — `GeoAbilitySystemLibrary`

### 1.1 `GetAbilityInfo` / `GetStatusInfo` — the `WorldContextObject` parameter is dead weight

- **What**: `GetAbilityInfo(UObject const* WorldContextObject)` and `GetStatusInfo(UObject const*)`
  (`GeoAbilitySystemLibrary.cpp:31-60`) never use the world context. They null-check it, then read
  `GetDefault<UGameDataSettings>()` — a process-wide CDO. A second overload `GetAbilityInfo()` exists with the
  identical two-line body.
- **Why**: Two functions with the same body, and a parameter that only exists to be null-checked. Call sites are
  already split roughly 50/50 between the two forms (`PlayableCharacter.cpp:154` and
  `GeoAbilitySystemComponent.cpp:141/168/204` pass `this`; `GeoHUD.cpp:318`, `GeoArena.cpp:292`,
  `GeoReloadAbility.cpp:158`, `GeoAbilitySystemComponent.cpp:29/65` do not) — proof the context is meaningless.
- **What to change**: Delete both `WorldContextObject` overloads. Keep `GetAbilityInfo()` / add `GetStatusInfo()`
  with no parameter, and mark them `UFUNCTION(BlueprintPure, Category="AbilitySystemLibrary|Info")` so Blueprint
  keeps its node. Update the 5 call sites that pass `this` to drop the argument.

### 1.2 `ApplySingleEffectData` duplicates the body of `ApplyEffectFromEffectData`

- **What**: `ApplySingleEffectData(FEffectData const&, ...)` (`:72-94`) re-implements the whole context pipeline
  already written in `ApplyEffectFromEffectData` (`:117-157`): `MakeEffectContext` → `FillEffectContext` →
  `static_cast<FGeoGameplayEffectContext*>` → `UpdateContextHandle` → `ApplyEffect`.
- **Why**: "No duplicated code — extract by concept". The two-pass rule documented in `Lib/CLAUDE.md` now lives in
  two places; a change to how the context is built (a new field, a different cast guard) has to be made twice, and
  the single-effect path already silently lost the `checkf` on `GeoEffectContext` that the array path has.
- **What to change**: Extract the shared middle into a private
  `static FGameplayEffectContextHandle MakeGeoEffectContext(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, FGeoGameplayEffectContext*& OutGeoContext)`
  and have both public entry points call it. The single-effect version then becomes ~4 lines.

### 1.3 The `IsValid` + `ensureMsgf` guard idiom is written three different ways in one file

- **What**: Three spellings of the same guard:
  - `:78-85` — `if (!IsValid(A) || !IsValid(B)) { ensureMsgf(IsValid(A) && IsValid(B), ...); return {}; }`
    (evaluates both checks twice)
  - `:122-134` — two separate `if (!ensureMsgf(IsValid(X), ...)) { return SpecHandles; }` blocks
  - `:495-505` — `if (!IsValid(X)) { ensureMsgf(X, ...); return nullptr; }` (twice, back to back)
- **Why**: `AI/CodingStyle.md` prescribes exactly one form: `if (!ensureMsgf(x, TEXT("..."))) { return; }`.
  The `:78` form evaluates `IsValid` four times and asserts a condition it has already proven false.
- **What to change**: Rewrite all of them as `if (!ensureMsgf(IsValid(X), TEXT("...")))`. Where two pointers are
  checked in a row and the message can be shared, merge into one condition with `&&` (style rule: "prefer fewer but
  longer `if` statements"). This removes ~15 lines across the file.

### 1.4 Two `GetInteractableActors` overloads are pure forwarders with hardcoded arguments

- **What**: `:370-376` forwards with `FVector2D::ZeroVector, 0.0f`; `:377-382` forwards with
  `FGenericTeamId(), 0`. Both are declared in the header (`:316-320`) and add nothing else.
- **Why**: Direct hit on "Never add a wrapper that just forwards with a hardcoded argument" and "Remove trivial
  wrapper functions that just delegate". The overload set is already 5 wide (2 templates + 3 free), which is the
  real cost — every call site has to work out which one it resolves to.
- **What to change**: Delete both. Give the main `UFUNCTION` overload default arguments
  (`FVector2D Location = FVector2D::ZeroVector, float MaxDistance = 0.f`) and let the teamless case pass
  `FGenericTeamId::NoTeam, TeamAttitudeMask::All` explicitly at its call site — that reads better anyway, since
  "no team, any attitude" is currently hidden behind `FGenericTeamId(), 0`.

### 1.5 `GetTeamInterface` is a bool-plus-out-param wrapper around a single `Cast`

- **What**: `:815-819` — `OutInterface = Cast<...>(Actor); return OutInterface != nullptr;`. It is public in the
  header but has only two call sites, both inside this same .cpp (`:763`, `:808`).
- **Why**: Trivial delegating wrapper, and the out-param form forces callers into a two-statement declare-then-call
  dance (`:762-767`, `:807-811`) where a single `if (IGenericTeamAgentInterface const* T = Cast<...>(Actor))` would
  do. It also duplicates what `Cast` already expresses.
- **What to change**: Delete the function and inline the `Cast` at both call sites. `GetTeamId` becomes 4 lines,
  `IsTeamAttitudeAligned` loses its out-param local.

### 1.6 `InitDeployable`, `FinishSpawnDeployable`, `GetEffectDataArray(UEffectDataAsset*)` are single-call wrappers

- **What**:
  - `InitDeployable` (`:555-562`) = `FillDeployableData` + `InitInteractable`; only caller is
    `FullySpawnDeployable` (`:485`).
  - `FinishSpawnDeployable` (`:525-534`) = one `ensureMsgf` + `FinishSpawning`; only caller is `:486`.
  - `GetEffectDataArray(UEffectDataAsset const*)` (`:452-461`) = ensure + return a public member; only caller is
    `GeoGameplayAbility.cpp:129`.
- **Why**: Three public API entries, each one call deep, each needing a doc comment in the header (`:166-180`,
  `:144-145`) and a mention in `Lib/CLAUDE.md`. That is a lot of surface for ~10 lines of behaviour.
- **What to change**: Inline `InitDeployable` and `FinishSpawnDeployable` into `FullySpawnDeployable` and delete
  them from the header (`FillDeployableData` stays — `CLAUDE.md` documents it as the pre-edit hook and it has real
  external callers). For `GetEffectDataArray(UEffectDataAsset*)`, move the `ensureMsgf` into
  `GeoGameplayAbility.cpp:129` and read `EffectDataAsset->EffectDataInstances` directly.

### 1.7 `StartSpawnProjectile` duplicates its failure handling in both branches

- **What**: `:607-653` — the pooled branch and the `SpawnActorDeferred` branch each end with an identical
  `if (!Projectile) { UE_LOG(...); return nullptr; }` block, differing only in the log text.
- **Why**: One post-condition written twice. The `AGeoProjectile* Projectile;` declared uninitialised above the
  branch is also a hazard the single-check form removes.
- **What to change**: Assign `AGeoProjectile* Projectile = nullptr;` in each branch, then do one null check +
  one `UE_LOG` after the `if/else`. The class name is already in the message, so a single text suffices.

### 1.8 `GetGameplayTagFromRootTagString`'s cache is more machinery than the thing it caches

- **What**: `:212-220` keeps a function-local `static TMap<FString, FGameplayTag>` to avoid
  `FGameplayTag::RequestGameplayTag`, with a comment justifying the optimisation. It also does a double lookup
  (`Find` then `operator[]`). Its only caller is `GetAbilityTagFromAbility` (`:236`), always with the same
  constant `RootTagNames::AbilitySpellTag`.
- **Why**: A mutable static map (not thread-safe, never cleared) to cache one constant lookup. "Don't add state to
  track what an existing construct already bounds" — a `static FGameplayTag const` local in the one caller bounds
  it exactly.
- **What to change**: Delete the function. In `GetAbilityTagFromAbility` use
  `static FGameplayTag const TagToMatch = FGameplayTag::RequestGameplayTag(FName(RootTagNames::AbilitySpellTag));`.
  If other callers appear later, promote it to a `FNativeGameplayTag` in `GeoGameplayTags.h` — which is where
  every other tag in the project already lives.

### 1.9 Header includes what it also forward-declares

- **What**: `GeoAbilitySystemLibrary.h:5-14` includes `GeoGameplayAbility.h`, `GeoAbilitySystemComponent.h`,
  `AbilityInfo.h` and `GeoDeployableBase.h`, then `:25-33` forward-declares `UAbilityInfo`, `UGameplayAbility`,
  `UAbilitySystemComponent`, `FGameplayTag`, `FGameplayEffectContextHandle` — types the includes already provide.
- **Why**: "Prefer forward declarations in headers over includes." This header is included by nearly every ability
  .cpp, so those four includes pull the deployable and ability hierarchies into most of the module's translation
  units. The redundant forward declarations are pure noise.
- **What to change**: Delete the forward declarations that the includes cover. Then drop
  `GeoDeployableBase.h` and `GeoAbilitySystemComponent.h` (only pointer/`TSubclassOf` use) in favour of
  `class AGeoDeployableBase;` / `class UGeoAbilitySystemComponent;`. `GeoGameplayAbility.h` and `AbilityInfo.h`
  must stay — the `GetAbilityCDO<T>` template body needs both.

### 1.10 `ApplyStatusToTarget` / `GetStatusTag` use a different naming convention from the rest of the file

- **What**: `:160-204` and `:424-440` use `pTargetASC`, `pSourceASC`, `statusTag`, `level`, `statusInfo`,
  `contextHandle`, `pGeoContext`, `effectContextHandle` — Hungarian prefixes and camelCase locals. Every other
  function in the file uses `SourceASC`, `AbilityLevel`, `ContextHandle`, `GeoEffectContext`.
- **Why**: "Be consistent: same code style, same naming convention throughout." This is the only file region that
  breaks it, so it reads as code from a different project.
- **What to change**: Rename to PascalCase without the `p` prefix, in both the .cpp and the header declarations
  (`GeoAbilitySystemLibrary.h:247-249`, `:365-369`). Pure rename, no behaviour change.

---

## 2. `AbilitySystem/Data` — `EffectData`, `AbilityInfo`

### 2.1 `FDamageEffectData` / `FHealEffectData` / `FShieldEffectData` are the same struct three times *(biggest win in this folder)*

- **What**: In `EffectData.cpp`, `FDamageEffectData::ApplyEffect` (`:106-131`), `FHealEffectData::ApplyEffect`
  (`:153-176`) and `FShieldEffectData::ApplyEffect` (`:178-203`) are line-for-line identical. They differ only in
  four values: the `UGameDataSettings` GE field, the SetByCaller tag, the `FScalableFloat` member, and the ensure
  message. `FLethalEffectData::ApplyEffect` (`:213-227`) is the same shape minus the SetByCaller line.
  The headers duplicate the same way: `bSuppressGameplayCue`, `bLimitGameplayCue`, `bSuppressCombatStats` are
  declared with identical comments in both `FDamageEffectData` (`EffectData.h:136-148`) and `FHealEffectData`
  (`:183-194`), and `bIsDamagePerSecond` / `bIsHealPerSecond` / `bIsShieldPerSecond` are one concept named three
  ways. `UpdateContextHandle` repeats the same three `if (bFlag) SetX(true);` blocks in both
  (`EffectData.cpp:75-86` vs `:139-150`).
- **Why**: This is ~120 lines expressing one behaviour. Every change to the apply pipeline (a new context flag, a
  different spec call) currently has to be made three or four times, and the per-second multiply
  (`* GetDeltaSeconds()`) is independently re-derived in each. It is also what forces the long `GetPtr<T>()`
  if/else chains elsewhere (see 2.2, 2.3).
- **What to change**: Insert an intermediate `FMagnitudeEffectData : FEffectData` holding `FScalableFloat Amount`,
  `bool bIsPerSecond`, `bool bSuppressGameplayCue`, `bool bLimitGameplayCue`, `bool bSuppressCombatStats`, plus two
  pure virtuals for the variation points:
  `virtual TSoftClassPtr<UGameplayEffect> GetEffectClass() const` and `virtual FGameplayTag GetMagnitudeTag() const`.
  Move the shared `ApplyEffect` and the shared `UpdateContextHandle` flag block into it. Damage/Heal/Shield then
  shrink to a handful of lines each (Damage keeps its extra `bDoNotRedirectSacrifice` + basic-ability lookup, Heal
  keeps `bSuppressHealProvided`).
  **Migration note**: these are `UPROPERTY` fields on serialized assets — renaming `DamageAmount`/`HealAmount`/
  `ShieldAmount` to `Amount` and the three `bIs*PerSecond` to `bIsPerSecond` needs `[CoreRedirects]` property
  entries in `DefaultEngine.ini`, or the designer values silently reset to zero. Do this in one commit and verify
  a damage ability in PIE before moving on.

### 2.2 `BuildEffectsSummary` re-does polymorphic dispatch by hand

- **What**: `AbilityInfo.cpp:128-180` is a 50-line `if (Data.GetPtr<FDamageEffectData>()) ... else if
  (Data.GetPtr<FHealEffectData>()) ...` chain producing one description line per effect type — for a hierarchy
  that already has virtual functions.
- **Why**: Adding a new `FEffectData` subclass requires editing an unrelated file, and forgetting to do so fails
  silently (the effect just never appears in any tooltip). The chain also has to be kept in the right order,
  since `GetPtr<FGameplayEffectData>` would match a subclass.
- **What to change**: Add `virtual FString GetDescriptionLine(FDescriptionFormat const&) const` to `FEffectData`
  (returning empty by default) and move each branch's body onto its own struct. `BuildEffectsSummary` collapses to
  a loop that appends non-empty lines. `FDescriptionFormat` and `FormatScalableRange` move from the anonymous
  namespace in `AbilityInfo.cpp` into `EffectData.h`/`.cpp`. After 2.1, Damage/Heal/Shield share one
  implementation on `FMagnitudeEffectData` with a virtual label.

### 2.3 `{Damage}` / `{Heal}` / `{Shield}` token resolution re-tests the token inside the loop

- **What**: `AbilityInfo.cpp:236-266` — inside the per-effect loop, each of the three branches re-evaluates
  `Token == TEXT("Damage") ? Data.GetPtr<FDamageEffectData>() : nullptr`. The token is constant for the whole
  loop, so this is three string compares per effect entry, written as a conditional inside an `if`-initialiser.
- **Why**: Hard to read (the `?:` inside the init-statement obscures that this is really "pick one type, then
  sum"), and it duplicates the type list that 2.2 already maintains.
- **What to change**: After 2.1/2.2 this branch disappears: sum `Amount` over entries whose
  `GetMagnitudeTag()` matches the tag for the token, resolved once before the loop. Failing that, at minimum hoist
  the three `Token ==` comparisons into three `bool const` locals above the loop.

### 2.4 `GetAbilityTagFromCDO` / `GetAbilityTypeTagFromCDO` / `GetAbilityTagFromAbility` — one function written three times

- **What**: `AbilityInfo.cpp:17-39` and `:41-60` are the same function differing only in the root tag and the
  ensure message. `GeoAbilitySystemLibrary.cpp:234-245` (`GetAbilityTagFromAbility`) is a third copy of the same
  "first asset tag under root" walk, in a different file.
- **Why**: "If two functions differ only by a constant, merge them into one with a parameter." Three copies of a
  6-line loop, and the library copy resolves its root tag through the cache described in 1.8 while these two
  resolve it differently again (`RequestGameplayTag` vs `FGeoGameplayTags::Get().Ability_Type`).
- **What to change**: Keep one implementation — `GeoASLib::GetFirstAssetTagUnderRoot(UGameplayAbility const&,
  FGameplayTag Root)` — and have all three call sites use it. `GetAbilityTagFromCDO`/`GetAbilityTypeTagFromCDO`
  become one-line wrappers that supply the root and the ensure message, or take the root as a parameter directly.

### 2.5 `AbilityDescriptions.txt` parsing/writing hand-rolls INI section handling

- **What**: `LoadDescriptionFromFile` (`:309-338`) and `WriteDescriptionToFile` (`:346-390`) together are ~80
  lines of manual `[Section]` detection, comment skipping, body replacement and re-joining — a hand-written INI
  parser and writer over a file whose format is exactly INI sections.
- **Why**: The two hand-rolled state machines (`bInSection`, `bReplaced`/`bSkippingBody`) are the trickiest code
  in the file and are only exercised in the editor, so a bug in them surfaces as a designer's text silently
  vanishing. `FConfigFile` already does all of it, including the UTF-8 write the comment at `:388` works around.
- **What to change**: Replace both with `FConfigFile` (`Read`, `SetString`/`GetString` on section
  `AbilityTag.ToString()`, `Write`). Descriptions are multi-line, so store them as a repeated key or with escaped
  newlines — check that against the current `.txt` before switching, since the file is authored by hand.
  Secondary: `LoadDescriptionFromFile` is called once *per ability* from `GetResolvedDescription` (`:396`), so
  the whole file is loaded and parsed N times when the HUD builds a description list — one parse into a
  `TMap<FGameplayTag, FString>` per call to the outer routine would fix it while keeping live re-read.

### 2.6 Three near-identical "walk every ability info" traversals

- **What**: `GetAllAbilityInfoPtrs` (`:530-546`), `GetAllPlayersAbilityInfos` (`:607-615`) and
  `GetAllAbilityInfos` (`:618-630`) each enumerate the same five arrays, in three different styles: an
  initialiser-list of array pointers, four explicit `Append`s, and a loop over the result of the second. Add
  `PopulateAbilityTags` (`:514-527`) and `FindAbilityInfoForListOfTag` (`:655-682`), which enumerate them again by
  hand.
- **Why**: Five places that must be updated together the day a sixth ability array is added, and nothing links
  them. `GetAllAbilityInfos` also copies every `FGameplayAbilityInfo` twice (once into the
  `GetAllPlayersAbilityInfos` temporary, then again into `AllInfos`).
- **What to change**: Keep `GetAllAbilityInfoPtrs()` as the single traversal (it already handles all five arrays)
  and add a `const` overload returning `TArray<FGameplayAbilityInfo const*>`. Express `GetAllAbilityInfos`,
  `PopulateAbilityTags` and `FindAbilityInfoForListOfTag` on top of it. `GetAllPlayersAbilityInfos` keeps its own
  body only because its return type is the derived struct — give it the same array-pointer-list form as
  `GetAllAbilityInfoPtrs` so all traversals read alike.

### 2.7 Doc drift: `Data/CLAUDE.md` names a struct that does not exist

- **What**: `Public/AbilitySystem/Data/CLAUDE.md` lists `FSingleUseDamageMultiplierEffectData`; the actual struct
  is `FContextDamageMultiplierEffectData` (`EffectData.h:222`).
- **Why**: Not code, but it is the map future sessions read first — a wrong name there costs a grep.
- **What to change**: Rename in the doc. (Flagged only — per `CLAUDE.md` the subfolder docs are the daily agent's
  job, not an interactive session's.)

---

## 3. `Actor/Projectile` + `Actor/Deployable`

### 3.1 The `bIsEnding` guard is written at six call sites instead of once in `EndProjectileLife`

- **What**: `GeoProjectile.cpp` checks `bIsEnding` before calling `EndProjectileLife` in `LifeSpanExpired`
  (`:175`), `OnSphereHit` (`:339`), `OnInstigatorRevived` (`:625`), plus `Tick` (`:196`), `AdvanceProjectile`
  (`:421`) and `IsValidOverlap` (`:247`). `EndProjectileLife` itself (`:368`) does *not* check it — it just sets
  it to true.
- **Why**: "One mechanism, not two" — the flag exists precisely to make ending idempotent, but idempotency is
  enforced by every caller rather than by the function that owns the flag. A new call site that forgets the guard
  double-broadcasts `OnProjectileEndLifeDelegate` and double-plays the impact FX.
- **What to change**: Add `if (bIsEnding) { return; }` as the first line of `EndProjectileLife`. Then
  `LifeSpanExpired` becomes `if (LifeSpanInSec != 0) { EndProjectileLife(); }`, `OnSphereHit` becomes
  `if (!ProjectileMovement->bShouldBounce) { EndProjectileLife(); }`, and `OnInstigatorRevived` becomes a single
  unconditional call. The `Tick` and `AdvanceProjectile` guards stay — they gate other work too.

### 3.2 `PlayImpactFx` opens with `if (!IsValid(this))`

- **What**: `GeoProjectile.cpp:348-353`. A member function testing its own `this` for validity.
- **Why**: It cannot be doing what it appears to. On the normal path (`EndProjectileLife` → `PlayImpactFx`) the
  actor is alive and the check is dead code. On the `Destroyed()` path (`:182-189`) the actor has already been
  marked garbage by `Destroy()`, so `IsValid(this)` is **false** and the guard silently skips the client-side
  impact sound and Niagara burst — the opposite of the intent at that call site.
- **What to change**: Delete the guard, then check in PIE whether client impact FX changes. If it does, that
  confirms the `Destroyed()` path was being suppressed and the behaviour is now correct; if a real re-entrancy
  concern turns up, express it as an explicit flag rather than a `this` validity test.

### 3.3 `IsValidOverlap` is five sequential early-returns, and recomputes ASCs the caller needs anyway

- **What**: `GeoProjectile.cpp:229-265` has five separate `if (...) { return false; }` blocks. Three of them are
  plain validity checks. It resolves `GetGeoAscFromActor` for owner, instigator and other actor purely to test
  them, discards all three, and `HandleValidOverlap` (`:274-275`) then resolves two of them again.
- **Why**: Style rule "prefer fewer but longer `if` statements — merge conditions with `&&`". Six
  `GetGeoAscFromActor` calls (each an interface cast plus a component lookup) per overlap where two suffice.
- **What to change**: Merge the two validity blocks and the `bIsEnding` / `CanBeDamaged` blocks into two `&&`
  conditions. Then have `IsValidOverlap` take `UGeoAbilitySystemComponent*& OutOwnerASC, UGeoAbilitySystemComponent*& OutTargetASC`
  (or return a small struct) so `HandleValidOverlap` reuses them instead of re-resolving. Function drops from
  ~36 lines to ~15.

### 3.4 Sound playback is spelled three different ways inside `AGeoProjectile`

- **What**:
  - `GetPitch(FGeoSoundEntry const&)` (`:295-298`) is a one-line forward to `UGeoSoundRowLibrary::GetPitch`.
  - `GetPitch_Implementation(EProjectileSoundType)` (`:283-292`) and `PlaySoundOneShot(EProjectileSoundType)`
    (`:308-314`) each repeat the same `ResolvedParams.SoundMap.Find(SoundType)` + delegate shape.
  - `InitProjectileLife` (`:580-588`) hand-rolls the `ShouldPlay` → `GetVolume` → `GetPitch` → play sequence
    against `LoopingSoundComponent`, which is exactly what `PlaySoundOneShot(FGeoSoundEntry const&)` (`:317-325`)
    does against `PlaySoundAtLocation`.
- **Why**: Four spellings of "resolve an entry, then play it". `Data/CLAUDE.md` states `UGeoSoundRowLibrary` is
  "the single playback path" — the looping block bypasses that intent.
- **What to change**: Delete `GetPitch(FGeoSoundEntry const&)` and call the library directly (2 call sites).
  Add `UGeoSoundRowLibrary::ConfigureAudioComponent(UAudioComponent*, FGeoSoundEntry const&, AActor* Instigator)`
  next to the existing `PlaySoundEntry2D` and have `InitProjectileLife` call it, so the looping path and the
  one-shot path share the `ShouldPlay`/`GetVolume`/`GetPitch` triple.

### 3.5 `ApplyProjectileParams` restates the parameter list a third time

- **What**: `GeoProjectile.cpp:540-573` — eleven consecutive
  `Resolved.X = ResolveOverrideParam(Params.OverrideX, Params.X, DefaultParams.X);` lines. The same field set is
  already declared in `FProjectileParamsBase` and again (with its `Override*` twin) in `FExternalProjectileParams`.
- **Why**: Adding one projectile parameter means three coordinated edits, and forgetting the third fails
  silently — the new param just never honours its override. `:563-565` already shows the failure mode:
  `LifeTimeThresholdBeforeOverlapSelf` is resolved using `Params.OverrideCanOverlapInstigator`, another field's
  mode. That is probably deliberate pairing, but nothing in the code says so.
- **What to change**: This one is *not* worth a reflection-driven rewrite — the explicit list is readable and the
  editor customization (`ExternalProjectileParamsCustomization`) already depends on the paired naming. Do the cheap
  half only: add a one-line comment at `:563` stating that the threshold intentionally shares the
  `bCanOverlapInstigator` override mode, so the next reader does not "fix" it. If the list grows past ~15 entries,
  revisit with a macro over a field list.

### 3.6 Recall cosmetics are written twice — server path and `OnRep` path

- **What**: `GeoDeployableBase.cpp` — `Recall()` (`:289-309`) fires the recall cue, the recall sound and (via
  `Explode()`) the explode cue + explode sound. `OnRep_Active` (`:473-489`) re-spells that identical sequence
  inline for clients.
- **Why**: Textbook "extract by concept": one concept ("play the recall cosmetics") written as two divergent
  copies. They have *already* drifted — `Recall()` guards the recall cue with
  `!IsDedicatedServer && RecallGameplayCueTag.IsValid()` and routes the explode cue through `Explode()`, while
  `OnRep_Active` guards neither and calls `ExecuteCue(ExplodeGameplayCueTag, GetGenericCueParams(...))` directly,
  skipping the `CueParams.Normal.X = Value` that `Explode()` sets.
- **What to change**: Extract `void PlayRecallCosmetics(float Value)` containing the cue/sound sequence and call
  it from both. Resolve the drift deliberately while doing so — decide whether the client path should carry
  `Value` in `Normal.X` (it currently cannot, since `Value` is not replicated; if it matters, replicate it,
  otherwise document that clients get 0).

### 3.7 `bBlinking` and `BlinkTimerHandle` both store "is blinking"

- **What**: `GeoDeployableBase` has a replicated `bBlinking` (`:150`, set in `StartBlinking` `:430`, cleared in
  `InitInteractable` `:54`) *and* `IsBlinking()` (`:520-523`) which reads `BlinkTimerHandle.IsValid()` instead.
  `OnHealthChanged` (`:452`) also asks the question through the timer handle.
- **Why**: "Don't add state to track what an existing construct already bounds." Two representations of one fact,
  read inconsistently — `GetRecallCueParams` (`:561`) uses the timer, replication uses the bool. They agree today
  only because `OnRep_Blinking` happens to start the timer on clients.
- **What to change**: Make `IsBlinking()` return `bBlinking` and route `OnHealthChanged`'s guard through it. The
  timer handle then has exactly one job (firing `TryRecallOrExpire`), which is what a timer handle should be.

### 3.8 `Expire()` disables collision twice; cue-param getters should be `const`

- **What**: `GeoDeployableBase.cpp` — `Expire` calls `SetActorEnableCollision(false)` at `:372` and again at
  `:390` inside the simulated-proxy branch. Separately, `GetSpawnCueParams`, `GetBlinkCueParams`,
  `GetGenericCueParams` and `GetRecallCueParams` (`:528-563`) mutate nothing but are all non-`const`, as is
  `PushAway`'s helper usage.
- **Why**: The duplicate call is dead. The missing `const` breaks the "`const` by default" rule and blocks
  calling them from the `const` methods that build cue params.
- **What to change**: Delete line `:390`. Mark the four cue-param getters `const` in the header and the .cpp.

### 3.9 `AGeoDeployableBase::Tick` rebuilds a constant `FDamageEffectData` every tick

- **What**: `:173-188` constructs `FDamageEffectData` from scratch on each tick; four of its five fields
  (`bSuppressGameplayCue`, `bLimitGameplayCue`, `bDoNotRedirectSacrifice`, and implicitly the effect class) are
  fixed per-deployable — only `DamageAmount` varies.
- **Why**: Constant construction inside the hot path, and the drain's configuration is spelled at the point of use
  rather than where the drain is set up (`InitDrain`, `:153-169`), so the two halves of one feature sit apart.
- **What to change**: Build the `FDamageEffectData` once in `InitDrain` into a member alongside
  `DrainMagnitudePerSecond`; `Tick` then only sets `DamageAmount` and applies. `InitDrain` becomes the single
  place the drain is described.

### 3.10 `PushAway` is one 79-line function doing four things

- **What**: `GeoDeployableBase.cpp:63-142` — filters characters, computes a push direction, sweep-tests and
  re-aims the target at the arena's fight centre, builds a root-motion source, and schedules its removal, all in
  one loop body.
- **Why**: The interesting logic (the blocked-push redirect toward `TargetPoint.FightCenter`, `:101-117`) is
  buried in the middle of boilerplate, and the function's name promises one action.
- **What to change**: Extract two private helpers — `FVector ComputePushTarget(AActor* Target, float PushDistance,
  AGeoArena const* FightingArena) const` (the direction + sweep + fight-centre redirect) and
  `void ApplyPushRootMotion(UCharacterMovementComponent* Movement, FVector const& From, FVector const& To)` (the
  root-motion source + removal timer). `PushAway` reduces to a filter loop plus two calls.

---

## 4. `Characters/`

### 4.1 The two charge-gauge visibility functions are one function written twice

- **What**: `PlayableCharacter.cpp` — `SetDeployChargeGaugeVisibility` (`:58-89`) and `SetChargeBeamGaugeVisible`
  (`:91-124`) have identical structure: cast the component's user widget to an interface, `ensureMsgf`, then
  branch on `bVisible` into (clear timer → unhide → set ability) or (set ability → `UpdateVisualChargeRatio` →
  set null → 0.15s hide timer via `CreateWeakLambda`). They differ only in the interface type, the setter name,
  the component, the timer handle, and one extra `SetSweetSpotRatios` call.
- **Why**: ~35 duplicated lines including the same magic `0.15f` twice and the same non-obvious
  "set ability → update → clear" ordering trick, whose explanatory comment
  (`// In case we haven't got time to enter visibility.`) is also duplicated. The two interfaces already declare
  the same `UpdateVisualChargeRatio() const` (`GeoDeployGaugeWidgetInterface.h:30`,
  `GeoChargeBeamGaugeWidgetInterface.h:32`) and a one-argument `Set*Ability` that differ only in name.
- **What to change**: Give the two UINTERFACEs a shared base `IGeoChargeGaugeWidgetInterface` declaring
  `SetChargeAbility(UGeoGameplayAbility*)` and `UpdateVisualChargeRatio() const` (rename `SetDeployAbility` /
  `SetChargeBeamAbility` to the common name). Then write one private helper
  `void SetChargeGaugeVisible(UWidgetComponent* Component, FTimerHandle& HideHandle, UGeoGameplayAbility* Ability, bool bVisible)`
  and reduce both public methods to a call plus, for the beam, the `SetSweetSpotRatios` line. Hoist `0.15f` to a
  named `constexpr float GaugeHideDelay`.

### 4.2 `ClassData.Find(GetPlayerClass())` + its ensure is repeated four times

- **What**: `PlayableCharacter.cpp` — `SetDeathVisuals` (`:264-268`), `GetDeathMontage` (`:274-278`), `GiveLife`
  (`:412-417`) and `ApplyClassData` (`:425-430`) each perform the map lookup and each handle the miss
  differently: two use `if (!ensureMsgf(...))`, one uses `if (!X) { ensureMsgf(X, ...); return; }`, one returns
  `nullptr` with no diagnostic at all.
- **Why**: One question ("what is this character's class data?") answered in four places with four different
  failure policies. The silent `nullptr` in `GetDeathMontage` is the odd one out and would hide a real
  configuration bug.
- **What to change**: Add `FPlayerClassData const* GetClassData(EPlayerClass Class) const` containing the lookup
  and one `ensureMsgf`, plus a no-argument overload defaulting to `GetPlayerClass()`. All four call sites become
  a single null-check on the result. Decide once whether a miss ensures (it should — it is a configuration bug per
  `AI/CodingStyle.md`).

### 4.3 `BindToOwnerASC` is wired in three places with three near-identical comments

- **What**: `AGeoCharacter::BeginPlay` (`GeoCharacter.cpp:188-191`), `AGeoCharacter::InitGAS` (`:174-177`) and
  `APlayableCharacter::InitGAS` (`PlayableCharacter.cpp:180-183`) each contain the same
  `if (IGeoCombattantWidgetHost* WidgetHost = Cast<IGeoCombattantWidgetHost>(CharacterWidgetComponent)) { WidgetHost->BindToOwnerASC(); }`
  block, each preceded by its own paraphrase of the same "attributes are set synchronously, re-bind now" comment.
- **Why**: Three copies of a load-order workaround. If the binding rule ever changes (a fourth entry point, a
  guard), it has to be found in three files. `AGeoDeployableBase::BeginPlay` (`:262-265`) is a fourth instance of
  the same shape on the deployable side.
- **What to change**: Add a protected `void BindCombattantWidgetToASC()` on `AGeoCharacter` holding the cast, the
  call and the single explanatory comment; call it from all three sites. (The deployable's copy stays separate —
  different base class — but should use the same function name so the pattern is greppable.)

### 4.4 `StopAllSpawnedElements` nests the deployable cleanup inside the ASC null-check

- **What**: `GeoCharacter.cpp:111-121` — `if (AbilitySystemComponent) { StopAllActivePatterns(); if (DeployableManagerComponent) { ForceExpireAll(); } }`.
- **Why**: The deployable manager has no dependency on the ASC, but with the current nesting a character whose ASC
  is null (a proxy during a class swap, a dedicated-server edge case) silently keeps its deployables alive. The
  nesting reads as accidental rather than intended.
- **What to change**: Split into two sibling `if` statements. Two lines, removes an implicit coupling.

### 4.5 `Revive()` and `OnRep_IsDead` duplicate the revive pair; `Death()`/`DeathLogic` mirror it

- **What**: `GeoCharacter.cpp` — `Revive()` (`:265-274`) ends with `ReviveLogic(); OnRevived.Broadcast();`, and
  `OnRep_IsDead` (`:257-261`) repeats the same two calls.
- **Why**: Small, but it is the same "extract by concept" case as 3.6: the server path and the `OnRep` path each
  spell out the revive sequence, so a third step added to revive can be added to only one of them.
- **What to change**: Extract `void HandleRevived()` containing the two calls and use it from both. Note the death
  side already does this correctly via `DeathLogic()` — this just makes revive symmetric with it.

---

## 5. `GeoTrinityUI/HUD/Menu` — the menu-widget boilerplate *(largest single line saving in the report)*

### 5.1 Every menu widget re-validates `BindWidget` properties the engine already guarantees

- **What**: `UGeoPauseMenuWidget::NativeConstruct` (`GeoPauseMenuWidget.cpp:20-54`) opens with seven consecutive
  five-line blocks:
  ```cpp
  if (!ResumeButton)
  {
      ensureMsgf(ResumeButton, TEXT("UGeoPauseMenuWidget: ResumeButton is not bound"));
      return;
  }
  ```
  The same shape appears in `GeoSettingsWidget.cpp` (5), `GeoServerRowWidget.cpp` (5),
  `GeoMainMenuWidget.cpp` (7), `GeoCreateServerWidget.cpp` (7), `GeoBrowseServersWidget.cpp` (7),
  `GeoLocalConnectWidget.cpp` (5), `GeoSoundSettingsWidget.cpp` (2), `GeoKeyBindingsWidget.cpp` (2),
  `GeoMenuButton.cpp` (2), `GeoAbilityDescriptionsWidget.cpp` (2) — roughly **50 blocks, ~250 lines**.
- **Why**: Every one of these properties is declared `UPROPERTY(meta = (BindWidget))` (confirmed in
  `GeoPauseMenuWidget.h:36-55`). UMG's widget-blueprint compiler **refuses to compile a Blueprint** whose
  `BindWidget` property has no matching widget of the right type — that is the entire purpose of the specifier,
  and `BindWidgetOptional` is the opt-in for nullable. So these checks assert a condition the asset pipeline has
  already proven; they can never fire for any widget that was able to load. This is exactly the "no dead code
  safety nets" rule.
- **What to change**: Delete the blocks. Before doing so, grep each widget header for
  `BindWidgetOptional` and **keep** a guard for those (`GeoKeyBindingsWidget`'s
  `SecondPlayerGamepadCheckBox` is documented as optional in `HUD/CLAUDE.md` and genuinely needs its null check).
  Do it one widget at a time and reopen each menu in PIE — a widget that turns out to be created from the C++
  class rather than a BP subclass would now null-deref instead of early-returning, which is the one case worth
  confirming.

### 5.2 `NativeDestruct` exists only to undo `NativeConstruct`'s `AddDynamic`

- **What**: `GeoPauseMenuWidget.cpp:72-103` is 30 lines of
  `if (X) { X->OnClicked.RemoveDynamic(this, &UGeoPauseMenuWidget::HandleY); }`, one per binding, mirroring the
  seven `AddDynamic` calls at `:56-62`. The same paired structure repeats in every menu widget listed in 5.1.
- **Why**: The only reason the removals exist is that `NativeConstruct` can run more than once on a reused widget
  instance, which would double-bind. `AddUniqueDynamic` states that intent in one word, and the bound objects are
  child widgets owned by this widget — they do not outlive it, so nothing leaks.
- **What to change**: Change the `AddDynamic` calls to `AddUniqueDynamic` and delete `NativeDestruct` entirely
  (plus its declaration and doc comment in each header). Across the menu widgets this removes ~150 lines and one
  whole class of "I added a binding but forgot to unbind it" bug.

### 5.3 The visibility fan-out and the binding fan-out repeat the member list a third time

- **What**: `SetButtonsVisible` (`GeoPauseMenuWidget.cpp:185-193`) sets the same visibility on five buttons named
  one by one — the third place (after the ensure block and the bind/unbind pair) that enumerates the same members.
- **Why**: Adding a button to the pause menu currently means four edits (header property, ensure block, bind,
  unbind, visibility) and forgetting any one fails quietly.
- **What to change**: After 5.1 and 5.2 remove two of the lists, this is the only one left, which is acceptable.
  If you want it gone too: collect the buttons into `TArray<TObjectPtr<UGeoMenuButton>>` once in
  `NativeConstruct` and loop. Judgement call — the explicit version is more readable at five entries, so this is
  optional and lowest priority in the section.

---

## 6. `AbilitySystem/Components` — `UGeoAbilitySystemComponent`

### 6.1 `MakeGeoEffectContext` is dead code that would return a dangling pointer

- **What**: `GeoAbilitySystemComponent.cpp:132-136` declares a local `FGameplayEffectContextHandle handle`,
  returns `handle.Get()`, and lets `handle` — the only owner of the context — destruct on return.
- **Why**: A grep across the whole solution finds **no call sites**: only the declaration
  (`GeoAbilitySystemComponent.h:35`) and the definition. So it is dead code, and the first caller to use it would
  get a pointer to a freed `FGeoGameplayEffectContext`.
- **What to change**: Delete the function and its declaration. Anything needing a Geo context should go through
  `GeoASLib::ApplyEffectFromEffectData`, which keeps the handle alive for the whole application (and which
  finding 1.2 consolidates).

### 6.2 The `UAbilityInfo` null-check is copy-pasted three times

- **What**: `GiveStartupAbilities(TArray…)` (`:141-146`), `GiveStartupAbilities(EPlayerClass…)` (`:168-173`) and
  `ClearPlayerClassAbilities` (`:204-209`) each open with the same five-line
  `UAbilityInfo* AbilityInfos = GetAbilityInfo(this); if (!AbilityInfos) { ensureMsgf(...); return; }`, the
  message differing only by the function name.
- **Why**: Three copies of a guard, in the deprecated `if (!X) { ensureMsgf(X, …); return; }` form flagged in 1.3.
- **What to change**: Add a file-local
  `static UAbilityInfo* GetCheckedAbilityInfo()` returning nullptr after one `ensureMsgf` that uses
  `__FUNCTION__` for context. Each call site becomes two lines. Combines with 1.1 (drop the `this` argument).

### 6.3 Three `GiveStartupAbilities` overloads where one and a half would do

- **What**: `:139` (tag array), `:160` (pure forwarder passing `StartupAbilityTags`), `:166` (player class). The
  tag-array overload has **no external callers** — grep shows it is reached only through the forwarder at `:162`.
  The class overload duplicates the array overload's spec-build/`GiveAbility`/`bStartupAbilitiesGiven` tail.
- **Why**: "Never add a wrapper that just forwards with a hardcoded argument", plus a public API entry nothing
  calls. The real callers are only `GiveStartupAbilities()` (`GeoCharacter.cpp:169`,
  `GeoInteractableActor.cpp:112`) and `GiveStartupAbilities(EPlayerClass)` (`PlayableCharacter.cpp:175`, `:396`).
- **What to change**: Fold the tag-array body into the no-argument overload (it always reads `StartupAbilityTags`)
  and delete the array overload from the header. Extract the shared tail —
  `void GiveAbilitySpec(TSubclassOf<UGameplayAbility>, int32 Level, FGameplayTag InputTag)` — and call it from
  both remaining overloads.

### 6.4 The Lyra prediction-key block is duplicated verbatim, comment included

- **What**: `:274-283` (inside `AbilityInputTagPressed`) and `:337-345` (inside `ReleaseAbilitySpec`) are the same
  six lines — `PRAGMA_DISABLE_DEPRECATION_WARNINGS`, `GetPrimaryInstance()`, the ternary onto
  `ActivationInfo.GetActivationPredictionKey()`, `PRAGMA_ENABLE_DEPRECATION_WARNINGS` — carrying the identical
  "Code from Lyra starter game" comment.
- **Why**: This is the most delicate code in the file (it works around a deprecated engine API), so having two
  copies means an engine upgrade has to fix it twice, and one copy will be missed.
- **What to change**: Extract `static FPredictionKey GetActivationPredictionKey(FGameplayAbilitySpec const& Spec)`
  holding the pragma pair, the ternary and the one comment. Both sites become a single call.

### 6.5 `AbilityInputTagPressed` and `AbilityInputTagHeld` share most of their loop

- **What**: `:238-286` and `:289-312` both validate the tag, take a `FScopedAbilityListLock`, iterate
  `GetActivatableAbilities()`, filter on `HasTagExact(inputTag)`, call `AbilitySpecInputPressed`, then
  conditionally `TryActivateAbilityWithTargetData` — the *only* logical difference being the polarity of
  `bActivateOnFreshPressOnly`. Pressed adds the alternate-release handling and the `InputPressed` replicated event
  on top.
- **Why**: The shared 15 lines drifted once already (Held has no `UE_VLOG`), and the `bActivateOnFreshPressOnly`
  polarity being split across two functions is what makes the feature hard to follow.
- **What to change**: Extract `void ActivateAbilitiesForInput(FGameplayTag const& InputTag, bool bFreshPress)`
  containing the lock + loop + filter + `AbilitySpecInputPressed` + the polarity-parameterised activation. Keep
  the alternate-release and `InvokeReplicatedEvent` steps in `AbilityInputTagPressed` around that call.

---

## 7. `AbilitySystem/Abilities/Base` — `UGeoGameplayAbility`

### 7.1 `CreateAbilityPayload()` draws two different random seeds

- **What**: `GeoGameplayAbility.cpp:96-102`:
  ```cpp
  int const Seed = GetNewSeed();
  return CreateAbilityPayload(
      GetFireOrigin2D(GetAvatarActorFromActorInfo(), GetGeoAbilitySystemComponentFromActorInfo(), GetNewSeed()),
      GetFireYaw(GetAvatarActorFromActorInfo(), Seed), GetStartTime(GetWorld()), Seed);
  ```
  `GetNewSeed()` is called a second time for the origin argument instead of reusing `Seed`. Since `GetNewSeed()`
  is `FMath::Rand32()` (`:492-495`), that is a *different* random value from the one stored in the payload.
- **Why**: It reads as a copy-paste slip, and it violates the GAS rule in `AI/CodingStyle.md` that every value in
  a shot must derive from the one replicated seed. It is currently harmless only because the base
  `GetFireOrigin2D` ignores its seed parameter (declared `int` with no name, `:409`) — but any override that
  starts using the seed (a spread, a jitter) would silently desync client and server.
- **What to change**: Pass `Seed` instead of `GetNewSeed()`. One-token fix; do it before anything else in this
  section.

### 7.2 `HandleAnimationMontage` validates the section name twice

- **What**: `:270-293` builds `FireSectionName`, tests `IsValidSectionName`, rebuilds it as `Fire1` on failure,
  assigns `SectionToJumpTo`, then immediately runs a *second* `IsValidSectionName(SectionToJumpTo)` check with an
  error log and a `Start` fallback.
- **Why**: Two fallback layers for one question, with two different fallbacks (`Fire1`, then `Start`). The nesting
  makes it hard to see that the `FireSectionIndex == 0` branch can never fail the second check.
- **What to change**: Collapse to one resolution step: compute the candidate name, and keep a single
  `if (!AnimMontage->IsValidSectionName(SectionToJumpTo))` that resets `FireSectionIndex` to 1 and retries `Fire1`,
  falling through to `Start` with the existing error log. One `if` instead of two nested layers.

### 7.3 Same camelCase/Hungarian naming drift as 1.10

- **What**: `GetCooldown` (`:137-149`) uses `cooldown` / `pCooldownEffect`; `UGeoAbilitySystemComponent` uses
  `inputTag`, `abilitySpec`, `activeScopeLock`, `originalPredictionKey`. Everything around them is PascalCase.
- **Why**: Same consistency rule as 1.10 — these are the last pockets of an older convention.
- **What to change**: Rename in place. Group this with 1.10 into one mechanical commit so the diff is obviously
  behaviour-free.

---

## 8. `GeoTrinityUI/HUD` (non-menu) — `AGeoHUD`, overlay, combatant widget

### 8.1 Four HUD queries repeat the same "find the spec for this ability tag" loop

- **What**: `GeoHUD.cpp` contains the identical loop body four times — `GetAbilityCooldown` (`:360-387`),
  `IsAbilityActive` (`:390-408`), `CanActivateAbility` (`:411-435`), `GetDeployCountForAbility` (`:438-464`). Each
  one: fetches `HudPlayerParams.GetGeoAbilitySystemComponent()`, early-returns if null, walks
  `ASC->GetActivatableAbilities()`, casts `Spec.Ability` to `UGeoGameplayAbility`, compares `GetAbilityTag()`, then
  promotes to `Spec.GetPrimaryInstance()`. Three of the four carry a near-identical comment explaining the CDO →
  primary-instance promotion.
- **Why**: ~70 lines of the file are one function written four times. The promotion step is the fragile part — it
  is the reason a cooldown or a stack count reads the CDO's empty state instead of the live instance — and having
  it written four times means the next query added will be the one that forgets it. `IsAbilityActive` already
  *does* forget it (`:398-405` returns `Spec.IsActive()` without promoting), which is correct there but not
  obviously so, because it reads as the odd one out.
- **What to change**: One private helper on `AGeoHUD`:
  ```cpp
  FGameplayAbilitySpec const* FindSpecForTag(FGameplayTag AbilityTag, UGeoGameplayAbility const*& OutInstance) const;
  ```
  returning the spec plus the promoted instance (falling back to the CDO). Each of the four becomes three or four
  lines. Keep `IsAbilityActive` reading the spec rather than the instance, and the helper makes that distinction
  visible instead of accidental.

### 8.2 `BroadcastInitialValues` and `BindCallbacksToDependencies` restate the same five attributes

- **What**: `:160-177` and `:180-230` list the same five attribute → delegate pairs (Health, MaxHealth, Shield,
  and — behind the same `Cast<UCharacterAttributeSet>` — Ammo, MaxAmmo). The second is five copies of a six-line
  `AddWeakLambda` block that differ only in which attribute getter and which broadcast delegate they name.
- **Why**: Adding a sixth attribute to the HUD means editing two functions in two places each, and the pairing
  between them is only enforced by eye. 50 lines express five facts.
- **What to change**: A local table of `{ FGameplayAttribute, FOnAttributeChangedSignature* }` walked by both
  functions — one loop broadcasts the current value, the other binds. The `UCharacterAttributeSet` cast then
  decides only whether the last two rows are appended to the table.

### 8.3 `ShowBossHealthBar` dereferences an unchecked interface cast

- **What**: `:291`:
  ```cpp
  if (UAbilitySystemComponent* BossASC = Cast<IAbilitySystemInterface>(Boss)->GetAbilitySystemComponent())
  ```
  The result of `Cast<IAbilitySystemInterface>` is dereferenced without a null check, inside a function whose own
  first lines carefully null-check `Boss`, `BossHealthBarWidgetClass` and `LocalPlayer`.
- **Why**: A crash rather than a missing bar if a boss class ever stops implementing the interface. It is also
  inconsistent with the rest of the file, which null-checks everything.
- **What to change**: `AEnemyCharacter` already exposes the ASC — call `Boss->GetAbilitySystemComponent()`
  directly and drop the cast. (Related, same function: `HideBossHealthBar()` is called at `:281` *before* the
  `ShouldDrawHUD` early-return at `:283`, so on a no-HUD path the old bar is torn down and no new one is built.
  Move the `ShouldDrawHUD` test up with the other guards.)

### 8.4 Two parallel mechanisms deliver the same attribute changes to the UI

- **What**: `AGeoHUD` binds `GetGameplayAttributeValueChangeDelegate` directly with weak lambdas and re-broadcasts
  through its own `OnHealthChanged` / `OnMaxHealthChanged` / `OnShieldChanged`. `UGenericCombattantWidget`
  (`:138-148`) instead subscribes to `UGeoAbilitySystemComponent`'s *own* `OnHealthChanged` / `OnMaxHealthChanged`
  dynamic delegates — and for Shield, which the ASC does not expose, falls back to the attribute delegate with a
  comment explaining the asymmetry.
- **Why**: `AI/CodingStyle.md`'s "one mechanism, not two". Two fan-out paths for the same three attributes, and
  the ASC's delegate set is incomplete (Shield missing) precisely because it is the second mechanism, so it never
  got finished. A reader has to know which of the two a given widget uses.
- **What to change**: Pick one. The attribute delegate is the engine's own mechanism and covers every attribute
  without new plumbing, so the smaller change is to drop `UGeoAbilitySystemComponent::OnHealthChanged` /
  `OnMaxHealthChanged` and have `UGenericCombattantWidget::BindStatCallbacks` bind all three attributes the same
  way it already binds Shield. That also deletes `UnbindStatCallbacks` entirely (`:152-165`), since weak lambdas
  self-clean — which the file's own comment at `:141-142` already says.

### 8.5 `UGenericCombattantWidget::OnHealthChanged` ignores the payload it is handed

- **What**: `:168-183` is bound to both `OnHealthChanged` and `OnMaxHealthChanged`, takes a `float NewValue`, and
  opens by discarding it — the comment says "Don't use NewValue, just get the ratio from ASC (it might be health
  or max health)". It then re-reads the ASC for the ratio, the shield and the visibility.
- **Why**: The delegate signature carries a value no handler wants, so every call site pays for a payload that is
  ambiguous by construction. `AGeoHUD` already has the right pattern for this: `OnPlayerDeployCountChanged`
  (`:263-267`) is a deliberately tagless "refresh now" ping, with a comment saying why.
- **What to change**: Folded into 8.4 — once the widget binds attributes directly, this becomes a single
  parameterless `RefreshStats()` bound to all three. If 8.4 is not taken, at least rename the handler
  `HandleStatsDirty` so the discarded parameter reads as intentional.

### 8.6 `InitOverlay`'s `checkf` is missing its semicolon

- **What**: `:65-71`:
  ```cpp
  checkf(OverlayWidgetClass, TEXT("Overlay Widget Class uninitialized, please fill out HUD %s"), *GetName())

      // Setup params the HUD may very probably need to access
      HudPlayerParams.PlayerController = PC;
  HudPlayerParams.PlayerState = PS;
  ```
  The odd indentation is clang-format treating the next statement as a continuation of the `checkf` expression.
- **Why**: It compiles today (the macro's expansion ends in a braced block in every configuration), so this is not
  a live bug — but it is a well-known trap with the `check` family, and the misformatting will reappear on every
  reformat until the semicolon is added.
- **What to change**: Add the `;`. Then the assignments re-indent to column 1 on the next format pass.

### 8.7 Inconsistent policy on unbinding weak lambdas

- **What**: `UGeoOverlayWidget::NativeDestruct` (`:51-59`) exists solely to `RemoveAll(this)` two
  `AddWeakLambda` bindings. `UGenericCombattantWidget:141-142` states the opposite policy for the same kind of
  binding: "A weak lambda self-cleans when this widget is destroyed, so no matching removal is needed".
- **Why**: Both are defensible for their own file — the overlay's `NativeConstruct` can run more than once, so
  without the removal the bindings would stack — but the codebase now documents two contradictory rules for the
  same construct, which is exactly the kind of thing that gets cargo-culted wrong.
- **What to change**: Low priority, and *not* a code change: keep both, but make the overlay's comment say why it
  removes (re-construction stacking) rather than leaving the removal unexplained. Same fix as 5.2 recommends for
  the menu widgets — prefer the binding form that makes re-entry harmless.

---

## 9. `Actor/GeoArena` + `GeoHexArena`

### 9.1 `EndFight`'s branch is about the boss, but is written as if it were about the barrier

- **What**: `GeoArena.cpp:170-188`:
  ```cpp
  if (bBossDefeated)
  {
      if (Barrier) { Barrier->SetClosed(false); }
  }
  else
  {
      ResetBoss();
  }
  ```
  `ResetBoss()` ends (`:85-88`) with that same `if (Barrier) { Barrier->SetClosed(false); }`. So *both* branches
  open the barrier; the only real decision is whether the boss respawns.
- **Why**: The reader has to notice the hidden side effect inside `ResetBoss` to see that the two branches agree
  on the barrier. And `if (Barrier) { Barrier->SetClosed(...) }` appears four times in the file (`:85`, `:142`,
  `:164`, `:179`).
- **What to change**: Add `void AGeoArena::SetBarrierClosed(bool bClosed)` holding the null check, then:
  ```cpp
  SetBarrierClosed(false);
  if (!bBossDefeated) { ResetBoss(); }
  ```
  and drop the barrier line from `ResetBoss` — leaving barrier state owned by the fight-lifecycle functions rather
  than smuggled through a spawn helper.

### 9.2 `ResetBoss` uses the spawn result without checking it

- **What**: `:82-83`:
  ```cpp
  Boss = GetWorld()->SpawnActor<AEnemyCharacter>(BossClass, FTransform(SpawnLocation), SpawnParams);
  Boss->Arena = this;
  ```
  `SpawnActor` can return null, and the function has already gone to the trouble of two `ensureMsgf` guards above.
- **Why**: Null-deref on a failed spawn, in the one function every fight reset runs through.
- **What to change**: `if (!ensureMsgf(Boss, TEXT("%s: failed to spawn %s"), *GetName(), *BossClass->GetName())) { return; }`
  — the same guard form the rest of the file already uses.

### 9.3 `SpawnLootBurst` re-scans the whole ability catalog on every burst

- **What**: `:286-309` walks `AbilityInfo->GetAllAbilityInfos()` looking for the one entry whose class derives from
  `UGeoReloadAbility`, to recover the CDO and its tag. `SpawnLootBurst` is a *repeating* timer
  (`Loot()`, `:270`, `bLoop = true`), so this catalog scan runs once per `LootSpawnInterval` for the whole loot
  shower, always producing the same answer.
- **Why**: Invariant work inside a repeating tick, and it pushes 20 lines of lookup in front of the 60 lines that
  actually spawn pickups, which is what makes the function hard to read.
- **What to change**: Resolve `ReloadCDO` / `ReloadTag` once in `Loot()` (or lazily into a cached member) and have
  `SpawnLootBurst` start at the actual spawning. The `ensureMsgf` + `ClearTimer` failure path then belongs in
  `Loot()`, where it can simply decline to start the timer.

### 9.4 `GetFightingArena` and `RespawnAllBosses` are the same traversal twice

- **What**: `:104-117` and `:196-205` both call `UGameplayStatics::GetAllActorsOfClass(WorldContextObject,
  StaticClass(), Arenas)` and loop with `CastChecked<AGeoArena>`.
- **Why**: Minor duplication, but `GetAllActorsOfClass` is the expensive way to ask this — and `TActorIterator`
  needs no temporary array or cast.
- **What to change**: Replace both bodies with `for (TActorIterator<AGeoArena> It(World); It; ++It)`. Two lines
  each, no array, no cast, no `CastChecked` that can only ever succeed.

### 9.5 Server-only ensures in `GeoHexArena` name the wrong function

- **What**: Eight `ensureMsgf(GeoLib::IsServer(this), ...)` guards, each with its own hand-written message. Two of
  them are copy-paste survivors: `HighlightTiles` (`:349`) and — via the same paste — the plural path report
  `"HighlightTile is server-only because it's replicated."`, while `SetHighlightedTiles` (`:377`) says
  `"Highlighting tiles is server-only"`. The message that fires does not name the function that failed.
- **Why**: The whole point of `ensureMsgf` over `ensure` is that the log tells you where you are. A wrong name
  costs more than no name.
- **What to change**: One shared macro or helper — `GEO_ENSURE_SERVER(this)` using `__FUNCTION__` — so the message
  is generated, not typed. This also folds into the cross-cutting `ensureMsgf` pass below.

### 9.6 `HighlightTiles` has a dead local and re-runs the guards it just passed

- **What**: `:355-363`:
  ```cpp
  if (Radius <= 0.f)
  {
      TArray<int32> Indices;      // never used
      FIntPoint Tile;
      if (GetTileUnderLocation(Location, Tile)) { HighlightTile(Requester, Tile); }
  }
  ```
  `Indices` is declared and never touched, and `HighlightTile` re-evaluates the same two `ensureMsgf` guards
  `HighlightTiles` evaluated two lines earlier.
- **Why**: Dead code plus a double guard; both branches of the `if` ultimately reach `SetHighlightedTiles`, which
  guards a *third* time.
- **What to change**: Delete `Indices`; have the `Radius <= 0` branch call `SetHighlightedTiles(Requester,
  {CoordToIndex[Tile]})` directly. Better: keep the guards only on `SetHighlightedTiles`, the one function that
  actually writes replicated state, and drop them from the two public wrappers.

### 9.7 `HighlightTile` indexes `CoordToIndex` with `operator[]`

- **What**: `:344` — `SetHighlightedTiles(Requester, {CoordToIndex[Tile]})`. Every other accessor in the file
  (`IsTileAlive:134`, `DestroyTiles:290`, `GetTileUnderLocation:129`) uses `Find` or `Contains` because an
  arbitrary `FIntPoint` need not be on the grid.
- **Why**: `TMap::operator[]` asserts on a missing key. `HighlightTile` is public API taking a raw `FIntPoint`, so
  an off-grid coordinate from a pattern or an ability is a crash, not a no-op.
- **What to change**: `if (int32 const* Index = CoordToIndex.Find(Tile)) { SetHighlightedTiles(Requester, {*Index}); }`
  — matching the file's own prevailing style.

### 9.8 `GetTilesIndexInRadius` is non-`const` and returns `int`

- **What**: `GeoHexArena.h:62` — `TArray<int> GetTilesIndexInRadius(FVector2D Center, float Radius);`. It mutates
  nothing (`:298-309`), and `int` should be `int32` per UE convention; every neighbouring index in the file is
  `int32`, including the `for (int const Index : ...)` at `:317` that consumes it.
- **Why**: Same issue as 3.8 — a getter that isn't `const` can't be called from the `const` accessors around it,
  and the `int`/`int32` mix is the only place in the class that breaks the convention.
- **What to change**: `TArray<int32> GetTilesIndexInRadius(FVector2D Center, float Radius) const;`.

### 9.9 `DestroyTiles` and `DestroyTilesInRadius` are the same function keyed differently

- **What**: `:282-296` and `:311-323` both guard server-only, loop setting `TileStates[Index].bAlive = 0`, and
  call `ApplyTileVisuals()`. One takes coordinates and resolves them through `CoordToIndex`; the other takes a
  centre and radius and resolves them through `GetTilesIndexInRadius`.
- **Why**: Small, but it is the third place in this file that pairs a mutation with a manual `ApplyTileVisuals()`
  call — and forgetting that call is a silent visual desync rather than an error.
- **What to change**: Have `DestroyTilesInRadius` convert to indices and call a single private
  `DestroyTileIndices(TConstArrayView<int32>)` that owns the guard, the loop and the `ApplyTileVisuals()`. Both
  public entry points then become two lines.

---

## 10. `GameClasses/`

### 10.1 `UGeoGameInstance` has a `GetSessionInterface()` helper and then ignores it four times

- **What**: `GeoGameInstance.cpp:261-265` is exactly the helper the file needs:
  ```cpp
  IOnlineSessionPtr UGeoGameInstance::GetSessionInterface() const
  {
      IOnlineSubsystem* OnlineSubsystem = Online::GetSubsystem(GetWorld());
      return OnlineSubsystem ? OnlineSubsystem->GetSessionInterface() : nullptr;
  }
  ```
  `LeaveSessionAndReturnToMenu`, `OnDestroySessionComplete`, `QuitGame` and `OnDestroySessionForQuitComplete` use
  it. `CreateAdvancedSession` (`:61-70`), `OnCreateSessionComplete` (`:102-110`) and `OnJoinSessionComplete`
  (`:152-179`) each re-derive it by hand instead — the last one twice in the same function (`:152` and `:174`).
- **Why**: Half the file uses the helper and half doesn't, so the null-handling policy differs per function: the
  helper returns null silently, the hand-rolled copies variously `ensureMsgf`, `UE_LOG(Error)`, or just skip.
- **What to change**: Route all seven through `GetSessionInterface()`. `OnJoinSessionComplete` collapses from
  three interface lookups and four early-returns to one.

### 10.2 `"GameSession"` is spelled as a literal six times

- **What**: `FName(TEXT("GameSession"))` appears at `:90`, `:208`, `:218`, `:237`, `:245` and inside the delegate
  handling. The engine already defines `NAME_GameSession` for exactly this.
- **Why**: A session name that only matches by string equality, typed six times, with no compiler check.
- **What to change**: Use `NAME_GameSession` (or one `static FName const GeoSessionName`) everywhere.

### 10.3 Session teardown is written twice for two different endings

- **What**: `LeaveSessionAndReturnToMenu` + `OnDestroySessionComplete` (`:199-231`) and `QuitGame` +
  `OnDestroySessionForQuitComplete` (`:234-258`) are the same four-step shape — check session state, either act
  now or register a destroy-complete delegate, clear the handle in the callback, act. They differ only in the
  terminal action (`OpenLevel` vs `QuitGame`) and in one extra `MainMenuMap` guard.
- **Why**: Four functions to express "destroy the session, then do X". Adding a third ending (return to lobby)
  means a fifth and sixth function.
- **What to change**: One private `DestroySessionThen(TFunction<void()> OnDone)` that handles the no-session case
  by calling `OnDone` immediately. The two public functions become three lines each and the two callbacks vanish.

### 10.4 `JoinAdvancedSession` is two lines of code and eighteen of commented-out code

- **What**: `:123-147` — the live body is three lines (wrap in `FBlueprintSessionResult`, call
  `BP_JoinAdvancedSession`); the rest is a commented-out C++ implementation, plus `OnJoinSessionComplete`
  (`:150-196`, 47 lines) which is that implementation's callback and therefore currently **unreachable**.
- **Why**: 65 lines of the file are dead. Git already holds the history, so the comment block is storage the
  version control system does better.
- **What to change**: Delete the comment block. Then either delete `OnJoinSessionComplete` too, or — if the C++
  path is meant to come back — uncomment it and delete the Blueprint detour. Do not leave both.

### 10.5 `UGeoGameInstance`'s ensure messages name a different class

- **What**: `:62`, `:67` — `ensureMsgf(..., TEXT("UGeoCreateServerWidget: Online subsystem not available"))`, inside
  `UGeoGameInstance::CreateAdvancedSession`. `:114` and `:118` log `"UGeoCreateServerWidget: ..."` too.
- **Why**: Same species as 9.5 — the message that fires names the class the code was pasted from, so the log
  points at the wrong file.
- **What to change**: Part of the cross-cutting `ensureMsgf` pass: generate the prefix from `__FUNCTION__` rather
  than typing a class name.

### 10.6 Debug on-screen messages are not guarded out of shipping

- **What**: `:93-94` and `:100` call `GEngine->AddOnScreenDebugMessage` unconditionally in session-creation paths.
- **Why**: Every other debug display in the codebase is behind `#if !UE_BUILD_SHIPPING` or a `CVar` —
  `GeoPlayerController.cpp:25-64`, `GeoHUD.cpp:552`, `GeoCombatStatsSubsystem.cpp:10-19`,
  `GeoGameCamera.cpp:199`. These two are the exception, and one of them prints the network version on every
  server creation in a shipped build.
- **What to change**: Delete them, or wrap in `#if !UE_BUILD_SHIPPING` to match the convention.

### 10.7 `GetLocalGeoPlayerController` null-checks the same pointer twice

- **What**: `GeoPlayerController.cpp:227-236`:
  ```cpp
  if (APlayerController* PlayerController = Iterator->Get())
  {
      if (PlayerController && PlayerController->IsLocalController() && ...)
  ```
- **Why**: The `if`-init already proved it non-null. Minor, but it is the kind of line that makes a reader look
  for the case the author was worried about.
- **What to change**: Drop the inner `PlayerController &&`. (Same function: `CastChecked` after an explicit
  `IsA(StaticClass())` can be a plain `Cast`, or the `IsA` can go.)

### 10.8 `FInputModeGameOnly().SetConsumeCaptureMouseDown(false)` appears twice, explained once

- **What**: `:86` carries a two-line comment explaining that the default mode eats the click that re-acquires
  mouse capture, so ability clicks would be lost. `:301` (`ClosePauseMenu`) repeats the same expression with no
  comment.
- **Why**: The reason is non-obvious enough to have earned a comment; a second copy without it invites someone to
  "simplify" it back to plain `FInputModeGameOnly()` and silently break clicks after closing the pause menu.
- **What to change**: One private `SetGameplayInputMode()` holding both the expression and the comment.

---

## 11. `System/`

### 11.1 `UGeoActorPoolingSubsystem` leaves freshly spawned actors *in* the pool while handing them out

- **What**: `GeoActorPoolingSubsystem.cpp:33-40` — when the pool is empty, `PopWithClass` spawns through the
  member helper:
  ```cpp
  if (!Actor) { ...; Actor = SpawnActor(Class, Params); }
  ```
  and `SpawnActor` (`:111-127`) ends with `Pool.FindOrAdd(Class).Add(NewActor); return NewActor;`. Nothing pops it
  back off. So the actor is returned to the caller as live **and** left sitting in the free list.
- **Why**: The next `PopWithClass` for that class hands the same actor to a second caller — two projectiles
  sharing one actor. `ReleaseActor` (`:146`) then `Add`s it a third time, so the free list accumulates duplicates
  of every actor that was ever spawned on demand rather than pre-spawned. This is a correctness bug, not a
  simplification; it surfaced because the same `Add` is written for two different purposes.
- **What to change**: Split the two jobs. `SpawnActor` should not touch `Pool` — let `PreSpawn` do
  `Pool.FindOrAdd(Class).Add(SpawnActor(...))` explicitly, and let `PopWithClass` return its spawn directly. The
  pool array then means exactly one thing: "actors that are free".
  **Verify with a PIE test** (spawn more projectiles than were pre-spawned, then check for shared/duplicated
  actors) before and after — I have not run this.

### 11.2 `PreSpawn`'s three ensures do not guard anything

- **What**: `:74-77`:
  ```cpp
  ensureMsgf(World, TEXT("World is invalid"));
  ensureMsgf(Class, TEXT("Class is invalid"));
  ensureMsgf(Count > 0, TEXT("Count must be greater than 0"));
  ```
  Execution continues regardless. With `Class == nullptr` it reaches `Pool.FindOrAdd(nullptr)` and then
  `SpawnActor(nullptr, ...)`, whose own failure ensure formats `*Class->GetName()` (`:118`) — a null dereference
  *inside the error message*.
- **Why**: The ensure fires, then the process crashes on the next line anyway, and the crash callstack points at
  the message formatting rather than the caller who passed null.
- **What to change**: `if (!ensureMsgf(Class && Count > 0, ...)) { return; }`. Part of the cross-cutting pass.

### 11.3 Failure is reported twice, two different ways

- **What**: `:12-16` and `:129-136` both do `UE_LOG(Error, ...)` and/or `ensureMsgf(false, TEXT(<same string>))`.
  `ensureMsgf(false, ...)` asserts a condition that is false by construction rather than testing the thing that
  actually went wrong.
- **What to change**: `if (!ensureMsgf(Class, TEXT("PopWithClass called with invalid Class"))) { return nullptr; }`
  and the same for `IsValid(Actor)` in `ReleaseActor`, dropping the paired `UE_LOG`.

### 11.4 `ReportDamageDealt` and `ReportHealingDealt` are one function written twice

- **What**: `GeoCombatStatsSubsystem.cpp:89-102` and `:118-131` are identical bar three field names
  (`TotalDamageDealt`/`SmoothedDPS`/`DamageBurst` vs `TotalHealingDealt`/`SmoothedHPS`/`HealingBurst`).
- **What to change**: One private
  `void ReportRate(AGeoPlayerState*, float Amount, float FActorCombatStats::* Total, float FActorCombatStats::* Smoothed, FBurstTracker FActorCombatStats::* Burst)`,
  or simply have both call a shared body that takes the three member pointers. `ReportDamageReceived` stays as it
  is — it genuinely only touches one field.

### 11.5 `FindOrAddStats` takes a `CurrentTime` it never uses

- **What**: `:70-79` — the parameter is unused; the body only tests `StatsPerActor.IsEmpty()` and
  `IsMatchInProgress()`. All three call sites compute and pass `CurrentTime` anyway.
- **Why**: Same species as 1.1 — a parameter that looks load-bearing and isn't.
- **What to change**: Drop the parameter.

### 11.6 `SetDebugCombatStats` takes nine positional floats

- **What**: `:60` passes nine literal zeroes; `:179-182` passes nine expressions in an order that has to match
  the declaration exactly — smoothed DPS, smoothed HPS, two burst maxima, two averages, two totals, damage
  received. Nothing but the parameter order distinguishes damage from healing.
- **Why**: Transposing any two of the nine compiles cleanly and produces a wrong-but-plausible HUD. The `:60` call
  in particular is nine zeroes that mean "clear everything" and would still compile if a field were added.
- **What to change**: Pass a struct — `FActorCombatStats` already holds most of it, so
  `SetDebugCombatStats(FGeoDisplayStats const&)` with named fields, and the reset call becomes
  `SetDebugCombatStats({})`.

---

## 12. `World/`

### 12.1 `AGeoGameCamera::Tick` is 125 lines doing five separate jobs

- **What**: `GeoGameCamera.cpp:131-255` gathers local players, decides spectate mode, averages the follow target,
  computes the zoom, draws debug, clamps to the volume bounds, then interpolates and applies. The bounds block
  alone (`:213-245`) is 33 lines of aspect-ratio and half-extent maths inline in the tick.
- **Why**: Same species as 3.10 (`PushAway`). Every one of the five stages is individually testable and none of
  them can be read in isolation today.
- **What to change**: Extract `GatherLocalPlayers(...)`, `UpdateZoom(FarthestDistance, DeltaTime)` and
  `ClampToBounds(FVector2D Target) const`. `Tick` becomes about fifteen lines that read as the sequence of
  stages, which is what the function is actually about.

### 12.2 `GetSpectateMoveInput` dereferences a character the caller allows to be null

- **What**: `:115` opens with `LocalCharacter->GetGeoInputComponent()->MoveAction` — two dereferences, no checks —
  while the caller (`:177`) passes `FirstLocalCharacter`, which `:148-153` explicitly allows to be null
  (`Cast<AGeoCharacter>(Pawn)` result stored unconditionally).
- **Why**: It is currently safe, but only via a two-step argument: a null `Character` means `:154` adds that pawn
  to `LivingPlayers`, so `bAllDead` is false and the spectate branch never runs. That invariant is nowhere stated,
  and it breaks the moment the `LivingPlayers` condition changes.
- **What to change**: Either guard at the top of `GetSpectateMoveInput` (`if (!LocalCharacter || !LocalCharacter->GetGeoInputComponent()) { return FVector2D::ZeroVector; }`)
  or, better, only assign `FirstLocalCharacter` when the cast succeeds and take it as a reference. A guard is one
  line and removes the need for the reader to reconstruct the argument.

---

## 13. `GeoTrinityEditor/Tool` — the builder utils

### 13.1 The fully-qualified function name is typed as a string literal 50 times

- **What**: Every entry point in `GeoStateTreeBuilderUtil.cpp` passes its own name to the three helpers as a
  literal:
  ```cpp
  UStateTreeEditorData* EditorData = GetEditorData(StateTree, TEXT("UGeoStateTreeBuilderUtil::ClearTransitions"));
  if (!EditorData) { return; }
  UStateTreeState* State = FindState(EditorData, StateName, TEXT("UGeoStateTreeBuilderUtil::ClearTransitions"));
  if (!State) { return; }
  ...
  CompileAndSave(StateTree, TEXT("UGeoStateTreeBuilderUtil::ClearTransitions"));
  ```
  A repo count: **50** such literals in `GeoStateTreeBuilderUtil.cpp` and **14** in `GeoWidgetBuilderUtil.cpp`.
  `BindConditionPropertyToPropertyFunction` alone types its own name **seven** times. Meanwhile `__FUNCTION__`
  appears exactly **twice** in the entire `Source` tree.
- **Why**: Every rename of a builder function silently leaves 3–7 stale strings behind, and the whole point of the
  `CallerName` parameter is to make the error message say where you are. `AddTransition` (`:302`, `:304`) already
  shows the drift starting — it hand-appends `" (source)"` / `" (target)"` because the literal alone was not
  enough.
- **What to change**: Drop the `TCHAR const* CallerName` parameter from `GetEditorData`, `FindState` and
  `CompileAndSave` and use `__FUNCTION__` inside them (or a `UE_LOG`-style macro that captures it). That removes
  64 string literals and one parameter from three helpers, and the messages become correct by construction.

### 13.2 The full preamble is 12 lines repeated in 15 functions

- **What**: Beyond the literals, the shape itself repeats: get editor data → bail; find state → bail;
  `EditorData->Modify()`; `State->Modify()`; *do the one interesting thing*; `CompileAndSave`. In most of the 15
  entry points the "one interesting thing" is two to five lines out of twenty-five.
- **Why**: The signal-to-boilerplate ratio makes it hard to see what each utility actually does, and it is why
  13.3 and 13.5 below went unnoticed.
- **What to change**: One scoped helper that owns the whole envelope:
  ```cpp
  // Resolves editor data + state, brackets the edit in Modify(), and compiles+saves on scope exit.
  bool WithState(UStateTree* Tree, FName StateName, TFunctionRef<void(UStateTreeEditorData&, UStateTreeState&)> Edit);
  ```
  Each entry point becomes its two interesting lines inside a lambda. Roughly 200 lines removed.

### 13.3 `AddFireAbilityStateByTagName` ensures the tag, then uses it anyway

- **What**: `:204-212`:
  ```cpp
  FGameplayTag Tag = FGameplayTag::RequestGameplayTag(AbilityTagName, false);
  ensureMsgf(Tag.IsValid(), TEXT("... — tag '%s' not found"), *AbilityTagName.ToString());
  AddFireAbilityState(StateTree, StateName, Tag, ParentStateName, InsertIndex);
  ```
  There is no `if (!...) { return; }`, so a mistyped tag name creates and *compiles and saves* a fire-ability state
  whose `AbilityTag` is empty.
- **Why**: Same species as 11.2 — an ensure that reports but does not guard. Here the consequence is a corrupted
  asset written to disk, which is worse than a crash because it is silent.
- **What to change**: `if (!ensureMsgf(Tag.IsValid(), ...)) { return; }`.

### 13.4 "Resolve a gameplay tag by name or bail" is written five times

- **What**: The same four-line block appears in `AddFireAbilityStateByTagName` (`:208`),
  `ReplaceFireAbilityTagInState` (`:231`), `AddTransition` (`:315`), `AddSendEventAfterNCyclesTask` (`:483`) and
  `SetRequiredEventToEnter` (`:545`) — five different message strings for one operation.
- **What to change**: `static bool ResolveTag(FName Name, FGameplayTag& OutTag)` with the message generated from
  `__FUNCTION__` per 13.1. It also fixes 13.3 for free, because the helper returns a bool the caller must use.

### 13.5 `BindConditionPropertyToPropertyFunction` modifies before it transacts

- **What**: `:406-419` calls `AddFunctionBinding` and `AddPropertyBinding` — both mutate `EditorData` — and only
  then, at `:421`, calls `EditorData->Modify()`. Every other function in the file calls `Modify()` before
  mutating.
- **Why**: `Modify()` is what records the pre-change state for undo. Called after the fact it records the
  *post*-change state, so undo does nothing (or restores the wrong snapshot). This is the one place in the file
  that gets the ordering wrong, which is exactly what 13.2's boilerplate hides.
- **What to change**: Move `EditorData->Modify()` above `:406`. Folding the envelope into the 13.2 helper makes
  the ordering impossible to get wrong.

### 13.6 Eight parameters, five of them consecutive `FName`s

- **What**: `:359-362` —
  `(UStateTree*, FName StateName, int32 ConditionIndex, FName ConditionPropertyName, FName PropertyFunctionStructName, FName FunctionOutputPropertyName, FName FunctionInputPropertyName, FName ContextClassName)`.
- **Why**: Same species as 11.6 — transposing any two of the five compiles cleanly and fails at runtime with a
  "not found" ensure that names the wrong thing.
- **What to change**: Group the last four into a small `FGeoPropertyFunctionBinding` struct with named fields. As
  a Blueprint-callable editor util it also gives the caller named pins instead of five identical `FName` boxes.

### 13.7 Three hand-written recursive walkers over the same tree

- **What**: `FindStateRecursive` (`:20-38`), `RemoveStateRecursive` (`:40-55`) and `LogStatesRecursive`
  (`:84-128`) each re-implement the descent through `State->Children`. `RemoveStateRecursive` additionally
  re-tests `States[i]` on both `:44` and `:49`.
- **Why**: Low priority — they are short and correct — but a fourth walker is inevitable and this is where it
  will be pasted from.
- **What to change**: One `ForEachStateRecursive(States, TFunctionRef<bool(UStateTreeState&)>)` returning early
  when the visitor returns true. Optional; take it only if a fourth walker gets added.

---

## 14. `AbilitySystem/Abilities/Pattern` — `UPattern` / `UTickablePattern`

### 14.1 `FillCueParam` divides by `StartDelay`, which is documented to be zero sometimes

- **What**: `Pattern.cpp:109`:
  ```cpp
  // Hack Normale to pack timing info into (start delay, elapsed time before stating, ratio)
  CueParams.Normal = FVector(StartDelay, TravelTime, 1 - ((StartDelay - TravelTime) / StartDelay));
  ```
  `StartDelay == 0` is an explicitly supported case — `:59-60` comments "A pattern with no wind-up at all is not
  late, it is meant to fire on arrival" and branches on exactly that.
- **Why**: With `StartDelay == 0` the third component is `±inf` or `NaN`, and it is fed straight into a Gameplay
  Cue as `CueParams.Normal.Z`, where a NaN typically propagates into Niagara as an invisible or exploded effect —
  a visual bug with no error and no obvious cause.
- **What to change**: `StartDelay > 0.f ? 1.f - ((StartDelay - TravelTime) / StartDelay) : 1.f` — a
  zero-wind-up pattern is by definition fully wound up at spawn.

### 14.2 Two `ensureMsgf(false, ...)` that dereference the thing they are reporting on

- **What**: `:91` — `ensureMsgf(false, TEXT("Pattern Instigator %s has no ASC !"), *StoredPayload.Instigator->GetName())`
  is reached when `GetGeoAscFromActor(StoredPayload.Instigator)` returned null, which includes the case where
  `Instigator` itself is null — and then the message formatting dereferences it. `:16` has the same shape.
- **Why**: Same species as 11.2 — the crash lands in the error handler rather than at the fault, so the callstack
  points at the wrong place.
- **What to change**: `if (!ensureMsgf(IsValid(InstigatorASC), TEXT("Pattern %s: instigator has no ASC"), *GetName())) { return; }`
  — test the real condition and do not format the null pointer.

### 14.3 The "can I play this montage locally" test is written three times

- **What**: `IsValid(AnimMontage) && !GeoLib::IsDedicatedServer(GetWorld()) && IsValid(AnimInstance)` appears at
  `:68` (with `bRendersLocally` standing in for the middle term), `:116` and `:150`.
- **Why**: The middle term carries a two-line comment at `:49-51` explaining why it is `IsDedicatedServer` and not
  `!IsServer` — the listen-server host must render. That reasoning lives at one of the three sites.
- **What to change**: `bool UPattern::CanPlayMontageLocally(UAnimInstance const* AnimInstance) const` holding all
  three terms and the comment. Three call sites become one condition each.

### 14.4 `UPattern` and `UTickablePattern` track "already running" two different ways

- **What**: `UPattern::InitPattern` (`:38-43`) guards on `bPatternIsActive`; `UTickablePattern::InitPattern`
  (`:192-197`) guards on `TimeSyncTimerHandle.IsValid()` — with the *same* error string copy-pasted, and calling a
  different overload of `EndPattern`.
- **Why**: Same species as 3.7 (`bBlinking` vs `BlinkTimerHandle`): one concept, two storage locations, and they
  can disagree — `EndPattern(true)` from the base clears `bPatternIsActive` but the derived class's timer handle
  is only cleared in its own override.
- **What to change**: Keep `bPatternIsActive` as the single answer; have the derived `InitPattern` rely on
  `Super::InitPattern`'s guard and clear its timer in `EndPattern` only.

### 14.5 `ensureMsgf(false, ...)` is used 32 times across 19 files

- **What**: A repo-wide count: **32** occurrences of `ensureMsgf(false, ...)` in 19 files — `Pattern.cpp`,
  `GeoActorPoolingSubsystem.cpp`, `GeoGameplayAbility.cpp`, `PatternAbility.cpp`, `GeoDeployAbility.cpp`,
  `GeoAutomaticFireAbility.cpp`, `GeoEnemyAIController.cpp`, `AbilityInfo.cpp`, `UGeoGameplayLibrary.cpp`,
  `HudFunctionLibrary.cpp` and others.
- **Why**: `ensureMsgf(false, ...)` asserts a constant. It cannot tell you which condition failed, it is
  unconditionally hit rather than testing anything, and in several cases (11.2, 13.3, 14.2) the surrounding code
  then continues or crashes formatting the message. The codebase's own style prescribes testing the condition.
- **What to change**: Mechanical pass converting each to `if (!ensureMsgf(<the real condition>, ...)) { return ...; }`.
  Fold into the cross-cutting `ensureMsgf` work below — this is the same pass, and 32 sites is a concrete count
  to work through.

---

## 15. `AI/` — `AGeoEnemyAIController`

### 15.1 `SetGenericTeamId` and `GetGenericTeamId` are the same body twice

- **What**: `GeoEnemyAIController.cpp:29-40` and `:42-53` both do:
  ```cpp
  IGenericTeamAgentInterface* TeamAgentInterface = Cast<IGenericTeamAgentInterface>(GetPawn());
  if (!TeamAgentInterface)
  {
      ensureMsgf(GetPawn(), TEXT("No Pawn on %s"), *GetName());
      ensureMsgf(TeamAgentInterface, TEXT("No IGenericTeamAgentInterface on %s"), *GetName());
      return ...;
  }
  ```
  — identical down to both ensure strings.
- **Why**: The second `ensureMsgf` asserts a variable the enclosing `if` has already proven false, so it always
  fires; it exists only to distinguish "no pawn" from "pawn without the interface", which one ensure with a
  ternary message would do better.
- **What to change**: `IGenericTeamAgentInterface* GetPawnTeamAgent() const` holding the cast and the diagnostic.
  Both accessors become two lines.

### 15.2 `ClearAggro` does the opposite of what its name suggests, and `TriggerAggro` calls it

- **What**: `:215-222`:
  ```cpp
  bAggroed = true;
  ClearAggro();
  ```
  `ClearAggro` (`:87-99`) stops the `AggroCheckTimer` and unbinds the damage delegate — i.e. it means "stop
  *watching* for aggro triggers", which is the right thing to do once aggroed, but reads as "un-aggro" next to
  `bAggroed = true`.
- **Why**: A reader hitting those two lines has to open `ClearAggro` to establish that they do not contradict each
  other. It is also called from `ResetAI` (`:57`) and `InitializeAggro` (`:72`), where "clear" *does* mean reset —
  so the name means two different things depending on the caller.
- **What to change**: Rename to `StopAggroWatch()`. No behaviour change; the `bAggroed = true; StopAggroWatch();`
  pair then reads correctly.

### 15.3 `OnPossess` and `ResetAI` duplicate the pawn-cast-and-initialise sequence

- **What**: `:55-68` and `:110-123` are the same four steps (cast to `AEnemyCharacter`, ensure, `InitializeStateTree`,
  `InitializeAggro`); `ResetAI` just prefixes `ClearAggro(); bAggroed = false;`.
- **What to change**: `ResetAI` becomes `ClearAggro(); bAggroed = false; OnPossess(GetPawn());` — or, cleaner,
  both call one private `InitializeForPawn(APawn*)`.

### 15.4 Two ensures name `OnPossess` from inside other functions

- **What**: `:80` and `:94` — `ensureMsgf(false, TEXT("GeoEnemyAIController::OnPossess — boss has no GeoAbilitySystemComponent"))`
  inside `InitializeAggro` and `ClearAggro`.
- **Why**: Same species as 9.5, 10.5 and 13.1 — the message names where the code was pasted from. `ClearAggro` is
  also called on every `TriggerAggro`, so this fires from a path `OnPossess` is nowhere near.
- **What to change**: Part of the cross-cutting `ensureMsgf` pass; use `__FUNCTION__`.

---

## 16. `GeoTrinityUI/HUD` — `UGeoAbilitySlotWidget`

### 16.1 `NativeTick` dereferences `GetAbilityCDO` unchecked, every frame

- **What**: `GeoAbilitySlotWidget.cpp:171-172`:
  ```cpp
  if (Remaining <= 0.f && HUD->IsAbilityActive(AbilityTag)
      && !GeoASLib::GetAbilityCDO(AbilityTag)->GetClass()->IsChildOf(UGeoAutomaticFireAbility::StaticClass()))
  ```
  `GetAbilityCDO` returns null for a tag with no catalog entry, and the result is dereferenced immediately.
- **Why**: Third instance of the same shape as 8.3 and 9.7, and this one is inside a per-frame tick on every
  ability slot, so a single mis-tagged ability crashes the client on the frame it activates.
- **What to change**: Hoist to a local with a null check, or better — the question being asked is "is this an
  automatic-fire ability", which the entry already knows at build time. Cache it as a `bool` on
  `FGeoAbilityBarEntry` when the bar is built and drop the per-frame CDO lookup entirely.

### 16.2 The "apply the displayed entry" block is written twice

- **What**: `InitSlot:29-50` and `SelectDisplayedEntry:78-87` both set the icon brush from
  `DisplayedEntry().Icon` and set `CountText`'s visibility from `DisplayedEntry().bIsDeployable`, then call
  `RefreshDeployCount()`.
- **What to change**: One private `ApplyDisplayedEntry()`; `InitSlot` calls it after seeding `Entries`, and
  `SelectDisplayedEntry` calls it after the index changes.

### 16.3 `RefreshDeployCount` exists, and `NativeTick` inlines it anyway

- **What**: `:118-129` is `RefreshDeployCount()`. `:153-161` repeats its body inside the tick — because the tick
  also needs `CurrentStacks` for the gray-out decision at `:162-165`, which the void helper does not hand back.
- **Why**: Two copies of the same `GetDeployCountForAbility` + `SetText` pair, kept in sync by hand, purely
  because of a return type.
- **What to change**: `int32 RefreshDeployCount()` returning the current stacks. The tick's block becomes
  `if (RefreshDeployCount() > 0) { Remaining = 0.f; }`.

### 16.4 The "hide the countdown" block appears three times

- **What**: `:41-44`, `:178-181` and `:191-194` all read
  `if (CountdownText && CountdownText->GetVisibility() != ESlateVisibility::Hidden) { CountdownText->SetVisibility(...); }`
  (the first without the visibility test).
- **What to change**: `void SetCountdownVisible(bool bVisible)` holding the null check and the
  already-in-that-state short-circuit. Three sites become one call each, and the `InitSlot` copy stops being the
  odd one out.

---

## Cross-cutting themes

Three patterns showed up in every folder audited. Each is one pass, not one fix per site.

### A. The `ensureMsgf` guard form

Findings 1.3, 4.2, 5.1, 6.2, 9.5, 11.2, 11.3, 13.3, 14.5, 15.1 are all this. `AI/CodingStyle.md` prescribes a
single form:

```cpp
if (!ensureMsgf(Condition, TEXT("..."))) { return; }   // when execution must stop
ensureMsgf(Condition, TEXT("..."));                     // when it must not
```

The codebase also uses `if (!X) { ensureMsgf(X, ...); return; }` (double-evaluates, asserts a condition already
proven false), `ensureMsgf(X, ...); if (!X) { return; }`, and — **32 times across 19 files** —
`ensureMsgf(false, ...)`, which asserts a constant and so cannot say what actually failed. A repo-wide grep finds
**163** `ensureMsgf` statements across 52 files; the non-conforming forms are concentrated in
`GeoTrinityUI/HUD/Menu` (~50, all removable outright per 5.1) and the ability-system files.

Order: do 5.1 first (deletes most of them rather than rewriting them), then the 32 `ensureMsgf(false, …)` sites,
then the remainder mechanically.

### B. Diagnostic messages that name the wrong function

Findings 9.5, 10.5, 13.1, 15.4 — and the general case. Messages are hand-typed class/function names that were
copy-pasted with the code around them, so the log points at the file the code came *from*:
`UGeoGameInstance` reporting as `UGeoCreateServerWidget`, `InitializeAggro` and `ClearAggro` both reporting as
`OnPossess`, `SetHighlightedTiles` reporting as `HighlightTile`. `GeoStateTreeBuilderUtil.cpp` takes it furthest
with **50** such literals, passing its own name to helpers as a parameter.

Meanwhile `__FUNCTION__` appears exactly **twice** in the entire `Source` tree. Adopting it — directly, or via a
small `GEO_ENSURE(cond)` macro that captures it — removes ~70 string literals and makes the class of bug
impossible.

### C. A careful guard immediately followed by an unchecked dereference

Findings 8.3, 9.2, 9.7, 11.2, 13.3, 14.2, 16.1. The recurring shape is a function that null-checks two or three
things at the top and then, a few lines later, dereferences a fourth without checking:
`Cast<IAbilitySystemInterface>(Boss)->GetAbilitySystemComponent()`, `SpawnActor(...)` used directly,
`CoordToIndex[Tile]`, `GetAbilityCDO(Tag)->GetClass()`. Twice (11.2, 14.2) the dereference is *inside the
`ensureMsgf` message*, so the crash lands in the error handler and the callstack points away from the fault.

These are one line each and independent of every other item — a good first commit.

---

## Summary — suggested order of work

Ranked by (lines removed) × (risk of the duplication causing a future bug), lowest-risk first within a tier.

Three things are worth doing before anything else, because they are behaviour bugs rather than tidiness, and none
of them depends on any other item.

| # | Item | Effort | Payoff | Status |
|---|---|---|---|---|
| **0a** | **11.1 — pooled actors are handed out while still in the free list** | small | **two callers can share one actor**; needs a PIE check | done, PIE check owed |
| **0b** | 7.1 — `CreateAbilityPayload` draws two seeds | one token | latent client/server desync | done |
| **0c** | 14.1 — `FillCueParam` divides by a `StartDelay` that is documented to be 0 | one ternary | NaN into a Gameplay Cue | done |
| 1 | Theme C — the seven unchecked dereferences (8.3, 9.2, 9.7, 11.2, 13.3, 14.2, 16.1) | one line each | seven crashes become no-ops | done |
| 2 | 6.1 — delete dead `MakeGeoEffectContext` | trivial | removes a dangling-pointer trap | done |
| 3 | 13.5 — `Modify()` after the mutation it is supposed to record | move one line | editor undo actually works | done |
| 4 | 5.1 + 5.2 — menu-widget ensure blocks and `NativeDestruct` | mechanical, ~11 files | **~400 lines** | done |
| 5 | 13.1 + 13.2 — `__FUNCTION__` and one scoped envelope in the builder utils | mechanical | **~260 lines**, 64 literals | done |
| 6 | Theme A — the `ensureMsgf` pass, starting with the 32 `ensureMsgf(false, …)` | mechanical | consistency + real diagnostics | done |
| 7 | 3.1, 3.2, 4.4, 3.8, 8.6, 9.6, 9.8, 10.7 — guard/dead-code/`const` cleanups | small | correctness + clarity | done |
| 8 | 10.4 — delete the 65 dead lines in `JoinAdvancedSession` / `OnJoinSessionComplete` | trivial | git already has them | done |
| 9 | 8.1 — one `FindSpecForTag` helper for the four HUD queries | small | ~70 lines | done |
| 10 | 1.x — `GeoAbilitySystemLibrary` overload and wrapper pruning | medium | ~80 lines, smaller API | done except 1.9 (second half) and 1.10 |
| 11 | 4.1–4.3, 6.2–6.5, 8.2, 9.1, 9.9, 10.1–10.3, 11.4–11.5, 13.4, 14.3, 15.1, 15.3, 16.2–16.4 — extract-shared-helper items | medium | **~400 lines** | done (also 4.5, 14.4) |
| 12 | 8.4 + 8.5 — collapse the two attribute-change fan-out mechanisms into one | medium | deletes `UnbindStatCallbacks` and two ASC delegates | **open** |
| 13 | 11.6 + 13.6 — replace long positional parameter lists with structs | small | removes two transposition traps | done |
| 14 | 2.1 + 2.2 + 2.3 — `FEffectData` hierarchy collapse | **large, needs CoreRedirects** | ~150 lines, best structural win | **open** |
| 15 | 2.5, 2.6, 3.10, 9.3, 12.1 — parser/traversal/long-function cleanups | medium | clarity | 2.6 / 3.10 / 9.3 done; **2.5 and 12.1 open** |
| — | 13.7 — three hand-written recursive StateTree walkers | small | one traversal instead of three | **open** |

Two findings that were reported but should **not** be applied as written:

- **3.4** is wrong. It says to delete `AGeoProjectile::GetPitch(FGeoSoundEntry const&)`. That function is `virtual`
  and `AGeoShieldBurstProjectile::GetPitch` overrides it to layer `BounceSoundSizePitchCurve` on top of `Super`;
  deleting it silently drops per-size bounce pitch. What was done instead: the virtual stays, and
  `UGeoSoundRowLibrary::ConfigureAudioComponent` grew a `float Pitch` parameter so the looping path still routes
  through it.
- **9.4** was judged not worth it on re-reading — see the status section at the top.

**Verification note**: the report itself was written without compiling. The applied work has been compiled
(Editor / DebugGame target, all three modules) except for the last batch, which is pending a build. The items
that still need more than a compile are listed under "Needs a runtime check" at the top of this file.

## Coverage

Audited folder by folder: `AbilitySystem/{Lib,Data,Components,Abilities/Base,Abilities/Pattern}`,
`Actor/{Projectile,Deployable}`, `Actor/GeoArena` + `GeoHexArena`, `Characters/`, `GameClasses/`, `System/`,
`World/`, `AI/`, `GeoTrinityUI/HUD` (menu and non-menu), `GeoTrinityEditor/Tool`.

Sampled rather than read line by line — the per-class ability folders
(`Abilities/{Boss,Circle,Square,Triangle,Common,Damaging}`) and the StateTree task classes. They were scanned for
the patterns this report already names (`ensureMsgf(false, …)`, duplicated guards, copy-pasted messages) and the
counts quoted in the cross-cutting themes include them, but no per-file findings were written up. The beam family
in particular (`GeoChannelBeamAbility` → `GeoMoiraBeamAbility` / `GeoSacrificeBeamAbility` /
`GeoChargeBeamAbility`) already has a proper `Tick`/`TickBeam` base-class split and did not look like a
duplication candidate on inspection.
