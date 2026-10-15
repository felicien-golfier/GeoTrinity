# HUD/Style/Glyph

Geometric drawing widgets.

| File | Role |
|---|---|
| `GeoShape.h` | Flat regular polygon or circle, hollow or solid, optionally turning (class shapes, menu hex shells); an `OutlineStyle` makes it wear a frame style |
| `GeoShapePainter.h` | Shared painting of a styled outline: ease towards active + runner travel |
| `GeoGemGlyph.h` | Gem (tier polygon in stat colour) or its socket (dark, with a line); highlight line + glow from `GemStyle`. The echo socket, empty or locked, is its own look: the Chip, Cut and Prism polygons outlined one inside the other, no Core mark |
| `GeoEchoHalo.h` | Halo of the echo socket: disc, ring, two counter-turning rosaces of outline squares, runner squares along the ring; colour from the caller (`SetColor`, `SetLocked`, `SetActive`, `PlayFlare`), look and speeds from a `UGeoFrameStyle` (idle ↔ active blend), drawn with `FGeoShapePainter`; never takes input |
| `GeoIconImage.h` | UMG wrapper drawing a `UGeoIcon` in its game colour × `Tint` |
| `SGeoIconImage.h` | Slate leaf painting a `UGeoIcon` over its box, re-read each paint; also used by the editor preview and thumbnail |
