# Tool

General-purpose utilities and helpers.

## `UGeoGameplayLibrary.h` (alias: `GeoLib`)
Blueprint-callable static helpers:

**Server check** — always use this:
- `IsServer(UObject*)` / `IsServer(UWorld*)` — true on dedicated server and listen server. Never use `HasAuthority()` or `IsNetMode(NM_DedicatedServer)`.

**Render/cosmetic gate** — use for visuals, NOT `!IsServer()`:
- `IsDedicatedServer(UObject*)` / `IsDedicatedServer(UWorld*)` — true only on a dedicated server (no viewport). Gate cosmetic-only work (montages, local Gameplay Cues, VFX) with `if (!IsDedicatedServer(...))`. `!IsServer()` wrongly skips the listen-server **host**, which is a rendering player — that bug makes boss montages/cues invisible to the host only.

**Local-avatar check** — use for "my own pawn" cosmetics, NOT `IsLocallyControlled()`:
- `IsLocalPlayerAvatar(APawn*)` — true only for the viewing human's own avatar (`IsPlayerControlled() && IsLocallyControlled()`). On a listen server the host's AI pawns are also locally controlled, so `IsLocallyControlled()` alone is true for every host enemy. Use this to hide the local player's own floating bar, pick the local-player hit-flash material, etc.
- `IsLocalPlayerAvatar(AActor const*)` — non-pawn overload; casts to `APawn` first and returns false for non-pawn actors. Convenience for call sites that only have an `AActor*`.

**Time**:
- `GetServerTime(WorldContext, bUpdatedWithPing)` — network-approximated server time. **Never use for local client timing** (charge duration, UI) — use `GetWorld()->GetTimeSeconds()` instead.

**Camera shake**:
- `TriggerCameraShake(WorldContext, ShakeClass, Scale)` — local player only

**Target points / respawn** (server):
- `GetTargetPoints(WorldContext, PurposeTag, ArenaTag)` — the `AGeoTargetPoint`s carrying both a `TargetPoint.*` purpose and an `Arena.*` tag.
- `TeleportPlayersToTargetPoints(WorldContext, PurposeTag, ArenaTag, ExemptZoneName = NAME_None)` — teleports player pawns to those points round-robin. **No exempt zone → teleports everyone** (the group respawn, which always moves the whole group to the checkpoint); pass a zone tag to skip pawns already standing inside an actor carrying it (the arena's fight-commit move leaves players already in position).

**Debug**:
- `GetColorForObject(Object)` — deterministic debug color per object

**Constant**: `ArbitraryCharacterZ = 50.0f` — character spawn Z offset

## `GeoAssetManager.h`
Custom asset manager subclass. Configure in `DefaultEngine.ini`.

## `Team.h`
Team definitions and `ETeam` enum used by `IGenericTeamAgentInterface`.

## Editor automation utilities (moved)
The `UEditorUtilityObject` builders `GeoStateTreeBuilderUtil`, `GeoWidgetBuilderUtil`, and `GeoHudWidgetBuilderUtil` now live in the editor-only **`GeoTrinityEditor`** module so the project can package. See `Source/GeoTrinityEditor/Public/Tool/CLAUDE.md`.
