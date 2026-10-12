# HUD/Style/Meter

| File | Role |
|---|---|
| `GeoMeter.h` | Draws a ratio in a `UGeoMeterStyle`: health with shield overhang, class gauge, cooldown sweep, status timer; ring may take a polygon shape and pulse with copies when ready |
| `GeoMeterStyle.h` | Look: track, fill, overhang (shield, behind and around the fill), ticks, outline |

- One style asset per meter kind: player health, team health, boss health, class ring, cooldown, status time, slot gauge.
- Shapes: `Bar` (left → right), `Ring` (clockwise), `Sweep` (radial pie: time), `Rise` (whole box filled bottom → top: a value). Time is radial, value rises.
- `Rise` takes `FillTint` like the others: one `DA_Meter_SlotGauge` filled in each class's colour.
