# PassiveBar — pending

Goal: show every granted passive (incl. Core gem passives) as a small icon tile with cooldown sweep + bright-when-ready frame, left of the ability bar, two stacked per column; status effects keep the right side.
Docs written: `Source/GeoTrinityUI/Public/HUD/AbilityBar/CLAUDE.md`, `Source/GeoTrinity/Public/AbilitySystem/Abilities/Gem/CLAUDE.md`, `AI/Python/Ability/CLAUDE.md`.

Built and scripted (editor build OK, `gem_core_passives.py`, `import_icons.py`, `passive_bar.py` run headless; assets saved, not committed). Left: checks that need a running game.

## Verify (needs PIE — only when the user asks)
- [ ] Bridge / layout audit of `WBP_AbilityBar` with 6 passives: 3 columns, no overlap with status bar (`Python/Runtime/pie_layout_audit.py`); tile size `TILE` in `passive_bar.py` is a 50 px estimate of (ability icon + key label − gap) / 2 — retune against the real key label height.
- [ ] Wake / Split / Kinship: cooldown sweep on the tile after a trigger; no trigger before it ends.
- [x] A passive of a class whose `AbilityInfo` entry has no `AbilityIcon` draws an empty tile: fill any such entry.
