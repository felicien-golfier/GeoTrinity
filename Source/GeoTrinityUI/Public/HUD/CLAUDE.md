# HUD

In-game HUD. Seam to gameplay: `GeoTrinity/Public/HUD/Interface/CLAUDE.md`.

| Folder / file | Holds |
|---|---|
| `GeoHUD.h` | `AGeoHUD`: overlay owner, ability-bar data, boss bar, damage-number registration, debug stats panel |
| `GeoOverlayWidget.h` | Root player overlay; `AbilityBar` / `StatusBar` BindWidgets; per-player column layout |
| `GeoUserWidget.h` | Base HUD widget: `InitFromHUD`, `BindCallbacksFromHUD` |
| `HudFunctionLibrary.h` | `ShouldDrawHUD()`, `GetHealthRatio()` |
| `AbilityBar/` | Ability bar, slots, status bar |
| `Combatant/` | Health bars, boss bar, damage numbers, widget component |
| `Team/` | Player card, team list, stats panel |
| `Gauge/` | World-space charge gauges |
| `Tutorial/` | Cat line, quest card → `Tutorial/CLAUDE.md` |
| `Menu/` `Style/` | see module map |

```
AGeoHUD (owns OverlayWidget)
├── OverlayWidget (UGeoOverlayWidget)
│     ├── AbilityBar (UGeoAbilityBarWidget)
│     └── StatusBar (UGeoStatusBarWidget)
└── BossHealthBarWidget (UGeoBossHealthBarWidget)  — during boss fights
```

## AGeoHUD
- `InitOverlay`: overlay created owned by the PlayerController (slots resolve key labels via `GetOwningLocalPlayer()`).
- `BindToPawn`, `BuildAbilityBar` (also from `OnRep_PlayerClass`), `ShowBossHealthBar` (local player 0 only).
- `RegisterASCForDamageNumbers`: Health / Shield deltas spawn numbers from `DamageNumberPool`.
- `GetActiveEffectIcons()`: status-bar data incl. the synthetic sweet-spot gauge entry.
- `IsAbilityReady(Tag)`: spec's ability (instance or CDO) → `IsReady(HUD ASC)`; drives slot frames.
- `GetAbilityGauge(Tag, Fill, bReady, Label)`: same spec lookup → `GetClassGauge` + `GetGaugeLabel`; false when the ability has no class gauge. Drives the slot value gauge.
- Non-shipping combat-stats panel (pure Slate, `Geo.ShowCombatStats`), rebuilt on roster change.

## Overlay layout
- `ApplyColumnLayout()`: each local player gets `1/NumLocalPlayers` of the width (solo whole screen; two halves; three left/middle/right); re-runs on `OnLocalPlayerAddedEvent` / `OnLocalPlayerRemovedEvent`, undoing the previous column first.

## Adding HUD changes
- Screen-space: `BlueprintImplementableEvent` on `AGeoHUD` → HUD BP → forward to `OverlayWidget`.
- World-space: `UWidgetComponent` (`Space=Screen`) attached to `WidgetAnchorComponent`.
