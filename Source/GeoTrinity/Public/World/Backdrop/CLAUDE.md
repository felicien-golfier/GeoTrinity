# World/Backdrop

| File | Role |
|---|---|
| `GeoBackdropComponent.h` | Parallax star backdrop: one plane per `Layers` entry, stacked behind and attached to the camera; depth comes from `M_ParallaxStars` offsetting by layer |
| `GeoBackgroundPulseComponent.h` | Floor pulses: simulated pulses (Location, Direction) or Actors mode (origin read off tracked actors each frame); fed to the pulse parameter collection |

- Planes ride the camera because an orthographic projection gives no parallax of its own.
