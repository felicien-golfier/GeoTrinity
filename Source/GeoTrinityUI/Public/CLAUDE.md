# GeoTrinityUI module (Public)

Runtime UI module (`Type=Runtime`): HUD, widgets, menus, styles. Depends on `GeoTrinity`; gameplay never references a class here (interfaces in `GeoTrinity/Public/HUD/Interface/`). Excluded from the dedicated-server target. `.cpp` mirrored in `Private/`.

| Folder | Holds | Doc |
|---|---|---|
| `HUD/` | `AGeoHUD`, overlay, in-game widgets | `HUD/CLAUDE.md` |
| `HUD/Menu/` | Main / pause menus and every page | `HUD/Menu/CLAUDE.md` |
| `HUD/Style/` | Themed primitives drawn with Slate | `HUD/Style/CLAUDE.md` |
| `CatFace/` | The Cat's Slate face, motion, expressions; HUD and loading screen | `CatFace/CLAUDE.md` |
| `LoadingScreen/` | Map-load screen painted by the movie player | `LoadingScreen/CLAUDE.md` |

Module class `FGeoTrinityUIModule` (`Private/GeoTrinityUI.cpp`): registers the loading-screen factory on startup, clears it on shutdown.

Tests of this module live in `GeoTrinityEditor/Private/Tests/UI/` (`GeoTrinityEditor/Private/Tests/CLAUDE.md`); no test code here.

UI rules (generic reusable styles, Blueprint-tweakable values): `AI/UI.md`. Widget building: `AI/MCP/MCP_UI.md`.

## Blueprint-added components
- Combatant health bar and deploy / charge-beam gauge components are not C++ subobjects: they are added in Blueprint and resolved at runtime with `FindComponentByClass<UWidgetComponent>()`. A damageable deployable without one (`BP_Deployable_Turret`, `BP_Pillar` have it) shows no bar; non-damageable ones (HealingZone, BuffPickup) need none.
- Configure: Screen space, draw-at-desired-size, absolute rotation, `WBP_CombattantLifeBar`.
- `CoreRedirects` in `Config/DefaultEngine.ini` map the old `/Script/GeoTrinity.X` HUD classes to `/Script/GeoTrinityUI.X` until every BP is re-saved.
- Server target (`GeoTrinityServer.Target.cs`) lists `GeoTrinity` only; building it needs a source engine (`AI/Commands.md`).
