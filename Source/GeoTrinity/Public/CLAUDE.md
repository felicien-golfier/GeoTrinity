# GeoTrinity module (Public)

Gameplay module. Headers in `Public/`, `.cpp` mirrored in `Private/` at the same relative path. Includes are written from `Public/` (`#include "Actor/Arena/GeoArena.h"`).

| Folder | Holds | Doc |
|---|---|---|
| `AbilitySystem/` | GAS: abilities, attributes, effect data, library, tags | `AbilitySystem/CLAUDE.md` |
| `Actor/` | Arenas, buttons, deployables, projectiles | `Actor/CLAUDE.md` |
| `AI/` | Enemy controller, blackboard, StateTree tasks | `AI/CLAUDE.md` |
| `Animation/` | Montage notifies | `Animation/CLAUDE.md` |
| `Characters/` | Player + enemy characters, components, class data | `Characters/CLAUDE.md` |
| `GameClasses/` | GameMode / State / PlayerState / Controller / viewport | `GameClasses/CLAUDE.md` |
| `Gem/` | Gem progression | `Gem/CLAUDE.md` |
| `HUD/Interface/` | Gameplay↔UI interfaces | `HUD/Interface/CLAUDE.md` |
| `Input/` | Enhanced Input bridge | `Input/CLAUDE.md` |
| `Settings/` | Developer + user settings | `Settings/CLAUDE.md` |
| `System/` | Pooling, stats, session, bullets | `System/CLAUDE.md` |
| `Tool/` | `GeoLib`, colour, icons, Niagara names, lag compensation | `Tool/CLAUDE.md` |
| `World/` | Camera, volumes, backdrop, world settings | `World/CLAUDE.md` |

Private-only: `Private/Tests/GeoGemTests.cpp` (`AI/Testing.md`).
