# HexDeployAnims — pending

Goal: the hex boss turret arrives by unfolding from a small square block like a transforming robot, the hex boss mine blooms from a closed hexagon; the turret's deploy sound is its own, so it is known by ear.
Priority: 2
Docs written: `AI/Python/Boss/CLAUDE.md` ("Deploy clips"), `AI/SoundCreation.md` (score table row), `AI/Python/Audio/CLAUDE.md`.

## Scripts
- [ ] `AI/Python/Boss/Hex/Mesh/generate_hex_turret_mesh.py` — split into 4 body pieces + cannon (R=33, cut at x=±R/2, rectangle split on y=0); re-run to rebuild `SM_HexTurret`.
- [ ] `AI/Python/Boss/Hex/Mesh/generate_hex_mine_mesh.py` — each spike its own convex prism; re-run to rebuild `SM_HexMine`.
- [ ] `AI/Python/Boss/Hex/Mesh/rig_hex_turret.py` — new; `SKM_HexTurret` on `SK_HexTurret` (bones in the doc), weights per piece. Model on `rig_hex_boss.py`.
- [ ] `AI/Python/Boss/Hex/Mesh/rig_hex_mine.py` — new; `SKM_HexMine` on `SK_HexMine`, `Core` slot kept.
- [ ] `AI/Python/Boss/Hex/Anim/hex_turret_deploy.py` — new; `AS_HexTurret_Deploy` (~0.8 s) + `AM_HexTurret_Deploy` in the `Cast` slot group. Run `Anim/verify_badge_scripts.py` first.
- [ ] `AI/Python/Boss/Hex/Anim/hex_mine_deploy.py` — new; `AS_HexMine_Deploy` (~0.5 s) + `AM_HexMine_Deploy`, `Cast` slot group.
- [ ] `AI/Python/Audio/hex_turret_deploy_score.py` — new; hiss + telescoping ratchet + two-latch lock, built from `mech_kit.py` but not its clamp / crack / tick one-shots; `ship` puts the notifies on `AM_HexTurret_Deploy`. Judge against the intro with `compare_sections.py` that the sound stays distinct.

## Editor / Blueprint (by hand or script)
- [ ] Hex boss turret BP — mesh = `SKM_HexTurret`, `InitMontage` = `AM_HexTurret_Deploy`, `InitDuration` ≥ 0.8 s from the spawner (`GeoSpawnOnTileAbility` `DeployableParams`).
- [ ] Hex boss mine BP — mesh = `SKM_HexMine`, `InitMontage` = `AM_HexMine_Deploy`, `InitDuration` ≥ 0.5 s; blinking material still on the `Core` slot.
- [ ] Both BPs: fire socket (turret) / any mesh reference moved onto the skeletal mesh; collision and hitbox unchanged.
- [ ] Spawner `InitDuration` — set to the clip lengths above on the hex boss turret / mine spawn abilities.

## Verify
- [ ] Rig scripts: bone counts, weights, bounds match the old static mesh at rest (report in `AI/Output/`).
- [ ] Clip silhouette check from above (`MCP_Animation.md`, "Checking a clip's silhouette"): frame 0 reads as a small square (turret) / closed hexagon (mine); last frame equals the old static look.
- [ ] Tutorial gate: `GeoTrinity.Tutorial.FullRun` only if `Source/` changes (none planned).
