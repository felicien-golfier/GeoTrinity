# AI/Python/Niagara

Niagara systems and emitter stacks. Docs: `AI/VFX.md`, `AI/MCP/MCP_Niagara.md`.

`niagara_stack_edit.py` (builder shim: modules, switches, inputs) is the toolkit the others use.

| Script | Does |
|---|---|
| `niagara_stack_edit.py` | Stack editing: modules, switches, inputs |
| `decode_niagara_parameters.py` | Decode a rapid-iteration store from the property API |
| `meaning_color_bindings.py` | Beam / telegraph systems' meaning-colour User params bound to material params |
| `buff_vfx.py` | Every buff a character or its shots can wear, one style |
| `geo_aura_vfx.py` | Five character auras as geometry |
| `empowerment_vfx.py` / `empowered_aura_vfx.py` | Empowerment aura (ortho view), three auras (electricity: `Electric/empowered_arc_vfx.py`) |
| `teleport_portal_vfx.py` | Teleport portal: intro + hold system and one-shot outro system, one sprite wearing `M_TeleportPortal` |
| `sacrifice_release_vfx.py` / `sacrifice_hole_vfx.py` | Square sacrifice detonation ray / keyhole during the channel |

## Electric/ — lightning, arcs, helices
| Script | Does |
|---|---|
| `charged_trail_vfx.py` | Electric trail behind a damage-boosted shot |
| `cone_coil_vfx.py` | Electric helix around a cone |
| `lightning_variants.py` | Three electric declinations |
| `static_electricity_vfx.py` | Blue bolts around a character |
| `empowered_arc_vfx.py` | Electric arc over the mesh |
