# AI/Python/Ability

Ability Blueprints, the ability catalog, StateTrees. Docs: `AI/MCP/MCP_NewEnemyAbility.md`, `MCP_StateTree.md`.

| Script | Does |
|---|---|
| `new_enemy_ability.py` | Create an enemy ability Blueprint and register it in the catalog |
| `ability_info_icons.py` | Normalise the catalog's player entries (deploy-count flag) |
| `ability_tags.py` | Read / re-set the asset tag container of an ability Blueprint |
| `boss_behaviour_trees.py` | Split boss StateTrees into the shared base + one tree per boss |
| `state_tree_edit.py` | StateTree states, tasks, transitions through the builder shim |
| `deploy_target_cue.py` | Deploy landing marker: ring plane on an `AGeoDeployTargetCue` BP |
| `triangle_momentum_passive.py` | Triangle stacking damage-boost passive, end to end |
| `gem_core_passives.py` | Core gems' passives: BP per Core, `AbilityInfo` entry (Shared, not at startup), `GrantedAbility` cell of `Data/gems.csv`, `GE_CD_Core_<Core>` cooldown GE for Wake / Split / Kinship; headless `-run=pythonscript` |
