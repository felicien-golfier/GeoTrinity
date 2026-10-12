# GemsSurviveDeath — pending

Goal: a dead player keeps every gem effect (stats GE + Core passives), so what outlives them (deployables, zones) still uses their gems; only abilities with something live stop, each one deciding via `OnOwnerDeath()`.

Implemented and tested (`GeoTrinity.Gems.StatsSurviveDeath`, `GeoTrinity.Abilities.OwnerDeath`). Delete this file once the item below is settled.

## Left
- [ ] Rez bug (passive missing after rez): maybe not death-related; look again now that the stats test is green.
