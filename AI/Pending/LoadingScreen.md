# LoadingScreen — pending

Goal: map loads are near instant, so every map change shows the Cat-on-the-bar loading screen for at least 2-3 s with a random Cat tip, hooked in the game instance.
Priority: 2
Docs written: `GeoTrinityUI/Public/LoadingScreen/CLAUDE.md` (new), `GeoTrinityUI/Public/CatFace/CLAUDE.md` (new, moved from `HUD/Tutorial/`), `GeoTrinityUI/Public/CLAUDE.md`, `GeoTrinityUI/Public/HUD/Tutorial/CLAUDE.md`, `GeoTrinityUI/Public/HUD/Style/CLAUDE.md`, `GeoTrinity/Public/HUD/Interface/CLAUDE.md`, `GeoTrinity/Public/GameClasses/CLAUDE.md` ("Loading screen"), `GeoTrinity/Public/Tool/CatLine/CLAUDE.md`, `GeoTrinityEditor/Private/Tests/CLAUDE.md`, `AI/Tutorial/IntroBoard.md`.
Expressions are their own feature: `CatExpressions.md` (after this one).

## Moves (grouping)
- [ ] `HUD/Tutorial/SGeoCatFace.h/.cpp`, `GeoCatFaceMotion.h/.cpp`, `GeoCatFaceWidget.h/.cpp` → `CatFace/` (Public + Private); fix includes (`GeoCatLineWidget.cpp`, `GeoCatFaceWidget.cpp`, `SGeoCatFace.cpp`, `GeoTrinityEditor/Private/Tests/UI/GeoCatLineTests.cpp`). Same module, so `/Script/GeoTrinityUI.GeoCatFaceWidget` is unchanged: no redirect, `WBP_CatLine` untouched

