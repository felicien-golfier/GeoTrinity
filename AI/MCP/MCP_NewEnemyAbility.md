# MCP New Enemy Ability Checklist

Steps required every time a new enemy `GA_` Blueprint ability is created.

## Key Constraints

- Tags in `Config/Tags/GeoGameplayTags.ini` require an **editor restart** to resolve — do this first.
- Every `GA_` ability CDO must carry two asset tags in its `AbilityTags` container: one under `Ability.Spell.*` and one under `Ability.Type.*` (`Special`, `SpecialAlternative`, `Basic`, `Dash`, `Reload`, `Passive`). The `Ability.Type.*` value matches the input slot the ability is bound to. Missing either trips an ensure when the ability catalog is populated.
- Reparenting an existing ability Blueprint to a different C++ base **clears its `AbilityTags`** — re-set both asset tags afterward and save; read or re-set them on an existing ability with `AI/Python/ability_tags.py`.
- `AbilityTag` on `FGameplayAbilityInfo` must be set in the `import_text` call: `AbilityTag=(TagName="Ability.Spell.X")`.
- `AbilityClass` on `FGameplayAbilityInfo` cannot be set via `set_editor_property` — use `import_text` instead.
- `import_text` requires the full `BlueprintGeneratedClass` path prefix, not the raw `get_path_name()` result — see `AI/Python/new_enemy_ability.py`.
- Always set `AbilityDisplayName` and `Description` in the same `import_text` call.
- The ability tag must be in `BP_StarBoss` ASC `StartupAbilityTags` or it is never granted at `BeginPlay`.
- After modifying any subobject property (e.g. ASC `StartupAbilityTags`), save with `EditorAssetLibrary.save_loaded_asset(bp_boss)`.

## Steps

1. Add tag to `Config/Tags/GeoGameplayTags.ini`, restart editor.
2. Run `AI/Python/new_enemy_ability.py` (fill the variables at the top).
3. **Always ask** whether the ability should be added to the boss behaviour StateTree before finishing. If yes, wire it — see `MCP_StateTree.md` and `AI/Python/state_tree_edit.py`.

## Assets

| Asset | Role |
|---|---|
| `/Game/AbilitySystem/Data/DA_AbilityInfo` | Global ability catalog — `EnemyAbilityInfos` array |
| `/Game/Characters/Enemies/BP_StarBoss` | ASC `StartupAbilityTags` at subobject index 7 |
