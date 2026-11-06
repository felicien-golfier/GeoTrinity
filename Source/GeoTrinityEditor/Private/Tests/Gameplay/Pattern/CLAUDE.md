# Tests/Gameplay/Pattern

Pattern and targeting tests (`Lib/CLAUDE.md`, "Targeting").

| File | Covers |
|---|---|
| `GeoPatternTests.cpp` | Safe spot maths: `Behind` / `Lane` / `Disc` shadows, mask slots; lateral wave roll; salve aim per `EGeoTarget`; beam yaw along the sweep; zone with several locations; pillar release / expiry; lingering zone until stopped, zone judged only from its landing |
| `GeoTargetTests.cpp` | Each mode's locations, seeded picks, health steps, scatter, follow / gone actor, empty = nothing fires, `FightCenter` point + `GetCenter()` + `RequiresArena`; patterns: payload = caster, targets apart, `GetTargetYaw` |
| `GeoClientSafeSpotTests.cpp` | Safe spot mask as a client uploads it (world set up as the server, then answering `NM_Client`) against the listen-server control: lateral wave fixed spots / live adds, devastating wave pillars |
| `GeoWaveHitTests.cpp` | Lateral wave hit test over the Zones room's spots (sampled at the judge's rate), three firings of one instance: no lane = every spot hit, lanes = out of them hit and in them safe (`WaveHits.*`) |
| `GeoZoneLayerTests.cpp` | Zone layer: slot packing (circle, turned + offset beam, timed fill), colour mask, registry order / hide / overflow, telegraph (`SpawnIndicator`) removing itself after its `Duration` |
