# Tool

General-purpose utilities and helpers.

## `UGeoGameplayLibrary.h` (alias: `GeoLib`)
Blueprint-callable static helpers:

**Server check** — always use this:
- `IsServer(UObject*)` / `IsServer(UWorld*)` — true on dedicated server and listen server. Never use `HasAuthority()` or `IsNetMode(NM_DedicatedServer)`.

**Render/cosmetic gate** — use for visuals, NOT `!IsServer()`:
- `IsDedicatedServer(UObject*)` / `IsDedicatedServer(UWorld*)` — true only on a dedicated server (no viewport). Gate cosmetic-only work (montages, local Gameplay Cues, VFX) with `if (!IsDedicatedServer(...))`. `!IsServer()` wrongly skips the listen-server **host**, which is a rendering player — that bug makes boss montages/cues invisible to the host only.

**Time**:
- `GetServerTime(WorldContext, bUpdatedWithPing)` — network-approximated server time. **Never use for local client timing** (charge duration, UI) — use `GetWorld()->GetTimeSeconds()` instead.

**Camera shake**:
- `TriggerCameraShake(WorldContext, ShakeClass, Scale)` — local player only

**Debug**:
- `GetColorForObject(Object)` — deterministic debug color per object

**Constant**: `ArbitraryCharacterZ = 50.0f` — character spawn Z offset

## `GeoAssetManager.h`
Custom asset manager subclass. Configure in `DefaultEngine.ini`.

## `Team.h`
Team definitions and `ETeam` enum used by `IGenericTeamAgentInterface`.

## Editor automation utilities (moved)
The `UEditorUtilityObject` builders `GeoStateTreeBuilderUtil`, `GeoWidgetBuilderUtil`, and `GeoHudWidgetBuilderUtil` now live in the editor-only **`GeoTrinityEditor`** module so the project can package. See `Source/GeoTrinityEditor/Public/Tool/CLAUDE.md`.
