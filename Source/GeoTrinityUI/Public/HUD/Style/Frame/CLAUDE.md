# HUD/Style/Frame

| File | Role |
|---|---|
| `GeoFrame.h` | `Border` drawing a `UGeoFrameStyle` around any content; idle ↔ active (hover, focus with `bActivateOnHoverAndFocus`, or set active) eased on an active timer |
| `GeoFrameStyle.h` | Look: cut-corner outline, optional glow, fill, grid, runners (small squares travelling the outline); every value with an Active twin is the active end of the blend |

- One style asset per kind of frame: button, panel, row, field, screen.
