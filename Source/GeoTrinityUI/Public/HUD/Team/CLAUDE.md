# HUD/Team

| File | Role |
|---|---|
| `GeoPlayerCardWidget.h` | One player: class shape + colour, name / role, health meter with shield overhang, optional numbers, class gauge frame (pulses when ready). Local wing (`WBP_PlayerWing`) and every team row use it |
| `GeoTeamListWidget.h` | Other players, one `RowClass` card each in join order; rebuilt on roster change, rows follow their own player |
| `GeoStatsPanelWidget.h` | One line per player writing combat figures in place every tick (non-shipping combat stats) |

- Look comes from `GeoUITheme` meter / frame styles (`Style/CLAUDE.md`), never per-widget values.
