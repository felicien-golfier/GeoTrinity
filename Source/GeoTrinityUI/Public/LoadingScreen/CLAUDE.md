# LoadingScreen

The screen every map load shows: the Cat's face rides a time-driven bar, a tip in its voice underneath, for at least `MinimumLoadingScreenSeconds` (2.5 s) even when the map loads instantly. Pure Slate, painted by the engine's movie player on its loading thread. Look: Research artifact, "Loading" tab, Concept 4.

| File | Role |
|---|---|
| `SGeoLoadingScreen.h` | Compound screen: backdrop, title, bar, Cat face on the bar's head, percent, tip. `SGeoLoadingScreen::Make(Request)` = the factory the module registers |
| `SGeoLoadingBackdrop.h` | Leaf: black field, fixed stars, hex lattice fading toward the centre; flat lines, no texture |
| `GeoLoadingTimeline.h` | Pure functions of elapsed seconds: bar progress, shown tip, talking flag; tested |

## Flow
1. `FGeoTrinityUIModule::StartupModule` (`Private/GeoTrinityUI.cpp`) registers `SGeoLoadingScreen::Make` with `GeoLoadingScreen::SetFactory` (`GeoTrinity/Public/HUD/Interface/CLAUDE.md`); `ShutdownModule` clears it.
2. `UGeoGameInstance` on `PreLoadMap` (`GameClasses/CLAUDE.md`, "Loading screen"): request → factory → `SetupLoadingScreen` → `PlayMovie`.
3. The movie player paints the widget while `LoadMap` runs and closes it at load end, never before the minimum. The world does not tick under it.

## Covered loads
- Every `LoadMap`: boot into the main menu (the game instance's `Init` runs before the first map), host, join, tutorial, `Geo.Tutorial.Jump`, tutorial exit, back to menu, network-failure return. A client travelling with its host shows its own.
- Not covered: the engine splash before the first map; the editor and PIE (movie player off); dedicated server (no UI module, so no factory).
- One kind today. The request carries the destination map so a per-map screen (the tutorial's intro board pages 1-2, `AI/Tutorial/IntroBoard.md`) is a choice inside `Make`, the hook untouched.

## Thread rules
- Everything that reads a UObject happens in `Make`, on the game thread: tips picked, markup converted, `{Key}` labels resolved, fonts and text styles copied into the widget. After `Construct` the widget reads no UObject, no world, no line bank.
- The assets behind those copies survive the load's garbage collection: the theme is resident (`UGeoUITheme::Get`); the module holds `DT_CatLine` in a `TStrongObjectPtr` for the process life (never the widget: the movie player may destroy it off the game thread).
- Time = real seconds since `Construct`, from the active timer's current time; never world time.
- Load end = `GetMoviePlayer()->IsLoadingFinished()`, the one engine call made from the loading thread.

## Bar (`GeoLoadingTimeline`)
- Driven by time, not load progress (`LoadMap` reports none worth showing).
- Progress = ease-out of `Elapsed / Minimum`, 0 → 1. While loading, capped at .9, then creeping toward 1 without reaching it (.9 + .09 × (1 − e^(−overtime / 2 s))). Loaded before the minimum: the curve runs to 1 at the minimum, when the screen closes. Loaded after: the screen closes at load end, wherever the creep is.
- Percent beside the bar: three digits + `%` (`072 %`), theme `Mono`.
- Cat face (`CatFace/CLAUDE.md`): centred on the fill's head, clamped inside the track; default palette; talking while a tip is fresh (`TipTalkSeconds`). Expression changes every eighth of the bar (`CatFace/CLAUDE.md`, "Expressions").

## Tip
- `Make` draws `TipCount` lines with `GeoCatLines::PickLine(Request.TipCategories, true)`: same bank and bag as every Cat speaker (`GeoTrinity/Public/Tool/CatLine/CLAUDE.md`). Drawn up front because the bank is not thread-safe; a long load cycles them.
- First tip shows whole, at once, and stays until the next (`TipSeconds`); no typewriter (a 2.5 s load leaves no time to type and read).
- Markup: `GeoCatLine::ToRichText` (`HUD/Tutorial/CLAUDE.md`, "Markup"); `{Key:<InputTag>}` = the key mapped for `Request.LocalPlayer`, the Cat line widget's resolver (`UHudFunctionLibrary::GetInputTagKeyLabel`). `SRichTextBlock` styled from `DT_CatLine` (the `Bold` row). `<Shake>` runs show still: the shake decorator is UMG only.
- `TipCount`, `TipSeconds`, `TipTalkSeconds` = `constexpr`s at the top of `SGeoLoadingScreen.cpp`, the one place to tune.

## Look
- Black field, stars, lattice, title "GEOTRINITY" (theme `Title`), track + fill, Cat, percent, tip (theme `Body`), laid out on a 1600 × 900 design box scaled to the viewport (Concept 4 positions).
- Colours and sizes: `FGeoLoadingScreenStyle` on the theme (`UGeoUITheme::LoadingScreen`, `HUD/Style/CLAUDE.md`), so a look change is a theme edit.