## Code (to implement)
Read `AI/CodingStyle.md` first.
- [ ] `GeoTrinity/Public/HUD/Interface/GeoLoadingScreenFactory.h/.cpp` — `FGeoLoadingScreenRequest` (`FString MapPackageName`, `FGeoCatLineCategories TipCategories`, `ULocalPlayer const* LocalPlayer`); namespace `GeoLoadingScreen`: `using FFactory = TFunction<TSharedPtr<SWidget>(FGeoLoadingScreenRequest const&)>`, `GEOTRINITY_API void SetFactory(FFactory)`, `GEOTRINITY_API TSharedPtr<SWidget> Make(FGeoLoadingScreenRequest const&)`; static `FFactory` in the `.cpp`
- [ ] `GeoTrinity.Build.cs` — `"MoviePlayer"` in `PrivateDependencyModuleNames`
- [ ] `GameClasses/GeoGameInstance.h/.cpp` — `MinimumLoadingScreenSeconds = 2.5f` (`EditDefaultsOnly`, `ClampMin = 0`, category `GeoLoadingScreen`), `LoadingTipCategories` (C++ default `{"Controls","Roles","Teamwork"}`); `Init` binds `PreLoadMap` (`AddUObject`), `Shutdown` override removes it; `OnPreLoadMap(FString const& MapName)` per `GameClasses/CLAUDE.md` ("Loading screen"): request (`LocalPlayer` = `GetFirstGamePlayer()`), `GeoLoadingScreen::Make`, attributes, `SetupLoadingScreen`, `PlayMovie` unless playing
- [ ] `GeoTrinityUI/Private/GeoTrinityUI.cpp` — `FGeoTrinityUIModule : IModuleInterface`: `StartupModule` → `GeoLoadingScreen::SetFactory(&SGeoLoadingScreen::Make)`, `ShutdownModule` → `SetFactory(nullptr)` and releases the `DT_CatLine` strong pointer; `IMPLEMENT_MODULE(FGeoTrinityUIModule, GeoTrinityUI)`. Keep the module's Default loading phase (loads before the engine creates the game instance)
- [ ] `GeoTrinityUI/Public/HUD/HudFunctionLibrary.h/.cpp` — `GetInputTagKeyLabel(ULocalPlayer const*, FGameplayTag)`: the lambda now in `UGeoCatLineWidget::ShowLine` (player ability info by input tag → `GetMappedKey` → display name, else the tag name), moved; `GeoCatLineWidget.cpp` calls it
- [ ] `GeoTrinityUI/Public/HUD/Style/GeoUITheme.h` — `FGeoLoadingScreenStyle` (`TrackColor`, `FillColor`, `LatticeColor`, `StarColor`, `TitleGlowColor`, `CatSize`), `UPROPERTY(EditAnywhere, Category = "GeoLoadingScreen") FGeoLoadingScreenStyle LoadingScreen`; defaults = Concept 4 values below
- [ ] `GeoTrinityUI/Public/LoadingScreen/GeoLoadingTimeline.h/.cpp` — `GEOTRINITYUI_API` pure functions: `GetBarProgress(double Elapsed, double Minimum, bool bLoaded)`, `GetTipIndex(double Elapsed, int32 TipCount)`, `IsTalking(double Elapsed)`; curves per `LoadingScreen/CLAUDE.md` ("Bar")
- [ ] `GeoTrinityUI/Public/LoadingScreen/SGeoLoadingBackdrop.h/.cpp` — `SLeafWidget`: black fill, 70 fixed stars (3 px squares, seeded like the concept's `stars(77)`), hex lattice r = 80 design units, opacity .3, radial fade (transparent at 15 % from centre → full at the edges); `MakeLines`, no brush texture
- [ ] `GeoTrinityUI/Public/LoadingScreen/SGeoLoadingScreen.h/.cpp` — `SCompoundWidget`; `static TSharedPtr<SWidget> Make(FGeoLoadingScreenRequest const&)` does every UObject read (`LoadingScreen/CLAUDE.md`, "Thread rules"): `TipCount` (4) lines through `GeoCatLines::PickLine`, `ToRichText` + `GetInputTagKeyLabel`, theme text styles copied, an `FSlateStyleSet` built from `DT_CatLine` rows (as `URichTextBlock::MakeStyleInstance` does), palette `GeoCatFace::DefaultPalette`; one active timer drives bar, percent, Cat position, tip swap, talking; `constexpr TipSeconds = 4.5`, `TipTalkSeconds = 1.2`, `TipCount = 4`
- [ ] `CatFace/SGeoCatFace.h` — `FGeoCatPalette` + `GeoCatFace::DefaultPalette` (today's `UGeoCatFaceWidget` defaults: Base `(1, .102, .047)`, Shade `(.55, .056, .026)`, Ink `(.12, .012, .006)`, Light `(1, .87, .84)`), `SetPalette`; `UGeoCatFaceWidget` property defaults read it
- [ ] Update the header doc comments of every touched class; clang-format every touched file

## Concept 4 layout (1600 × 900 design box, copy from the Research artifact "Loading" tab)
- Title "GEOTRINITY" centred at (800, 190), size 96, letter spacing 34, glow
- Track: x 200 → 1400, y 528, height 24, teal `#1fd6d6` at 14 %; centre line y 540, white 18 %, 3 px. Fill: line y 540 from 200 to the head, teal, 5 px, glow
- Cat: centred on the head at y 540, size 120, x clamped to [250, 1350]
- Percent: right-aligned at (1400, 672), mono 26, spacing 6, teal. Tip: centred at (800, 790), body 38, white

## Tests
- [ ] `GeoTrinityEditor/Private/Tests/UI/GeoLoadingScreenTests.cpp`:
  - `GeoTrinityUI.LoadingScreen.Timeline` — progress 0 at 0; rises; reaches 1 at the minimum when loaded; capped at .9 while loading; past the minimum still loading: above .9, below 1, rising; tip index 0 for the first `TipSeconds`, then 1, wraps at `TipCount`; talking only for the first `TipTalkSeconds` of each tip
  - `GeoTrinityUI.LoadingScreen.TipPool` — the C++ default `LoadingTipCategories` count > 0 lines in the real `Data/CatLines.txt` (`GeoCatLines::CountLines`)
- [ ] Existing `GeoTrinityUI.Tutorial.CatFaceMotion` / `CatLineMarkup` still green after the move

## Verify
- [ ] `Tools\Build_Editor.bat` (editor closed, `GEO_NO_PAUSE=1`): Editor target, multi-module change (`AI/CodingStyle.md`, "Modules")
- [ ] `Tools\Run_Tests.bat GeoTrinityUI`
- [ ] Tutorial gate: `GeoTrinity.Tutorial.FullRun` green (root `CLAUDE.md`); the movie player is off in the editor, so it must not change
- [ ] User looks in a standalone game or package (movie player is off in PIE): boot → menu, menu → tutorial, `Geo.Tutorial.Jump Dash`, tutorial exit; each ≥ 2.5 s, tip readable, bar ends full on a quick load, the tutorial's first line starts after the screen leaves
