# GeoTrinityEditor module (Public)

Editor-only module (`Type=Editor`): automation utils and Details/thumbnail customizations kept out of packaged builds. Depends on `GeoTrinity` public headers only.

| Folder / file | Holds |
|---|---|
| `GeoTrinityEditorModule.h` | Registers every customization in `StartupModule`, unregisters in `ShutdownModule` |
| `Tool/` | `UEditorUtilityObject` builders for Python → `Tool/CLAUDE.md` |
| `Detail/` | `IPropertyTypeCustomization`s → `Detail/CLAUDE.md` |
| `Gem/` | `GeoGemCsvSync`: catalog asset ↔ `Data/gems.csv`, both ways, on save; re-applies gem stats of running PIE players |
| `Thumbnail/` | `GeoIconThumbnailRenderer`: `UGeoIcon` thumbnails as drawn in game |
| `Private/Tests/` | Every automation test of the project (`Gameplay/`, `UI/`, `Tutorial/`); editor-only so none ships → `Private/Tests/CLAUDE.md`. `Tutorial/` also needs `UnrealEd` / `LevelEditor` |

- `UEditorUtilityObject` needs the editor-only `Blutility`; never move builders back under `WITH_EDITOR` in `GeoTrinity`.
