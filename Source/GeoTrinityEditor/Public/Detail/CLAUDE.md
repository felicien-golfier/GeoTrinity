# GeoTrinityEditor/Detail

Details-panel customizations. A new one must be registered in `FGeoTrinityEditorModule::StartupModule` (`RegisterCustomPropertyTypeLayout` keyed on the struct name).

| File | Role |
|---|---|
| `ExternalProjectileParamsCustomization.h` | Each value under the `EOverrideParam` toggle named by its `OverrideToggle` metadata; hidden unless the toggle is `OverrideValue`. Metadata-driven: a new param needs no change here |
| `GeoCurveSourceCustomization.h` | Curve + its two sources (attribute / ability level) on `FGeoSoundEntry`, `FGeoVFXParams`, `FGeoBurstVFXParams`: sources hide each other, curve hidden until one is set |
| `GeoIconCustomization.h` | `UGeoIcon` Preview row on top, live while strokes are edited |

- Use a customization (not `EditCondition`) when a base-struct value is gated by a derived-struct toggle.
