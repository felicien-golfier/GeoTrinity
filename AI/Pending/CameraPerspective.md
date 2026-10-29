# CameraPerspective — pending

Goal: switch the game camera between orthographic and perspective at runtime with a console command, with zoom, framing, backdrop and mouse aim working in both.
Priority: 2
Docs written: `Source/GeoTrinity/Public/World/Camera/CLAUDE.md` ("Projection"), `Source/GeoTrinity/Public/World/Backdrop/CLAUDE.md`, `Source/GeoTrinity/Public/Input/CLAUDE.md`, `Source/GeoTrinity/Public/Tool/CLAUDE.md` (GeoLib), `Source/GeoTrinityEditor/Private/Tests/Gameplay/CLAUDE.md`, root `CLAUDE.md` + `AI/ArtDirection.md` (ortho-by-default rule).

## Code (to implement)
- [ ] `World/Camera/GeoGameCamera.h/.cpp`
  - CVar `Geo.CameraPerspective` (bool, 0) next to `Geo.ShowCameraZoom`.
  - `UPROPERTY PerspectiveFOV = 30.f` (Category `GeoCamera|Projection`, ClampMin 5, ClampMax 120).
  - `static float GetPerspectiveDistance(float Width, float FOVDegrees)` (public, pure).
  - `BeginPlay`: capture `OrthoFloorDistance` = actor Z / −`GetCameraComponent()->GetForwardVector().Z` (ensure forward points down).
  - `Tick`: first thing, compare CVar to `bPerspective`; on change → `ApplyProjection()` (`SetProjectionMode`, `SetFieldOfView(PerspectiveFOV)`, reset offset in ortho).
  - `UpdateZoom`: after `SetOrthoWidth`, in perspective → `ApplyViewOffset()`: component relative location = −local forward × (`GetPerspectiveDistance(CurrentOrthoWidth, PerspectiveFOV)` − `OrthoFloorDistance`), then `Backdrop->ApplyViewOffset(Offset, PerspectiveFOV)`.
  - Update header comments that say "Orthographic follow camera" / "orthographic projection gives…".
- [ ] `World/Backdrop/GeoBackdropComponent.h/.cpp` — keep the created planes + their authored relative locations; `ApplyViewOffset(float Offset, float FOV)`: plane i at authored distance + Offset, side = max(LayerSize, 2·d·tan(FOV/2)) (square plane: the width is the larger extent at aspect ≥ 1), scale = side / mesh size. No-op on dedicated server / `!Active`.
- [ ] `Tool/UGeoGameplayLibrary.h/.cpp` — `GeoLib::IntersectRayWithZPlane(FVector Origin, FVector Direction, float Z, FVector2D& OutXY) -> bool` (`GEOTRINITY_API`, test uses it).
- [ ] `Input/GeoInputComponent.cpp` `UpdateMouseLook` — aim point via `IntersectRayWithZPlane` at the pawn's Z; keep old behaviour on false (no update).
- [ ] `GameClasses/Player/GeoPlayerController.cpp` `Geo.DestroyTileUnderMouse` — replace its inline ray/plane maths with the GeoLib helper.

## Tests
- [ ] `Source/GeoTrinityEditor/Private/Tests/Gameplay/GeoCameraTests.cpp`
  - `GeoTrinity.Camera.PerspectiveDistance` — width seen at returned distance equals input width (FOV 30, 60, 90).
  - `GeoTrinity.Camera.RayToZPlane` — straight-down ortho ray, slanted perspective ray, parallel ray → false.
  - `GeoTrinity.Camera.ProjectionSwitch` — test world (`Fake/GeoTestWorld.h`), spawn `AGeoGameCamera`, set CVar 1, tick → Perspective + component offset > 0; CVar 0, tick → Orthographic + offset 0. Restore CVar at the end.
- [ ] Run: `Tools\Run_Tests.bat GeoTrinity.Camera`, then the tutorial gate `GeoTrinity.Tutorial.FullRun` (root `CLAUDE.md`).

## Verify (user, by eye — not automated)
- [ ] `Geo.CameraPerspective 1` in PIE: same floor point centred, zoom wheel / volumes / coop spread move the camera, stars still behind the floor, aim follows the cursor.
- [ ] Outline post-process (`MI_DeployableOutline`) and any depth-reading material look right in perspective; note fixes here if not.
