# MCP New Enemy Ability Checklist

Steps required every time a new enemy `GA_` Blueprint ability is created.

1. Add the tag to `Config/Tags/GeoGameplayTags.ini` and **restart the editor** — tags do not resolve in Python
   until then.
2. Run `AI/Python/Ability/new_enemy_ability.py`, filling the variables at the top.
3. **Always ask** whether the ability should join the boss behaviour StateTree before finishing. If yes, wire it
   — see `MCP_StateTree.md` and `AI/Python/Ability/state_tree_edit.py`.

## Constraints

- Every `GA_` CDO carries two asset tags in `AbilityTags`: one under `Ability.Spell.*` and one under
  `Ability.Type.*` (`Special`, `SpecialAlternative`, `Basic`, `Dash`, `Reload`, `Passive`), the type matching
  the input slot it binds to. Missing either trips an ensure when the ability catalog is populated.
- Reparenting an existing ability Blueprint to a different C++ base **clears `AbilityTags`** — re-set both and
  save. Read or re-set them on an existing ability with `AI/Python/Ability/ability_tags.py`.
- On `FGameplayAbilityInfo`, both `AbilityTag` and `AbilityClass` go in through `import_text`
  (`AbilityTag=(TagName="Ability.Spell.X")`); the property setter does not work for the class, and the import
  needs the full `BlueprintGeneratedClass` path prefix rather than the raw path result. Set
  `AbilityDisplayName` and `Description` in the same call.
- The ability tag must be in `BP_StarBoss`'s ASC `StartupAbilityTags` or it is never granted at `BeginPlay`.
  After modifying any subobject property, save the owning Blueprint explicitly.

| Asset | Role |
|---|---|
| `/Game/AbilitySystem/Data/DA_AbilityInfo` | Global ability catalog — `EnemyAbilityInfos` array |
| `/Game/Characters/Enemies/BP_StarBoss` | ASC `StartupAbilityTags` at subobject index 7 |
