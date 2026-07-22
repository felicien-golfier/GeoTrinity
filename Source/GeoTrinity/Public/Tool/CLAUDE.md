# Tool

General-purpose utilities and helpers.

## `UGeoGameplayLibrary.h` (alias: `GeoLib`)
Blueprint-callable static helpers:
- `IsServer(...)` — true on dedicated **and** listen server. Always use this over `HasAuthority()`/`IsNetMode(NM_DedicatedServer)`.
- `IsDedicatedServer(...)` — true only with no viewport. Use to gate cosmetic-only work (montages, local Gameplay Cues, VFX) — `!IsServer()` wrongly skips the listen-server host, which is a rendering player.
- `IsLocalPlayerAvatar(APawn*/AActor*)` — true only for the viewing human's own avatar. On a listen server the host's AI pawns are also "locally controlled", so use this instead of `IsLocallyControlled()` for "my own pawn" cosmetics.
- `GetServerTime(...)` — network-approximated server time; **never** for local client timing (charge duration, UI) — use `GetWorld()->GetTimeSeconds()`.
- `TriggerCameraShake(...)` — local player only.
- `GetTargetPoints(WorldContext, PurposeTag, ArenaTag)` — `AGeoTargetPoint`s carrying both tags.
- `TeleportPlayersToTargetPoints(..., ExemptZoneName=NAME_None)` — round-robin teleport; no exempt zone means everyone moves (group respawn); pass a zone to skip pawns already standing inside it (arena fight-commit).
- `GetColorForObject(Object)` — deterministic debug color.
- `ArbitraryCharacterZ = 50.0f` — character spawn Z offset.

## `GeoAssetManager.h`
Custom asset manager subclass, configured in `DefaultEngine.ini`.

## `Team.h`
Team definitions and `ETeam` enum used by `IGenericTeamAgentInterface`.

## Editor automation utilities (moved)
`GeoStateTreeBuilderUtil`, `GeoWidgetBuilderUtil`, `GeoHudWidgetBuilderUtil` now live in the editor-only **`GeoTrinityEditor`** module so the project can package. See `Source/GeoTrinityEditor/Public/Tool/CLAUDE.md`.
