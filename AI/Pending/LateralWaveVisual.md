# LateralWaveVisual — pending

Goal: the lateral wave reads right: its wind-up preview stretches out in the direction the beam will travel, and each firing travels in one of the 4 basic directions, picked at random.
Priority: 1
Docs written: none yet (workflow steps 2-3 when picked up). Owner docs: `Source/GeoTrinity/Public/AbilitySystem/Abilities/Pattern/Hazard/CLAUDE.md` ("LateralWave"), `AI/Materials.md` ("Safe spots on a beam"), `AI/Tutorial/Steps.md` (steps 8-9, 10).

## Found (2026-10-28 playtest)
- The wind-up rectangle (`M_ZoneIndicatorRay`, laid by `MoveBeamVfx`) does not extend along the beam's travel: the preview and the moving band disagree.
- `TravelYaw` is fixed per pattern (0 = +X); the user wants a random one of 0 / 90 / 180 / 270 per firing.

## Discuss first (step 2)
- [ ] Random direction: rolled from the pattern seed (`StoredPayload.Seed`, `AI/CodingStyle.md` GAS conventions) so client and server agree; how the wave start point and `TravelDistance` follow a turned wave in a non-square room.
- [ ] Safe spots (`SafeSpotOffsets` X along / Y across): they turn with the wave; the Zones room's `SAFE_WAVE_LANES` and the bot's lane checks (`Path.SafeSpotsRetry`, `Path.MinibossWaveLanes`) follow.
- [ ] Preview: grow the rectangle along the travel (from the start edge) vs show the whole swept area with a direction cue.

## Then
- [ ] Docs, code, materials, `tutorial_waves.py` / `tutorial_encounters.py`, tutorial bot stages, `GeoTrinity.Pattern` test of the yaw roll
