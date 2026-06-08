# MCP Level Editing

Editing per-level settings (World Settings, GameMode override, PlayerControllerClass) via `execute_script`.

## Key Constraints

- Load the level first and save it after; World Settings edits are not persisted on their own.
- World Settings is the level's `AWorldSettings` instance, read off the editor world.
- A `GameMode` override is a World Settings property; `PlayerControllerClass` is a property on the GameMode CDO.
- Make "where this happens" a per-map decision by adding a UPROPERTY to the project's `AWorldSettings` subclass and reading it at runtime.
- Class-valued properties take a Blueprint's generated class.

## Calling from Python

See `AI/Python/level_settings.py` for the load → set → save pattern.
