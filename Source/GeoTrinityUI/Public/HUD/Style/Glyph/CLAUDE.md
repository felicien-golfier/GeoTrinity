# HUD/Style/Glyph

Geometric drawing widgets.

| File | Role |
|---|---|
| `GeoShape.h` | Flat regular polygon or circle, hollow or solid, optionally turning (class shapes, menu hex shells); an `OutlineStyle` makes it wear a frame style |
| `GeoShapePainter.h` | Shared painting of a styled outline: ease towards active + runner travel |
| `GeoGemGlyph.h` | Gem (tier polygon in stat colour) or its socket (dark, with a line); highlight line + glow from `GemStyle` |
| `GeoIconImage.h` | UMG wrapper drawing a `UGeoIcon` in its game colour × `Tint` |
| `SGeoIconImage.h` | Slate leaf painting a `UGeoIcon` over its box, re-read each paint; also used by the editor preview and thumbnail |
