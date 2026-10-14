# AI/Python/Asset

Generic asset editing, generation, saving. Doc: `AI/MCP/MCP_Blueprint.md`.

| Script | Does |
|---|---|
| `save_dirty_assets.py` | List and save dirty content packages, report to a file |
| `struct_container_edit.py` | Rewrite a struct array / map on an asset or Blueprint |
| `curve_asset_authoring.py` | Write a curve asset from a table of keys |
| `boss_fight_loop.py` | Boss fight-loop layers (`loop.json`) onto its loop sound component, keeping tuned drift |
| `import_sound_waves.py` | Import `SFX_` WAVs of each `SourceArt/Audio` folder as `SW_` (replace, loop flags, prune orphans) |
| `import_textures.py` | Import images as textures with the right sRGB / compression |
| `trim_sound_start_silence.py` | Cut leading silence off every Sound Wave at the SFX root, reimport in place |
