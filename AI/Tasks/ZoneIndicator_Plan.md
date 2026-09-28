# Zone Indicator — Plan

Round zone telegraph (`NS_Round_ZoneIndicator`) moves off the gameplay cue and into a local C++ helper, so it can draw
meaning colours (Color + SecondaryColors) taken straight from what it announces, and so gameplay-critical telegraphs
never ride an unreliable cue multicast.

## Findings (why)

- `GC_ZoneIndicator` (`/Game/AbilitySystem/GameplayCues/`, GameplayCueNotify_Static, `OnExecute`) only writes
  `User.Color` via `GetPaletteColorFromIndex(GameplayEffectLevel)` — never `GeoASLib::SetCueMeaningColors`, so
  secondary colours are ignored even when authored. It reads RawMagnitude → `User.Radius`, Normal.X → `User.Lifetime`,
  Normal.Y → AdvanceSimulationByTime (late catch-up); attaches to SourceObject's root if it is an actor, else spawns at
  Location.
- Colour is authored twice: the cue's `FGeoCueParam` beside the owner's own `FDeployableDataParams` Color/SecondaryColors.
  A cue also cannot carry `EGeoColor::Override`.
- GAS `ExecuteGameplayCue` is an **unreliable** multicast; late joiners never see it; deployable ASCs are Minimal and
  skip the deployer's client. Beams work because `UGeoBeamVFXComponent` replicates state (BeamState/BeamSystem/BeamColors).
- `PatternStartMulticast` (on `UGeoAbilitySystemComponent`) is **reliable** → a pattern runs on every client and can
  spawn its telegraph locally.

Assets firing `GameplayCue.Generic.ZoneIndicator` (9):
- `GA_Tutorial_{BurstDamage,BurstHeal,DamageOverTime,DamageReduction,HealOverTime,Lethal}` — `UGeoZoneAbility`
  (ServerOnly), `TelegraphCue`. **The only case needing cue replication.**
- `BP_SpawnPillarPattern` — `USpawnPillarPattern::ExecuteGameplayCue` (local, per zone location).
- `BP_HexBomb_Deployable`, `BP_Pillar_Deployable` — deployable cue slot (Spawn/Blinking?, to confirm), local.

## Design

**`FGeoZoneIndicatorState`** — one indicator's full state: Location, Radius, Color (`FGeoColorParam`),
SecondaryColors (`TArray<FGeoColorParam>`), StartServerTime, Duration.

**Helper** (GeoLib or a small `GeoZoneIndicator` namespace):
- `UNiagaraComponent* SpawnZoneIndicator(WorldContext, State, AttachTo = nullptr)` — spawns
  `UGameDataSettings::RoundIndicatorSystem` (new, mirrors `RayIndicatorSystem`), applies state, advances simulation by
  `ServerNow - StartServerTime`. Returns null on dedicated server.
- `ApplyZoneIndicatorState(Component, State)` — radius, lifetime, `GeoNiagaraParams::SetMeaningColors`. Reused for
  updates (moving aim, resize).

**Several overlapping indicators:** no custom ID system.
- Local owners hold their components (pattern → array, deployable → one) and destroy them on end.
- Future replicated case (allies see a player's aim) → `UGeoZoneIndicatorComponent` on the character with a
  `FFastArraySerializer` of states; the item's ReplicationID is the identity, Post/PreReplicated callbacks
  spawn/update/destroy. Progress comes from each state's own StartServerTime, so overlaps never interfere.
  **Not in this pass** — waiting on the design call.

**Keep `GC_ZoneIndicator`** only as a cosmetic wrapper for specialised telegraphs (make it call the helper /
`SetCueMeaningColors`).

## Steps

1. Helper + `FGeoZoneIndicatorState` + `RoundIndicatorSystem` setting (DefaultGame.ini →
   `/Game/Art/VFX/Generic/Niagara/NS_Round_ZoneIndicator`).
2. Tutorial abilities → pattern: new zone pattern (Pattern subclass) doing what `UGeoZoneAbility` does —
   init: resolve location on the server (TargetPointTag + Offset, ship in pattern data like SpawnPillar's
   CreatePatternData) and spawn the indicator locally with the zone's own colours for FireDelay; start: burst (hazard
   in circle, `bHasHazard`) or server-spawn the zone deployable (`LifeDrainMaxDuration > 0`); end: destroy indicators.
   Migrate the 6 `GA_Tutorial_*` onto a `PatternAbility` + this pattern. Before migrating, check whether each one's
   `TelegraphCue.Color` matches `ZoneParams.Color` (a mismatch changes its look). Delete `UGeoZoneAbility` if nothing
   else uses it.
3. `USpawnPillarPattern`: replace the per-location cue with the helper (colours from `PillarParams`).
4. Deployables (HexBomb, Pillar): replace the zone-indicator cue with the helper, colours/size from `Data.Params`;
   destroy the indicator if the deployable ends early.
5. Rewire `GC_ZoneIndicator` (or leave for specialised uses), update `AI/VFX.md` (lines ~20, ~313).
6. Later / design call: `UGeoZoneIndicatorComponent` (replicated, fast array) for a player's aim seen by allies;
   `ADeployableSpawnerProjectile::Params` would need `Replicated` for the in-flight stage.

## Rules for the dev session

- Read `AI/CodingStyle.md` before any code; read the public-folder CLAUDE.md of each touched class; read whole cpp files.
- Never build — hand over unverified code and ask the user to build.
- Asset edits over MCP: read `AI/MCP/CLAUDE.md` first.
