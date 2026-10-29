# World/Backdrop

| File | Role |
|---|---|
| `GeoBackdropComponent.h` | Parallax star backdrop: one plane per `Layers` entry, stacked behind and attached to the camera; depth comes from `M_ParallaxStars` offsetting by layer |
| `GeoBackgroundPulseComponent.h` | Floor pulses: simulated pulses (Location, Direction) or Actors mode (origin read off tracked actors each frame); fed to the pulse parameter collection |

- Planes ride the camera because an orthographic projection gives no parallax of its own. Perspective keeps the same fake parallax (planes still ride the camera).
- `ApplyViewOffset(Offset, FOV)` (from `AGeoGameCamera`, perspective zoom): pushes every plane back by `Offset` so it keeps its world depth under the floor while the camera component slides back, and scales it to cover the frustum at its own distance (`2·d·tan(FOV/2)`, never below `LayerSize`). `Offset` 0 = authored ortho layout.
