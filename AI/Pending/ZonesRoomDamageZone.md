# ZonesRoomDamageZone — pending

Goal: in the Zones (Color) room the hurting zone sits in the middle of the room and wears the Damage colour, so the colour lesson reads at a glance.
Priority: 1
Docs written: none yet (workflow steps 2-3 when picked up). Owner docs: `AI/Tutorial/Steps.md` (step 7), `AI/Python/Tutorial/CLAUDE.md` (`tutorial_encounters.py`).

## Found (2026-10-28 playtest)
- Seen in game: the hurting zone was white and off centre.
- Since then (another session, same day): `tutorial_encounters.py` `ZONE_COLORS["Damage"]` = `Damage`, `ZONE_SPOTS["Damage"]` = room centre; the scene matches. Not yet seen in game.

## Discuss first (step 2)
- [ ] User checks the centred Damage-colour zone in game; then rework scope: the whole step 7 layout (Shield / Heal placement inside the grown zone, growth time, `ColorZoneMaxRadius`).
- [ ] Shield colour: `Override` or its own palette colour (`Tool/Visual/CLAUDE.md`).

## Then
- [ ] Docs (Steps.md step 7), script (`ZONE_COLORS`, point), `encounters_diff.py` first, re-run, bot `Zones` stages if the layout changes (`Tests/Tutorial/CLAUDE.md` coverage)
