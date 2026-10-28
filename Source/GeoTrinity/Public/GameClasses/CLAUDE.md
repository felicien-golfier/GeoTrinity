# GameClasses

Unreal framework classes.

| Folder / file | Holds |
|---|---|
| `GeoGameInstance.h` | Persists across levels; Steam session, `LeaveSessionAndReturnToMenu()` → `MainMenuMap`; `TutorialMap`, `TrainingRoomMap` (soft refs, set on `BP_GeoGameInstance`); `OpenTutorial(Room = None)` (console `Geo.Tutorial.Jump <Room name or number>`) / `LeaveTutorial()` / `IsInTutorial()` (`AI/Tutorial/Flow.md`) |
| `Match/` | `GeoGameMode`, `GeoGameState`, `GeoPlayerState` → `Match/CLAUDE.md` |
| `Player/` | `GeoPlayerController`, `GeoGameViewportClient` (couch coop) → `Player/CLAUDE.md` |
| `MainMenu/` | `GeoMainMenuGameMode` (no pawn), `GeoMainMenuPlayerController` (creates `UGeoMainMenuWidget` on BeginPlay) |

## Loading screen
The game instance owns when a loading screen shows; the UI module owns what it is (`GeoTrinityUI/Public/LoadingScreen/CLAUDE.md`).
- `Init` binds `FCoreUObjectDelegates::PreLoadMap` once; `Shutdown` unbinds. Every `LoadMap` passes there (`OpenLevel`, `ServerTravel`, `ClientTravel`; no seamless travel in the game), the first one into `MainMenuMap` included.
- `OnPreLoadMap(MapName)`: skipped when `!IsMoviePlayerEnabled()` (editor, PIE, `-nomovie`) or `GeoLoadingScreen::Make` returns null (server). Else `FLoadingScreenAttributes`: `WidgetLoadingScreen` = the made widget, `MinimumLoadingScreenDisplayTime` = `MinimumLoadingScreenSeconds`, `bAutoCompleteWhenLoadingCompletes` = true, `bAllowEngineTick` = false; `SetupLoadingScreen`, then `PlayMovie()` unless `IsMovieCurrentlyPlaying()` (the movie player's own `PreLoadMap` handler may have run first, finding nothing set up).
- `bAllowEngineTick` false: the new world does not tick until the screen leaves, so nothing starts unseen (the tutorial's first Cat lines, a boss intro). The minimum holds the game thread up to 2.5 s; the loading thread keeps the screen animating.
- `MinimumLoadingScreenSeconds` (2.5, `EditDefaultsOnly`), `LoadingTipCategories` (`FGeoCatLineCategories`, C++ default `Controls`, `Roles`, `Teamwork`; the category dropdown on `BP_GeoGameInstance`).
- `MoviePlayer` = private dependency of `GeoTrinity.Build.cs`.
