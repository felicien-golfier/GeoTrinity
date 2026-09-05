# Writing Ability Descriptions

Descriptions live in `Content/Data/AbilityDescriptions.txt` — one `[Ability.Spell.X]` section per ability keyed
by its AbilityTag, `#` for comments. The file is re-read on every menu open, so edits show live without a
rebuild, and it is staged into packages (`DirectoriesToAlwaysStageAsUFS` in `DefaultGame.ini`). An ability with
no section falls back to the `Description` field of its **AbilityInfo** entry. Both are shown in the pause
menu's Abilities view, resolved by `FGameplayAbilityInfo::GetResolvedDescription()`.

The AbilityInfo asset mirrors the file editor-side: it loads section text when opened and pushes edits back on
save, but never re-reads on edit, so a value just typed is never reset. After changing the file externally,
click **Reload Descriptions From Disc** on the asset. The file stays the source of truth and is the only thing
read at runtime.

Write plain text with `{Token}` placeholders, replaced with the **live values on the ability** (CDO + effect
data), so descriptions never drift from balance changes. Never hardcode a number that exists on the ability.
An unresolved token stays visible (`{Typo}`) and logs a warning.

Cooldown is shown next to the ability's name in the UI — don't append it to the description body.

## Tokens

| Token | Resolves to |
|---|---|
| `{Cooldown}` / `{FireDelay}` | Cooldown seconds (from the cooldown GE) / effective fire delay |
| `{Damage}` / `{Heal}` / `{Shield}` | Sum of all `FDamageEffectData` / `FHealEffectData` / `FShieldEffectData` amounts |
| `{Effects}` | Auto-generated list, one line per effect entry (see below) |
| `{EffectValue}` / `{EffectDuration}` | Magnitude and duration of the first `FGameplayEffectData` entry, as two independent scalars — word them yourself (`{EffectValue:%} boost for {EffectDuration}s`) rather than taking the bundled `{Effects}` line |
| `{PropertyName}` | Any numeric or `FScalableFloat` UPROPERTY on the ability class, by exact C++ name (`{DashDistance}`) |
| `{EffectProperty}` | A single `TInstancedStruct<FEffectData>` UPROPERTY holding an `FGameplayEffectData` — its `Magnitude` |
| `{ArrayName}` | Any `TArray<TInstancedStruct<FEffectData>>` UPROPERTY — expands like `{Effects}` over that array |
| `{A*B}` | Product of two numeric/`FScalableFloat` properties, for a derived cap like per-unit × max-count. Honors suffixes |

## Suffixes

Append in any order (`{X:range:%}`):

| Suffix | Effect | Applies to |
|---|---|---|
| `:range` | Renders as a min-max range over curve levels 1–10 (`Damage: 12-45`), collapsing to one number when flat. Use where the curve is driven by something other than ability level (the reload's remaining-ammo scale). Without it a scalable value shows only the current level | any token, `{Effects}` and arrays included (every value inside them ranges) |
| `:%` | value × 100 → `1.5` becomes `150%` | scalar tokens only |
| `:+%` | (value − 1) × 100 → `1.5` becomes `50%` | scalar tokens only |

Pick the percent form by the sentence: **"X% of"** → `:%` (a value that *is* a share); **"increase by X%"** →
`:+%` (a multiplier phrased as a bonus). Ranges percent-format both ends. Scalar tokens are the numeric /
`FScalableFloat` properties plus `{Damage}`/`{Heal}`/`{Shield}`/`{Cooldown}`/`{FireDelay}`; `{Effects}` and
array tokens ignore `:%`/`:+%` but honor `:range`.

## `{Effects}`

Expands `GetEffectDataArray()` (shared `EffectDataAssets` + inline `EffectDataInstances`) into one line each:
`Damage: 12` at the current level; buffs as `DamageBoost: 1.2 for 10s`, named by the `DataTag` leaf (fallback:
GE class name) with the duration only when non-zero; statuses as `Burn status (30% chance)`; `Lethal` for
lethal entries. Use it whenever an ability applies several effects instead of listing them by hand.

## Examples

```
Fires a projectile dealing {Damage} damage. Costs 1 ammo.

Restores your ammo and drops a random buff pickup. The emptier your magazine, the stronger the buff:
{Effects:range}

Dash {DashDistance} units in your movement direction.
```
