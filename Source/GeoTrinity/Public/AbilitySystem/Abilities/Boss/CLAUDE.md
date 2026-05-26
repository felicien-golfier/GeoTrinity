# Abilities/Boss

Boss-specific gameplay abilities.

---

## `GeoDelayedFatalZoneAbility.h` — boss fatal zone spawner

Extends `UPatternAbility`. Picks a random player's location server-side as the zone origin, encodes it in the payload `Origin` field, then launches `UFatalZonePattern` on all clients. All clients derive the same zone position deterministically from the payload.

- Overrides `GetFireOrigin2D(Instigator, SourceASC, Seed)` — selects a random player position using seeded RNG; the chosen 2D position becomes the zone center for all clients
- Pattern: `UFatalZonePattern` (see `Pattern/CLAUDE.md`)

---

## `GeoDevastatingWaveAbility.h` — expanding radial wave

Extends `UPatternAbility`. Teleports the boss to `TeleportLocation` by overriding `GetFireOrigin2D` (so the payload `Origin` is the destination), then launches `UDevastatingWavePattern`.

- Overrides `GetFireOrigin2D` — returns `TeleportLocation`; teleport happens in `UDevastatingWavePattern::StartPattern`
- Effect data configured on the ability via `EffectDataAssets`/`EffectDataInstances`; forwarded automatically to the pattern by `PatternAbility`
- Pattern: `UDevastatingWavePattern` (see `Pattern/CLAUDE.md`)
