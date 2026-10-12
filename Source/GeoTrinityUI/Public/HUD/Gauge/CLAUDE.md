# HUD/Gauge

World-space charge gauges on the character's widget components; driven through `IGeoChargeGaugeWidgetInterface`.

| File | Role |
|---|---|
| `GeoDeployChargeGaugeWidget.h` | Progress bar of the deploy ability's `GetChargeRatio()`; set `DeployAbility` before adding to viewport |
| `GeoChargeBeamGaugeWidget.h` | `ChargeBar` + `SweetSpotBar` window; gradient bands shade toward the window centre while the sweet-spot passive gauge is full |

- Attach to `WidgetAnchorComponent` (non-rotating), `Space = Screen`.
