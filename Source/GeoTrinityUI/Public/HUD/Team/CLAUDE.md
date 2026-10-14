# HUD/Team

| File | Role |
|---|---|
| `GeoPlayerCardWidget.h` | One player: class shape + colour, name / role, health meter with shield overhang, optional numbers, class gauge frame (pulses when ready). Local wing (`WBP_PlayerWing`) and every team row use it |
| `GeoTeamListWidget.h` | Other players, one `RowClass` card each in join order; rebuilt on roster change, rows follow their own player |
| `GeoXpStripWidget.h` | Local wing only: thin XP bar under the health bar + class level + echo level, bare numbers (`WBP_XpStrip`, in `WBP_PlayerWing` below the health meter) |
| `GeoStatsPanelWidget.h` | One line per player writing combat figures in place every tick (non-shipping combat stats) |

- Class gauge: first granted ability whose `GetClassGauge(ASC, Fill, bReady)` returns true (one per class: sweet-spot, sacrifice detonate, recall turret); none → ring and text collapsed. The card never names an ability class.
- Gauge text = that same ability's `GetGaugeLabel` through `GaugeFormat` — the same number as its slot tile (`Abilities/Base/CLAUDE.md`, "HUD readiness"). No label → text collapsed, ring still shown. Never written from the fill.
- Look comes from `GeoUITheme` meter / frame styles (`Style/CLAUDE.md`), never per-widget values.

## XP strip (local wing)
Design: UI canvas "HUD · Wings", under the health bar.
- Placement: same left edge and width as the health bar, 6 px under it; bar 5 px thin (`UGeoMeter` bar style, near-white at 75%), so the wing grows by ~11 px and nothing moves. Numbers sit left of the bar, the class level flush with the health bar's left edge, never over the bar.
- Numbers only, no words, no `/ max`: class level (light), then a middle point and the echo level (violet, same size; `EchoLevelFormat` "· {0}"). Two fixed-width slots always reserved: the echo slot is Hidden (not Collapsed) while the shared echo level is 0, shown from 1 whatever class is played, so the class level never moves and the bar never shifts when the echo lands (the echo level never shows a max: `GeoTrinity/Public/Gem/CLAUDE.md`, "Echo socket").
- Fill: one continuous progress, no visual difference between the two: XP into the current class level, then at class level 20 XP into the current echo level. Same source as the character sheet's XP meter.
- Quiet: no pulse, no popup; a level-up just resets the bar. Local player only; other players' cards have none.
- Data: the local card (`XpStrip`, `BindWidgetOptional`) hands the strip its player state (`InitForPlayer`) and calls `Refresh` on a class change; the strip reads that player's local gem profile (`UGeoGemProfileSave::GetXpProgress`, the same function as the character sheet's XP meter) and refreshes on `UGeoGemSubsystem::OnProfileCommitted`. It never looks the player up itself.
- Colours: `LevelColor` / `EchoLevelColor` on the strip, applied at construct (a `UGeoText` role would repaint the BP value); echo reads violet, never the class level's colour.
- Gap: `HealthStack` keeps 12 above and 2 below the bar, the strip 2 under that (`xp_strip.py`), so bar and strip read as one block.
