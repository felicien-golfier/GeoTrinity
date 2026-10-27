# AI/StateTree/Utility

| Task | Role |
|---|---|
| `STTask_SendEventAfterNCycles` | Counts state entries in blackboard `CycleCount`; at `CyclesRequired` sends `EventTag` and resets |
| `STTask_PlayMontage` | Plays a montage through the pawn ASC (replicates; tree is server-only); async finish on `FOnMontageEnded`, `ExitState` stops it if current; `PlayRate` clamped ≥ 0.01 |
| `STTask_CatSpeak` | `AGeoTrainingCat::Speak(Categories)` then Succeeds at once (the line's own length is not awaited; the next one replaces it); `Categories` = `FGeoCatLineCategories` |
| `STTask_SetInvulnerable` | Sets the pawn invulnerable (`bInvulnerable`, default true) while the state runs |
