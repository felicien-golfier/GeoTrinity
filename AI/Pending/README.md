# AI/Pending

One **temporary** file per feature: everything only this feature's implementation needs — one-off steps, sources, assets to fetch, values to copy, Blueprint / editor work. Lasting design rules go in the owning folder's `CLAUDE.md` instead. Created in workflow step 3 (root `CLAUDE.md`), consumed in step 4, **deleted when empty**.

## Template: `AI/Pending/<Feature>.md`
```
# <Feature> — pending

Goal: one line — the Why.
Priority: 1 | 2 | 3 — 1 = next to pick up; listed in "Order" below.
Docs written: list of CLAUDE.md files that describe it.

## Code (to implement)
- [ ] path/File.h/.cpp — what it must do (names from the CLAUDE.md)

## Scripts
- [ ] AI/Python/<Topic>/<script>.py — what it builds; how to run

## Editor / Blueprint (by hand or script)
- [ ] Asset `/Game/...` — property = value, component, node to add

## Verify
- [ ] How to see it work (test, bridge script, PIE only if asked)
```

- Concrete asset paths, property names, values; no prose.
- A step an editor script can do becomes a script row; the rest stays here for the user.

## Order
Every Pending file carries `Priority:` (1 high, 2 normal, 3 later). This list is sorted by it: a new file, or a changed priority, moves its row in the same change; a deleted file leaves it.

| Priority | File |
|---|---|
| 1 | `ZonesRoomDamageZone.md` |
| 1 | `LateralWaveVisual.md` |
| 1 | `TutorialMinibossRework.md` |
| 1 | `Tutorial.md` |
| 2 | `DashVoidEdge.md` |
| 2 | `TutorialPolish.md` |
| 2 | `QuestDummies.md` |
| 2 | `TutorialExit.md` |
| 2 | `TutorialTeleporters.md` |
| 2 | `TutorialAggroRadius.md` |
| 2 | `TrainingCat.md` |
| 2 | `EncounterHandEdits.md` |
| 2 | `TestAim.md` |
| 2 | `PendingSweep.md` |
| 2 | `GemShapes3D.md` |
| 2 | `GemEcho.md` |
| 2 | `SoundConcurrency.md` |
| 2 | `TeleportPortal.md` |
| 2 | `CatLineTypewriter.md` |
| 2 | `LoadingScreen.md` |
| 2 | `CatExpressions.md` |
| 3 | `CatAnimation.md` |
| 3 | `CatClipsPick.md` |
| 3 | `CatEvilFace.md` |
| 3 | `CatLocomotionIdles.md` |
| 3 | `CatPortrait.md` |
| 3 | `CatWalkRun.md` |
| 3 | `EchoSocketHalo.md` |
| 3 | `EchoUiPolish.md` |
| 3 | `MaterialFunctionDedup.md` |
| 3 | `MenuGuide.md` |
| 3 | `MessageLog.md` |
