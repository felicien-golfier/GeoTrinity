# GemEcho — pending

Goal: each class opens one extra Echo socket at class level 20 that holds one Chip / Cut / Prism of a build and gives a share of that gem's max per class, the share growing with an Echo level (+1% of one tier per level, cycling Chip / Cut / Prism) fed by the XP earned at class level 20.
Docs written: `Source/GeoTrinity/Public/Gem/CLAUDE.md` ("Echo socket"), `Source/GeoTrinityUI/Public/HUD/Menu/Gems/CLAUDE.md` ("Echo socket (Loadout page)").
Built and tested (editor build, `GeoTrinity.` 17 / 17, loadout page checked in PIE); what is left is the user's call:

## Open points
- [ ] Socket name: "Echo" is the doc's placeholder.
- [ ] XP curve numbers: `Curve_EchoXp` is 1000 XP at level 2, x1.02 a level (6963 at level 100, about 300k XP in all) — tune in the asset.
- [ ] Design doc "Proposal: Echo socket" still shows the old table (33% flat, shard ranks, 100 ranks +1 point); update it to the docs above.
- [ ] Pause-menu "Gems" screenshot / look of the echo socket at the board centre (44 px, drawn as a Core socket): judge and adjust `EchoPosition` / `EchoSize` on `WBP_GemLoadout`.
- [ ] Delete this file once the above are settled.

## Status 2026-10-25
Code and tests exist. Remaining user decisions: socket name, XP curve, design doc 'Proposal: Echo socket' update, look of the socket. Text on socket look is stale (see EchoUiPolish / EchoSocketHalo).
