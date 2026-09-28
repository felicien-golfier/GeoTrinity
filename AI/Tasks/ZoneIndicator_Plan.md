# Zone Indicator — Plan

Telegraphs (round and ray) leave the gameplay cue for C++: a player's through the replicated
`UGeoIndicatorComponent`, anything already running on every machine through its static `SpawnIndicator`. Both draw
meaning colours straight from the owner, and no gameplay-critical telegraph rides an unreliable cue multicast.

## Findings (why)

- `GC_ZoneIndicator` only writes `User.Color` (`GetPaletteColorFromIndex(GameplayEffectLevel)`), never
  `GeoASLib::SetCueMeaningColors`, so secondary colours are ignored.
- A replicated `ExecuteGameplayCue` is an unreliable multicast; late joiners never see it. `UGeoZoneAbility`
  (server-only, `GA_Tutorial_*`) telegraphed that way.
- A pattern runs on every machine (reliable `PatternStartMulticast`); a deployable's cues run locally from replicated
  state — both already reliable.
- Telegraph colour is the meaning of what lands, not the owner's tint: the pillar blinks in Damage while its
  `Params.Color` is `DeployableBlockingAllies`; `BP_SpawnPillarPattern` telegraphs in Damage too. So deployables and
  SpawnPillar keep their local cue (right colour source, already reliable) — only the cue Blueprint needs fixing.
- Every `GA_Tutorial_*` lands on `TargetPoint.Tutorial.<Name>` in its arena (zero `Offset`), FireDelay 1,
  BurstAttitude 4. The arena point lookup moved to `UPatternAbility::TargetPointTag`, folding
  `UGeoDevastatingWaveAbility` (deleted; `GA_DevastatingWave` is a plain `UPatternAbility` on `TargetPoint.BossSpawn`). Burst ones
  (`BurstHeal`, `BurstDamage`) have `ZoneParams.Color = Override` (white) — the colour lived on `TelegraphCue` only
  (Heal / Damage). Lingering ones match their `TelegraphCue` colours (DamageReduction + DamageBoost secondary).

## Done (code, unbuilt)

1. `UGameDataSettings::RoundIndicatorSystem` + `DefaultGame.ini` entry.
2. `UGeoIndicatorComponent` (`Characters/Component/`), default subobject on `AGeoCharacter`: fast array of
   `FGeoIndicatorState` (Shape Round/Ray, Location or owner-attached, Size, Colors, StartServerTime, Duration),
   `COND_SkipOwner` — the owner adds its own from the same predicted path. `AddIndicator` / `RemoveIndicator` (handle =
   ReplicationID, per machine), server clamps StartServerTime to `[now - MaxLatencyCompensation, now]`, expiry by
   timer. Static `SpawnIndicator` = the one drawing function (catch-up via `AdvanceSimulationByTime`).
3. `UGeoChannelBeamAbility` windup → Ray on the indicator component; `UGeoBeamVFXComponent` lost `bIsIndicator` /
   `IndicatorSystem` (live beam only).
4. `UZonePattern` (`Pattern/`): circle at payload origin in `ZoneParams` colours for the wind-up; burst = base hazard
   (`TeamAttitude`, `StartCue`), lingering = server spawns the zone and every machine ends.

5. Done: the 6 `GA_Tutorial_*` are `UPatternAbility` children launching `BP_ZonePattern_*` (same folder), with
   `TeamAttitude = 4` and `StartCue = BurstCue`; `UGeoZoneAbility` is deleted. `UGeoZoneIndicatorCue` (native,
   draws through `SpawnIndicator` with the cue's meaning colours) replaces `GC_ZoneIndicator`'s Blueprint graph.

6. Done: `ZoneClass` / `ZoneParams` filled on the 6 patterns (bursts take their old telegraph colour, Heal / Damage);
   `GC_ZoneIndicator` reparented onto `UGeoZoneIndicatorCue`, its `OnExecute` graph removed, cue tag kept.

## Left

7. Test in PIE (listen server + client): tutorial zones, channel beam windup, pillar / hex bomb blink telegraphs.
8. Player abilities needing a warning for allies (deploy aim; `ADeployableSpawnerProjectile::Params` would need
   `Replicated` for the in-flight part) — design call pending.
