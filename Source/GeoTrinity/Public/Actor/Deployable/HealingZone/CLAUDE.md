# Actor/Deployable/HealingZone

`GeoHealingZone.h` — Circle area heal.

- `bUseRegularDrain`: fixed drain always, plus `DrainMagnitudePerSecond × dt × HealedNum` server-side per healed target.
- `ActorsInZone` tracks overlaps; server `Tick()` heals allies below full (`FHealEffectData`).
- `GetDurationPercent()` = health ratio; `OnRep_Data()` syncs capsule size on clients.
