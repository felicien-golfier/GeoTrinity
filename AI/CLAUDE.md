# AI/

Agent-facing docs, scripts and workspaces. Code maps are in the `CLAUDE.md` files under `Source/`.

## Topic docs
| Context | File |
|---|---|
| Code style, GAS conventions, error handling | `CodingStyle.md` |
| Build commands, dev environment, copyright | `Commands.md` |
| Distributable package | `BuildPackage.md` |
| Automated tests: layers, headless, reports | `Testing.md` |
| Networking, data structures, effect system | `Architecture.md` |
| Ability descriptions (`{Token}` syntax) | `AbilityDescriptions.md` |
| How the game must look: shape / colour / motion | `ArtDirection.md` |
| UI rules: reusable styles, Blueprint-tweakable values | `UI.md` |
| Arena wall cubes | `ArenaWalls.md` |
| Material structure: functions, parameters, graphs | `Materials.md` |
| VFX / Niagara | `VFX.md` |
| Sound effect end to end | `SoundCreation.md` |
| Animation authoring | `MCP/MCP_Animation.md` |

## Folders
| Folder | Purpose |
|---|---|
| `MCP/` | Editor / tool automation rules → `MCP/CLAUDE.md` |
| `Python/` | Scripts by topic → `Python/CLAUDE.md` |
| `Audio/` | Sound workspace → `Audio/CLAUDE.md` |
| `Pending/` | One temporary file per feature: editor / Blueprint work left → `Pending/README.md` |
| `Output/` | Script reports, probes, exports (`Output/README.md`) |
| `Tasks/` | Daily-agent task lists (`TODO.md`, `Tasks.md`) |

## Working rules
- A workflow that proved itself becomes a script in `AI/Python/<Topic>/` (row in that folder's `CLAUDE.md`) and a section in the matching topic doc the same session; never left in the scratchpad.
- A render / export / file the user will look at ends with `explorer.exe /select,"<path>"`; audio is played (`AI/SoundCreation.md`, "Delivering").
- Verify UI before reporting with what needs no mouse (tests, bridge probes, `Python/Runtime/pie_layout_audit.py`); real-click PIE only when the user said to test it yourself (`UI.md`, "Check your own work").
- This folder, the module `CLAUDE.md`s and `TroobleshootingLogs.md` are the only memory: nothing durable lives anywhere else.
