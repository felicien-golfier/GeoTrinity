# MCP Blueprint Creation — Unreal Python Reference

Practical knowledge for creating and configuring Blueprint assets via `mcp-unreal` `execute_script`.

## Prerequisites

- Unreal Editor must be open before starting Claude Code.
- Tags added to `Config/Tags/GeoGameplayTags.ini` require an **editor restart** to resolve.

## Key Patterns

See `AI/Python/` for call patterns covering:
- Creating a Blueprint asset via `AssetTools` and a factory
- Setting CDO properties via `get_default_object`
- Setting a `FGameplayTag` via `import_text`
- Setting a `TSubclassOf` property
- Accessing and mutating subobject components via `SubobjectDataSubsystem`
- Copying a `TInstancedStruct` from an existing asset
- Moving an asset
- Searching assets by keyword via `AssetRegistry`

**`EditDefaultsOnly` private properties** need `meta=(AllowPrivateAccess="true")` to be accessible from Python. When the property is in project C++ code, add the meta specifier directly — do not use workarounds.

## Naming Conventions

| Asset type | Prefix |
|---|---|
| Gameplay Ability Blueprint | `GA_` |
| Pattern Blueprint | `BP_` |
| Effect Data Asset | `DA_` |
| Widget Blueprint | `WBP_` |

Enemy abilities live under `/Game/AbilitySystem/Abilities/Enemy/`, one subfolder per ability.
HUD widgets live under `/Game/HUD/`. For widget-specific automation see `MCP_UI.md`.
