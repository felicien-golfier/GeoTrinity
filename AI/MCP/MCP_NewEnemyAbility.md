# MCP New Enemy Ability Checklist

Steps required every time a new enemy `GA_` Blueprint ability is created.

## Key Constraints

- Tags in `Config/Tags/GeoGameplayTags.ini` require an **editor restart** to resolve — do this first.
- `AbilityTag` on `FGameplayAbilityInfo` is `Transient`. Re-population is triggered by calling `set_editor_property("EnemyAbilityInfos", ...)` which fires `PostEditChangeProperty → PopulateAbilityTags`.
- `AbilityClass` on `FGameplayAbilityInfo` cannot be set via `set_editor_property` — use `import_text` instead.
- The ability tag must be in `BP_StarBoss` ASC `StartupAbilityTags` or it is never granted at `BeginPlay`.

## Steps

1. Add tag to `Config/Tags/GeoGameplayTags.ini`, restart editor.
2. Run `AI/Python/new_enemy_ability.py` (fill the variables at the top).
3. Wire into the StateTree if needed — see `MCP_StateTree.md` and `AI/Python/state_tree_edit.py`.

## Assets

| Asset | Role |
|---|---|
| `/Game/AbilitySystem/Data/DA_AbilityInfo` | Global ability catalog — `EnemyAbilityInfos` array |
| `/Game/Characters/Enemies/BP_StarBoss` | ASC `StartupAbilityTags` at subobject index 7 |
