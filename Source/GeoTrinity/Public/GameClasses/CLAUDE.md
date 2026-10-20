# GameClasses

Unreal framework classes.

| Folder / file | Holds |
|---|---|
| `GeoGameInstance.h` | Persists across levels; Steam session, `LeaveSessionAndReturnToMenu()` → `MainMenuMap`; `TutorialMap`, `TrainingRoomMap` (soft refs, set on `BP_GeoGameInstance`); `OpenTutorial(Room = None)` (console `Geo.Tutorial.Jump <Room name or number>`) / `LeaveTutorial()` / `IsInTutorial()` (`AI/Tutorial/Flow.md`) |
| `Match/` | `GeoGameMode`, `GeoGameState`, `GeoPlayerState` → `Match/CLAUDE.md` |
| `Player/` | `GeoPlayerController`, `GeoGameViewportClient` (couch coop) → `Player/CLAUDE.md` |
| `MainMenu/` | `GeoMainMenuGameMode` (no pawn), `GeoMainMenuPlayerController` (creates `UGeoMainMenuWidget` on BeginPlay) |
