# Ability Bar HUD — abilities, cooldowns & deployable counts

## STATUS (2026-06-05) — assets built; ⚠️ uncommitted C++ awaiting editor-closed rebuild (see NEXT STEPS #0)

**Done (compiles clean, GeoTrinityEditor):**
- `AGeoHUD` (`GeoHUD.h/.cpp`): `FGeoAbilityBarEntry`; `GetAbilityBarEntries` / `GetAbilityCooldown` / `GetDeployCountForAbility`; `InitAbilityBar` + `RefreshAbilityBar` BlueprintImplementableEvents; **tagless** `OnPlayerDeployCountChanged` ping (public) + `HandleDeployCountChanged`; `BindToPawn(APlayableCharacter*)` does the pawn-dependent deploy binding + initial `InitAbilityBar`, called from `AGeoPlayerState::OnPlayerPawnSet` (NOT InitOverlay — pawn isn't guaranteed there). `RefreshAbilityBar` called from `OnRep_PlayerClass`.
- `AbilityInfo.h`: `bShowDeployCount`. `GeoDeployAbility.h`: public `GetDeployableActorClass()`.
- Widget classes: `UGeoAbilitySlotWidget` (`HUD/GeoAbilitySlotWidget.h/.cpp`) — BindWidget `Icon`/`CooldownSweep`/`CountdownText`/`CountText`(opt) + `CooldownSweepMaterial` + MID `Fill` param; `NativeTick` early-outs when ready; `InitSlot`/`RefreshDeployCount`. `UGeoAbilityBarWidget` (`HUD/GeoAbilityBarWidget.h/.cpp`) — BindWidget `SlotBox`(HorizontalBox) + `SlotWidgetClass`; `BuildBar(AGeoHUD*)` clears+rebuilds, AddUniqueDynamic to ping.
- Shim split: `UGeoWidgetBuilderUtil` = generic primitives only (public static `BeginBuild`/`FinishBuild`/`ConstructRootPanel`; `SetRootPanel`, `SetImageRoot`, `SetImageRootFromMaterial`, `InspectWidgetBlueprint`). New `UGeoHudWidgetBuilderUtil` (`Tool/GeoHudWidgetBuilderUtil.h/.cpp`) = content builders `BuildAbilitySlotWidget(WBP, IconSize)` + `BuildChargeBeamGaugeWidget`. `charge_beam_gauge.py` updated to `GeoHudWidgetBuilderUtil`.
- Docs: generic-shim rule added to `AI/MCP/MCP_EditorUtility.md`.
- `AI/Python/ability_bar.py` written: builds `M_CooldownSweep` (UI/Translucent, `Fill` scalar → Step over atan2 angle), `WBP_AbilitySlot` (parent `GeoAbilitySlotWidget`, via `build_ability_slot_widget`), `WBP_AbilityBar` (parent `GeoAbilityBarWidget`, root HorizontalBox `Root` via `set_root_panel`). All under `/Game/HUD/AbilityBar`.

**DONE (compiled & live in editor):**
- `M_CooldownSweep`, `WBP_AbilitySlot`, `WBP_AbilityBar` all built (`ability_bar.py`). Inspected: slot = Overlay `Root` → `Icon`/`CooldownSweep`/`CountdownText`/`CountText`; bar = HorizontalBox **`SlotBox`** (matches BindWidget).
- `SetRootPanel` parameterized with `FName RootName = "Root"` (forwards to `ConstructRootPanel`); `ability_bar.py` passes `"SlotBox"` for the bar root. **(already compiled in — docstring confirmed live)**
- **Crash fix**: `BeginBuild` now empties `WidgetBlueprint->WidgetVariableNameToGuidMap`. The WBP compiler only auto-assigns GUIDs when that map is empty; on a *rebuild* of an existing asset the stale entries skipped that path, so freshly-named widgets got no GUID → `ensureAlwaysMsgf "Widget [X] was added but did not get a GUID"` (WidgetBlueprintCompiler.cpp:794). Generic fix → all content builders rebuild cleanly. **(live-compiled, verified working — no crash on re-run)**
- CDO refs wired + saved: `WBP_AbilitySlot.CooldownSweepMaterial = M_CooldownSweep`, `WBP_AbilityBar.SlotWidgetClass = WBP_AbilitySlot_C`.
- MCP UI docs updated (`MCP_UI.md`: rebuild GUID-map purge, parameterized root name, primitive-vs-content split, set-default-via-CDO, **driving a child widget from C++**; `MCP_EditorUtility.md`: added `UGeoHudWidgetBuilderUtil` row). DocStyle-compliant.

**⚠️ UNCOMMITTED C++ ON DISK — NEEDS A FULL EDITOR-CLOSED `GeoTrinityEditor` REBUILD BEFORE ANYTHING ELSE WORKS:**
Decision (user-approved): drive the bar from C++ via a `BindWidget`, NOT BP event-graph nodes. Changes written but **not yet compiled** (new UCLASS + removed BlueprintImplementableEvents ⇒ header regen, live coding can't do it):
- NEW `HUD/GeoOverlayWidget.h/.cpp` — `UGeoOverlayWidget : UGeoUserWidget`, `meta=(BindWidget) UGeoAbilityBarWidget* AbilityBar`, `BuildAbilityBar(AGeoHUD*)` → `AbilityBar->BuildBar(HUD)`.
- `GeoHUD.h/.cpp` — removed `InitAbilityBar`/`RefreshAbilityBar` BlueprintImplementableEvents; replaced with ONE C++ `BuildAbilityBar()` that casts `OverlayWidget`→`UGeoOverlayWidget` and forwards. `BindToPawn` now calls `BuildAbilityBar()`. Added `#include "HUD/GeoOverlayWidget.h"`.
- `GeoPlayerState.cpp` — `OnRep_PlayerClass` now calls `BuildAbilityBar()` (was `RefreshAbilityBar()`).
- (Build.cs unchanged — UMG/UMGEditor already deps.)

**NEXT STEPS (resume here):**
0. **Editor must be closed** → run full build (`AI/Commands.md` Bash build, or `build_project` target `GeoTrinityEditor`). Confirm `Result: Succeeded`. Relaunch editor, `status` → editor_online:true.
1. **Wire overlay (finish task #5)** via MCP/Python on `/Game/HUD/WBP_MainOverlay` (confirmed = HUD `OverlayWidgetClass`; HUD BP = `/Game/HUD/BP_GeoHudMain`; overlay root canvas = `CanvasPanel_21`, has ProgressBar_Health/Shield + AmmoText):
   - Reparent `WBP_MainOverlay` → `UGeoOverlayWidget`.
   - Add `WBP_AbilityBar` to `CanvasPanel_21`, **named `AbilityBar`** (must match BindWidget), anchored **bottom-center**.
   - `InspectWidgetBlueprint` to confirm bind OK + no compile error.
   - ⚠️ RISK A: Python reparent of a populated WBP may need an `FBlueprintEditorUtils::ReparentBlueprint`-style shim if `set_editor_property("parent_class", …)` + recompile is flaky.
   - ⚠️ RISK B: adding a child UserWidget as a *named BindWidget variable* is the same GUID-map territory that crashed us — inspect right after, watch for the GUID ensure. May need a small shim that constructs the sub-widget into the canvas with a name (mirrors `BuildAbilitySlotWidget`).
2. Populate `AbilityIcon` + `bShowDeployCount` on `UAbilityInfo` entries for all classes (deploy abilities → true). Find the data asset via `UGameDataSettings` (task #6).
3. PIE verify: icons row bottom-center, cooldown sweep + countdown, deploy badge decrements/increments, class-change rebuild, no idle tick cost (task #7).
4. Update remaining docs: `Tool/CLAUDE.md` (new `GeoHudWidgetBuilderUtil` + trimmed `GeoWidgetBuilderUtil` incl. RootName param + GUID-map purge), `HUD/CLAUDE.md` (add `UGeoOverlayWidget` + slot/bar widgets; **fix stale line ~37**: `InitAbilityBar`/`RefreshAbilityBar` BP events → C++ `BuildAbilityBar`).

## Context
The player HUD shows health/ammo/shield and a boss bar, but nothing tells the player **what abilities they have, which are on cooldown, and how many deployables remain**. Goal: a single compact row of icons at the **bottom-center** of the screen, one slot per granted (non-passive) player ability, each showing:
- the ability icon,
- a radial cooldown sweep + countdown number while on cooldown,
- (deployable abilities only) a remaining-count badge.

The data already exists: `UAbilityInfo` holds icons/input/tags, GAS exposes cooldown remaining/duration per ability, and `UGeoDeployableManagerComponent` broadcasts deploy count. So this is a thin C++ data-gathering layer on `AGeoHUD` plus a UMG ability-bar widget (BP), following the project's "screen-space UI = BP event forwarded from HUD" convention.

**Decisions:** Hybrid (C++ data, BP layout) · radial sweep + seconds text · event-driven + light tick only while a cooldown is active.

> **Revalidated 2026-06-05** against commits `f0549d5` + `eec95ac`. All relied-on APIs still exist. Three corrections folded in below:
> - Deployable cap check was split into `HasReachMaxLimit` (commit `f0549d5`); there is no per-class current-count getter, so `GetDeployCountForAbility` computes live count itself from the manager.
> - **Deploy-count class resolution: map via `DeployableSlots` keys / live buckets** (chosen) — `GetDeployCountForAbility` works purely off `UGeoDeployableManagerComponent`, not the ability CDO.
> - `ChangeClass()` insertion point confirmed: `PlayableCharacter.cpp:339` — sequence is `ClearPlayerClassAbilities → GiveStartupAbilities → ApplyClassData`; refresh hook goes right after `GiveStartupAbilities`.
> - Death path (`bIsDead`/`Revive`) added but does NOT re-grant abilities, so the only rebuild hook stays in `ChangeClass`. No change needed for death.

## Design overview
```
APlayableCharacter (granted ability specs + DeployableManagerComponent)
        │
AGeoHUD ── GetAbilityBarEntries()  → TArray<FGeoAbilityBarEntry>  (icon, tags, isDeployable)
        ├─ InitAbilityBar() / RefreshAbilityBar() BP events   (build slots)
        └─ OnPlayerDeployCountChanged delegate                (refresh count badges)
        │
WBP_AbilityBar (UGeoUserWidget) ── HorizontalBox of WBP_AbilitySlot
WBP_AbilitySlot ── Image(icon) + radial-fill material + countdown Text + count Text
        └─ Tick: only while on cooldown / deployable → query HUD helper
```

## C++ changes

### 1. Entry struct + gather helpers — `AGeoHUD` (`GeoHUD.h/.cpp`)
```cpp
USTRUCT(BlueprintType)
struct FGeoAbilityBarEntry
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FGameplayTag AbilityTag;
    UPROPERTY(BlueprintReadOnly) FGameplayTag InputTag;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<UTexture2D const> Icon = nullptr;
    UPROPERTY(BlueprintReadOnly) bool bIsDeployable = false;
};
```
Add to `AGeoHUD`:
- `UFUNCTION(BlueprintCallable) TArray<FGeoAbilityBarEntry> GetAbilityBarEntries() const;`
  - From `HudPlayerParams.GetGeoAbilitySystemComponent()`, iterate `GetActivatableAbilities()`; for each spec get the `UGeoGameplayAbility` CDO → `GetAbilityTag()`.
  - Skip passives (`UGeoGameplayAbility::IsPassive()`).
  - Match the global `UAbilityInfo` entry by tag (`GetAbilitiesForClass(PlayerClass)` / `GetAllPlayersAbilityInfos`) → pull `AbilityIcon`, `InputTag`, and the new `bShowDeployCount` → `bIsDeployable`.
- `UFUNCTION(BlueprintPure) void GetAbilityCooldown(FGameplayTag AbilityTag, float& OutRemaining, float& OutDuration) const;`
  - Find granted spec by tag; call native `Spec.Ability->GetCooldownTimeRemainingAndDuration(ActorInfo, OutRemaining, OutDuration)`.
- `UFUNCTION(BlueprintPure) void GetDeployCountForAbility(FGameplayTag AbilityTag, int32& OutCurrent, int32& OutMax) const;`
  - Read avatar's `UGeoDeployableManagerComponent` (`GetComponentByClass`). Resolve the bucket **off the manager, not the ability CDO** (chosen approach): match against `DeployableSlots` keys; `OutCurrent` = matching live `GetDeployables<AGeoDeployableBase>()` count, `OutMax` = the slot's cap if present else `GetMaxDeployables()`. Mirror the lookup logic already in `UGeoDeployableManagerComponent::HasReachMaxLimit` (`.cpp:20`) so behavior matches the deploy gate exactly.
  - Note: if a deploy ability has no `DeployableSlots` entry it shares the global pool — in that case `OutMax = GetMaxDeployables()` and `OutCurrent` = total live deployables. Designers wanting a per-ability badge must add a `DeployableSlots` entry for that class.

### 2. BP-facing wiring on `AGeoHUD`
- `UFUNCTION(BlueprintImplementableEvent) void InitAbilityBar();` — implemented in HUD BP, forwards to the overlay's ability-bar widget which calls `GetAbilityBarEntries()` and builds slots. Call from `InitOverlay()` after abilities are granted.
- `UFUNCTION(BlueprintImplementableEvent) void RefreshAbilityBar();` — rebuild after class change.
- `UPROPERTY(BlueprintAssignable) FOnDeployCountChanged OnPlayerDeployCountChanged;` (reuse signature from `GeoDeployableManagerComponent.h`). In `BindCallbacksToDependencies()`, find the avatar's `UGeoDeployableManagerComponent` and bind its `OnDeployCountChanged` → re-broadcast so BP count badges refresh without polling.

### 3. `FPlayersGameplayAbilityInfo` — `AbilityInfo.h`
Add `UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Cosmetic") bool bShowDeployCount = false;` so designers explicitly flag Mine/Turret deploy abilities. `GetAbilityBarEntries()` copies it into `FGeoAbilityBarEntry::bIsDeployable`.

### 4. Class switching — `PlayableCharacter.cpp`
`ChangeClass()` clears/re-grants abilities, so the bar must rebuild. After abilities are re-granted, reach the HUD (via `GetGeoPlayerController()`) and call `RefreshAbilityBar()`.

## Editor automation (MCP / Python) — corrected per `AI/MCP/*`
ALWAYS read `AI/MCP/CLAUDE.md` (+ `MCP_UI.md`, `MCP_Material.md`) before this work. Key constraints these docs impose, which change the approach:

- **Python cannot read/write a widget designer tree** (`MCP_UI.md`). Any multi-element slot layout must be built by a C++ `UEditorUtilityObject` shim — extend the existing `UGeoWidgetBuilderUtil` (`Source/GeoTrinity/Public/Tool/GeoWidgetBuilderUtil.h/.cpp`), which already owns `BeginBuild`/`FinishBuild` (validate → clear root → compile → save) and a `HorizontalBox`-free image/canvas pattern. Adding a new `UFUNCTION` to a shim **requires a full build with the editor closed** (`MCP/CLAUDE.md`).
- **No `M_CooldownRadial` with an `If` node** — `MaterialExpressionIf` scalar constants aren't Python-settable (`MCP_Material.md`). Reuse the existing **hard-edge fill pattern** (`Step`-based) from `M_ZoneIndicator` / `M_HealingZone` (`FillPercent` / `DurationPercent` scalar). For a *radial sweep* the same approach applies but compare the pixel's **angle** (`atan2(uvn.y, uvn.x)` normalized to 0..1) against the `Fill` scalar via `Step` — pixel-perfect, no blur, no `If`.
- **Naming/location** (`MCP_UI.md`): `WBP_` prefix, assets under `/Game/HUD/`. Widget names passed to `ConstructWidget` become the `BindWidget` variable names — match them in the C++/BP widget class.

### Steps
1. **Material `M_CooldownSweep`** (`/Game/HUD/`, Python via `MaterialEditingLibrary`, `MCP_Material.md` pattern): Unlit + Masked; `ScalarParameter "Fill"` (0 = ready/icon clear, 1 = fully swept) driving a `Step` over normalized angle. One `MaterialInstanceDynamic` per slot at runtime.
2. **Shim functions on `UGeoWidgetBuilderUtil`** (new `UFUNCTION`s → full build):
   - `BuildAbilitySlotWidget(UWidgetBlueprint*, ...)` — builds `SizeBox → Overlay`: `Icon` (Image), `CooldownSweep` (Image w/ the material), `CountdownText` (centered Text), `CountText` (corner Text), optional `KeyHintText`. Names are the `BindWidget` targets.
   - `BuildAbilityBarWidget(UWidgetBlueprint*)` — root `HorizontalBox` named e.g. `SlotBox` for runtime population. (If preferred, the bar can stay a thin BP since a `HorizontalBox` root is trivial — but routing it through the shim keeps tree-building in one place and avoids the Python tree limitation.)
3. **Widget classes** (`UGeoUserWidget` subclasses, can be BP or thin C++ with `BindWidget`):
   - **`WBP_AbilitySlot`** — holds `FGeoAbilityBarEntry` + HUD ref; creates the `M_CooldownSweep` MID on init and assigns it to the `CooldownSweep` image brush. **Tick**: early-out unless on cooldown or `bIsDeployable`; else `HUD->GetAbilityCooldown` → set MID `Fill` + countdown text/visibility; `HUD->GetDeployCountForAbility` → set count text.
   - **`WBP_AbilityBar`** — on `InitAbilityBar`/`RefreshAbilityBar`: clear `SlotBox`, `HUD->GetAbilityBarEntries()`, create a `WBP_AbilitySlot` per entry into `SlotBox`, init each with entry + HUD ref. Bind `OnPlayerDeployCountChanged` to refresh badges.
4. Add `WBP_AbilityBar` into the existing player overlay BP, anchored **bottom-center** (top-left-only anchor + pixel offsets per `MCP_UI.md` "Fixed-Position Canvas Slot"). Implement `AGeoHUD::InitAbilityBar`/`RefreshAbilityBar` in the HUD BP to forward to it.
5. Populate `AbilityIcon` + `bShowDeployCount` on each `UAbilityInfo` entry (Python CDO/data-asset edit, `MCP_Blueprint.md`) for all player classes; Mine/Turret deploy abilities → `bShowDeployCount = true`.

### Reusable Python
Per `MCP/CLAUDE.md`, multi-step builds go in `AI/Python/*.py` (referenced by path, not pasted). Model the asset-creation + shim-call flow on `AI/Python/charge_beam_gauge.py` (widget BP creation + `BuildChargeBeamGaugeWidget` shim call) and `AI/Python/crosshair_cursor.py` (image-root shim usage). Add e.g. `AI/Python/ability_bar.py`.

## Files to modify
- `Source/GeoTrinity/Public/AbilitySystem/Data/AbilityInfo.h` — add `bShowDeployCount`.
- `Source/GeoTrinity/Public/HUD/GeoHUD.h` + `Private/HUD/GeoHUD.cpp` — `FGeoAbilityBarEntry`, the three helpers, `InitAbilityBar`/`RefreshAbilityBar` events, deploy-count delegate + binding.
- `Source/GeoTrinity/Private/Characters/PlayableCharacter.cpp` — refresh bar after `ChangeClass()`.
- `Source/GeoTrinity/Public/Tool/GeoWidgetBuilderUtil.h` + `.cpp` — new `BuildAbilitySlotWidget` / `BuildAbilityBarWidget` shim `UFUNCTION`s (editor-only; full build required).
- New assets (under `/Game/HUD/`): `WBP_AbilityBar`, `WBP_AbilitySlot`, material `M_CooldownSweep`; edits to player overlay BP, HUD BP, `UAbilityInfo` data asset.
- New Python: `AI/Python/ability_bar.py` (asset creation + shim calls).
- Docs to update after implementation (follow `MCP_DocStyle.md`): `AI/MCP/MCP_UI.md` (new ability-bar shim functions), `Source/GeoTrinity/Public/Tool/CLAUDE.md` (list the new `GeoWidgetBuilderUtil` functions), `Source/GeoTrinity/Public/HUD/CLAUDE.md` (ability-bar widgets + `AGeoHUD` helpers).

**Reuse:** `UAbilityInfo::GetAbilitiesForClass` / `GetAllPlayersAbilityInfos`, `UGeoGameplayAbility::GetAbilityTag` / `IsPassive`, native `GetCooldownTimeRemainingAndDuration`, `UGeoDeployableManagerComponent::GetDeployables<T>()` / `GetMaxDeployables()` / `OnDeployCountChanged`, `AGeoHUD::GetHudPlayerParams()`.

## Verification
1. Build via `AI/Commands.md` Bash build (mandatory after coding).
2. PIE (MCP `pie_control`): bottom-center row shows one icon per non-passive granted ability with correct art.
3. Fire a cooldown ability → radial sweeps + seconds count down, then clears. Cross-check with GasDebugger plugin ground-truth cooldown.
4. Deploy mines/turrets → count badge decrements; recall/expire → increments back (via `OnDeployCountChanged`, no polling).
5. `ChangeClass` → bar rebuilds with new class's abilities/icons.
6. Confirm no per-frame cost when all abilities ready (slot Tick early-outs).
