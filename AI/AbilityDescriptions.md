# Writing Ability Descriptions

Descriptions live in the plain-text file **`Content/Data/AbilityDescriptions.txt`** — one `[Ability.Spell.X]` section
per ability, keyed by the AbilityTag; `#` lines are comments. The file is re-read on every menu open, so edits show
live without a rebuild, and it is staged into packaged builds (`DirectoriesToAlwaysStageAsUFS` in `DefaultGame.ini`).
An ability with no section falls back to the `Description` field of its entry in the global **AbilityInfo** data asset
(`TriangleAbilities` / `CircleAbilities` / `SquareAbilities` / `SharedAbilities`). They are shown in the pause menu's
**Abilities** view (`UGeoAbilityDescriptionsWidget`), resolved at display time by
`FGameplayAbilityInfo::GetResolvedDescription()` (`AbilitySystem/Data/AbilityInfo.cpp`).

You can edit descriptions either directly in this file or in the AbilityInfo asset — they stay in sync (editor-only).
The asset's `Description` field shows each ability's section text; editing it writes that section back to this file,
and it reloads from the file on open and on every edit (so external edits to other sections are never overwritten).
The file remains the source of truth and is the only thing read at runtime.

Write plain text with `{Token}` placeholders. Tokens are replaced with the **live values configured on the ability**
(CDO + its effect data), so descriptions never drift from balance changes. Never hardcode a number that exists on the
ability — use a token.

## Tokens

| Token | Resolves to |
|---|---|
| `{Cooldown}` | Ability cooldown in seconds (from the cooldown GE) |
| `{FireDelay}` | Effective fire delay in seconds (`GetFireDelay()`) |
| `{Damage}` | Sum of all `FDamageEffectData` amounts in the ability's effect data |
| `{Heal}` | Sum of all `FHealEffectData` amounts |
| `{Shield}` | Sum of all `FShieldEffectData` amounts |
| `{Effects}` | Auto-generated list, one line per effect entry (see below) |
| `{PropertyName}` | Any numeric or `FScalableFloat` UPROPERTY on the ability class, by exact C++ name (e.g. `{MaxSpawnRadius}`, `{DashDistance}`) |
| `{ArrayName}` | Any `TArray<TInstancedStruct<FEffectData>>` UPROPERTY, by exact C++ name — expands like `{Effects}` over that array (e.g. `{BlinkBonusEffect}` on the turret recall) |

An unresolved token stays visible in the text (`{Typo}`) and logs a warning — check the log if a brace shows up in-game.

## Token suffixes

Append one or more suffixes to a token (in any order, e.g. `{X:range:%}`) to change how it renders:

| Suffix | Effect | Applies to |
|---|---|---|
| `:range` | Render the value as a **min-max range** over curve levels 1–10 (`Damage: 12-45`), collapsing to a single number when flat. Use for values whose curve is driven by another system than ability level (e.g. the reload's remaining-ammo power scale). Without it a scalable value shows the single value at the current ability level. Works on scalar tokens **and** on `{Effects}` / array tokens (every value inside them ranges). | any token |
| `:%` | value × 100 → `1.5` becomes `150%` | scalar tokens only |
| `:+%` | (value − 1) × 100 → `1.5` becomes `50%` | scalar tokens only |

**Percentages** — use so a multiplier never shows as a bare `x1.5`. Pick by the sentence: **"X% of"** → `:%` (a value that *is* a fraction/share, e.g. `{SelfHealPercent:%}`, `{MaxDamageMultiplier:%}`); **"increase by X%"** → `:+%` (a multiplier phrased as a bonus, e.g. `{EnemyBounceMultiplier:+%}` for a 1.5× → "50%"). Ranges percent-format both ends (`50-150%`). Scalar tokens = numeric / `FScalableFloat` property, `{Damage}`/`{Heal}`/`{Shield}`, `{Cooldown}`, `{FireDelay}`; `{Effects}` and array tokens ignore `:%`/`:+%` but honor `:range`.

## `{Effects}`

Expands `GetEffectDataArray()` (shared `EffectDataAssets` + inline `EffectDataInstances`) into one line per entry:

- `Damage: 12`, `Heal: …`, `Shield: …` — the value at the current ability level (add `:range` → `Damage: 12-45`)
- Buffs (`FGameplayEffectData`): `DamageBoost: 1.2 for 10s` — named by the `DataTag` leaf (fallback: GE class name),
  magnitude, plus duration when non-zero
- Statuses: `Burn status (30% chance)`
- `Lethal` for lethal entries

Use it whenever the ability applies several effects (e.g. the reload's random buffs) instead of listing them by hand.
Add `{Effects:range}` when the values are driven by another system and you want the full range (as the reload does).

## Examples

Basic attack:
```
Fires a projectile dealing {Damage} damage. Costs 1 ammo.
```

Reload (ammo-scaled buffs — `:range` because the buff strength is driven by remaining ammo, not ability level):
```
Restores your ammo and drops a random buff pickup. The emptier your magazine, the stronger the buff:
{Effects:range}
```

Dash:
```
Dash {DashDistance} units in your movement direction.
```

Note: cooldown is shown next to the ability's name in the UI, not in the description body — don't append `Cooldown: {Cooldown}s.` to descriptions.
