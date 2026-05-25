# TODO

Tasks marked `[x]` without a recurrence tag are permanently done.
Tasks tagged `[recur:daily]` are reset to `[ ]` each day by this automated agent.

## Tasks

- [ ] [recur:daily] For every header modified in the last 25h and every public function that has no comment or a malformed comment: add or fix its Unreal-style JavaDoc header. Rules below.

  **Scope**
  - Run `git diff --name-only HEAD` to get today's modified files. Only open and process `.h` and `.cpp` files from that list — do not scan the whole codebase.
  - Within those files: add comments to public functions that have none, fix comments that are malformed or contradict the current implementation, and update any comment whose intent changed today.
  - Do not touch headers that are already correct.
  - Only comment where the method is publicly declared — not in the implementation.

  **Comment format**

  Simple functions — inline description only (no parameter block needed):
  ```cpp
  /** Checks whether the deployable has expired. */
  bool IsExpired() const;
  ```

  Complex functions — multi-line block:
  ```cpp
  /**
   * Steeps a cup of tea.
   * Does not apply milk. Caller is responsible for milk application.
   *
   * @param Vessel        The cup to steep into. Must not be null.
   * @param Duration      Steep time in seconds. Range: [30, 300].
   * @param Strength      Target strength (0 = weak, 1 = strong).
   * @return              True if the tea was successfully steeped.
   * @warning             Vessel temperature must be above 90°C before calling.
   */
  bool SteepTea(UCup* Vessel, float Duration, float Strength);
  ```

  **Rules**
  - Comments document **intent**, not implementation. Code documents implementation.
  - Parameter block required when parameters have non-obvious units, ranges, or constraints.
  - `@return` only when the return value is not already fully described by the function purpose.
  - `@warning`, `@note`, `@see`, `@deprecated` each on their own line after the main block.
  - Class comments must state: what problem the class solves, and why it was created.

  **Report**
  After completing the task, optionally append a short timestamped comment (usually 1–5 lines, it can be longer if needed.) directly below this bullet. Flag anything worth surfacing: incoherent naming, a function whose comment contradicts its implementation, suspicious patterns, or anything that looks like a latent bug. Skip the report if nothing notable was found.
  Format: `<!-- [YYYY-MM-DD] <findings> -->`

<!-- [2026-05-14] Fixed malformed double-comment on AGeoProjectile::InitProjectileLife (was two separate /** */ blocks; merged @note Server only. into the single block). Fixed typo "Bluperint" and duplicate @param bMustBeDamageable in GetInteractableActors BlueprintCallable comment. Added missing public-method comments across GeoGameplayAbility.h (virtual fire-origin/yaw/seed/time hooks and no-param CreateAbilityPayload overloads), GeoDeployableBase.h (Recall/RecallEffect/ExecuteRecallCue/Explode/ExplodeEffect), GeoAbilitySystemLibrary.h (FillEffectContext, non-template GetAbilityCDO, GetTeamId, 2 GetInteractableActors overloads), and GeoBuffPickup.h (InitInteractable). Notable: the Deployable/CLAUDE.md had an incorrect description of when ExecuteRecallCue fires — the server never fires it directly; clients receive it via OnRep_Expired replication. -->

<!-- [2026-05-15] Added missing Unreal-style JavaDoc headers across all five modified headers. GeoGameplayAbility.h: added comments for OnAvatarSet, ActivateAbility, EndAbility (4-param), InputReleased, GetFireDelay, IsPassive, UpdatePayloadFromTargetData. GeoDeployableBase.h: added comments for GetLifetimeReplicatedProps and BeginPlay. GeoProjectile.h: fixed incorrect "@note Server only." on InitProjectileLife (function is also called on clients for non-pooled replicated projectiles); added comments for GetLifetimeReplicatedProps, BeginPlay, LifeSpanExpired, Tick, OnSphereOverlap, PlayImpactFx. GeoBuffPickup.h: added comments for GetLifetimeReplicatedProps, BeginPlay, Tick. GeoAbilitySystemLibrary.h: added per-function comments to all 12 Blueprint getter and 12 Blueprint setter functions in the context section. -->

<!-- [2026-05-16] Scope: same 5 headers as yesterday (no new developer commits today). All public functions already fully and correctly commented — no additions or fixes needed. -->

<!-- [2026-05-17] Scope: git diff --name-only HEAD returned no .h or .cpp files. No developer commits in the last 25h outside of daily TODO runs. No-op. -->

<!-- [2026-05-18] Scope: git diff --name-only HEAD returned no .h or .cpp files. No developer commits in the last 25h outside of daily TODO runs. No-op. -->

<!-- [2026-05-19] Scope: GeoEnemyAIController.h, STTask_FireProjectileAbility.h, STTask_SelectNextFiringPoint.h, AbilityInfo.h, GeoStateTreeBuilderUtil.h, FatalZonePattern.h (+ CPPs). Added missing public comments to GeoEnemyAIController (constructor + OnPossess), STTask_FireProjectileAbility (EnterState + ExitState), STTask_SelectNextFiringPoint (EnterState). Fixed stale class comment in AbilityInfo.h: PlayersAbilityInfos was deleted as a property in commit cc7b701 but the doc block still referred to it; replaced with GetAllPlayersAbilityInfos(). Fixed GenericAbilityInfos → EnemyAbilityInfos in the same class comment (the field was renamed). GeoStateTreeBuilderUtil.h and FatalZonePattern.h already had accurate comments. -->

<!-- [2026-05-20] Scope: GeoDeployableBase.h, GeoMoiraBeamAbility.h, GeoAbilitySystemLibrary.h, GeoPillar.h (+ CPPs). Added comment for new PushAway() in GeoDeployableBase.h (server-only root-motion push on spawn). Added comment for newly public InitInteractable override in GeoDeployableBase.h. Fixed malformed @param LineHalfWidth in GetInteractableActorsInLine (was blank). Added InitInteractable and GetLifetimeReplicatedProps comments to GeoPillar.h. GeoMoiraBeamAbility.h class comment was already updated by the developer. Notable: IsInBeam() was deleted from GeoMoiraBeamAbility but its impl used BeamLength (not in the header); it was replaced by GetInteractableActorsInLine which reads GeneralSpellDistance from GameDataSettings instead — the Circle/CLAUDE.md had both BeamLength and RadialGrowthPerAbsorbedZone listed as stale field names. -->

<!-- [2026-05-21] Scope: STTask_MoveTo.h, GeoAITask_MoveTo.h (new), FatalZonePattern.h, Pattern.h, GeoDeployableBase.h, GeoMine.h, GeoPillar.h, GeoProjectile.h, GeoGameplayAbility.h, GeoRecallTurretAbility.h, GeoAbilitySystemLibrary.h, Team.h (+ CPPs). Added comment for PrepareMoveToTask() in STTask_MoveTo.h (explains why the override exists). Added comment for FillCueParam() in Pattern.h (hook for subclasses to inject custom cue data). Fixed stale ExecuteCue() comment in GeoDeployableBase.h (was "Fires RecallGameplayCueTag" but function now takes a tag param — generalised in 3f5330b); added StartBlinking() and GetGenericCueParams() comments. Added comments for InitInteractable, ApplyExplodeEffect, and GetLifetimeReplicatedProps in GeoMine.h. GeoAITask_MoveTo.h, GeoPillar.h, GeoProjectile.h, GeoGameplayAbility.h, GeoRecallTurretAbility.h, GeoAbilitySystemLibrary.h, Team.h were already fully commented. -->

<!-- [2026-05-22] Scope: git diff --name-only HEAD returned no .h or .cpp files. No developer commits in the last 25h outside of daily TODO runs. No-op. -->

<!-- [2026-05-23] Scope: GeoGameplayAbility.h, GeoChargeBeamAbility.h, PlayableCharacter.h, GeoChargeBeamGaugeWidget.h (new), GeoDeployChargeGaugeWidget.h, GeoWidgetBuilderUtil.h (new). Added comment for new ApplyChargingCurve() in GeoGameplayAbility.h (easing helper for charge ratio). In GeoChargeBeamAbility.h: fixed malformed GetUpdatedTargetData() comment (said "integer 0–100" but implementation encodes as permillage 0–1000); added missing comments for SetChargeGaugeVisible override, FireGameplayCue, Fire, and OnFireTargetDataReceived. Fixed blank @param bVisible in PlayableCharacter.h::SetDeployChargeGaugeVisibility. Added SetSweetSpotRatios, UpdateVisualChargeRatio, and NativeTick comments to GeoChargeBeamGaugeWidget.h. Re-added NativeTick comment to GeoDeployChargeGaugeWidget.h (was removed in commit 55bf82a). Fixed GeoWidgetBuilderUtil.h::InspectWidgetBlueprint from // to /** */ Unreal JavaDoc style. Notable: the GetUpdatedTargetData comment "0–100" was actively contradicted by the implementation (which uses * 1000.f); the commit message for 55bf82a even said "encoded 0–100" — the actual encoding is 0–1000 (permillage). -->

<!-- [2026-05-24] Scope: git diff --name-only HEAD returned no .h or .cpp files. No developer commits in the last 25h outside of daily TODO runs. No-op. -->

<!-- [2026-05-25] Scope: git diff --name-only HEAD returned no .h or .cpp files. No developer commits in the last 25h outside of daily TODO runs. No-op. -->

- [ ] [recur:daily] Read every `.h` and `.cpp` file changed in the last 25h and update the corresponding `CLAUDE.md` files to stay in sync with the code. End with the CLAUDE.md at the root, ensure Structure is still fine and update what's needed.

  **Scope**
  - Run `git diff --name-only HEAD` to get today's modified files. Only open `.h` and `.cpp` files from that list.
  - For each changed file, identify its subfolder `CLAUDE.md` (the one sitting next to the `.h` files) and the parent-folder `CLAUDE.md` above it — update both if needed.
  - Only update what actually changed. Do not touch sections unaffected by today's diff.

  **Two-tier CLAUDE.md rule — strictly enforce this:**
  - **Subfolder CLAUDE.md** (lives next to the `.h` files) — this is the full reference. Include: class purpose, lifecycle, override points, key fields, networking rules, and any non-obvious constraints. Keep it concise but complete. Remove lines that are no longer accurate.
  - **Parent-folder CLAUDE.md** (one level up, e.g. `Deployable/CLAUDE.md` vs `Mine/CLAUDE.md`) — one short sentence per subfolder/file, nothing more. It is an index, not a reference. Do not duplicate details already in the subfolder CLAUDE.md.

  **Writing rules**
  - Be concise. Every line must earn its place — delete stale or redundant lines before adding new ones.
  - Document *why* something works the way it does, not *what* the code does (the code documents itself).
  - Use bullet points, not prose paragraphs.
  - Never describe the task, the fix, or the PR in the CLAUDE.md — only the steady-state design.

  **Report**
  After completing the task, optionally append a short timestamped comment directly below this bullet listing which CLAUDE.md files were updated and what changed. Skip if nothing needed updating.
  Format: `<!-- [YYYY-MM-DD] <findings> -->`

<!-- [2026-05-14] Updated: Actor/Deployable/CLAUDE.md — fixed wrong description of ExecuteRecallCue call site (was "Server/owner calls it in Recall()"; actual code only calls it on non-server in Recall(), clients always receive it via OnRep_Expired). Abilities/Pattern/CLAUDE.md — added FatalZonePattern section. Abilities/CLAUDE.md — added Boss/ row to subfolder table and FatalZonePattern to Pattern row. Created Abilities/Boss/CLAUDE.md for GeoDelayedFatalZoneAbility. Abilities/Circle/CLAUDE.md — updated GeoChargeBeamAbility to document server-side hit detection (no projectile), sweet-spot logic, and FireGameplayCue behaviour. Root CLAUDE.md — added Boss/ to source structure. -->

<!-- [2026-05-15] Updated: Actor/Projectile/CLAUDE.md — corrected InitProjectileLife Lifecycle entry from "server only" to accurately reflect that it is called by pool Init() on both machines AND in BeginPlay on clients for non-pooled replicated projectiles. Actor/Deployable/CLAUDE.md — corrected Lifecycle point 5 for Recall(): clarified that ExecuteRecallCue() is only called on non-server machines (safety path), not unconditionally. No other CLAUDE.md changes needed; all other documented behavior was accurate. -->

<!-- [2026-05-16] Scope: same 5 headers as yesterday. All CLAUDE.md files already accurate — no updates needed. -->

<!-- [2026-05-17] Scope: no .h or .cpp files in git diff --name-only HEAD. No developer commits in last 25h. No CLAUDE.md updates needed. -->

<!-- [2026-05-18] Scope: no .h or .cpp files in git diff --name-only HEAD. No developer commits in last 25h. No CLAUDE.md updates needed. -->

<!-- [2026-05-19] Updated: AbilitySystem/Data/CLAUDE.md — GenericAbilityInfos → EnemyAbilityInfos (field renamed in cc7b701). Tool/CLAUDE.md — added GeoStateTreeBuilderUtil section (new file). Root CLAUDE.md — removed defunct AI/Tasks/ row (BTTask files deleted in 83cb25d), added GeoStateTreeBuilderUtil to Tool/ comment. AI/StateTree/CLAUDE.md and Pattern/CLAUDE.md were already accurate. -->

<!-- [2026-05-20] Updated: Abilities/Circle/CLAUDE.md — GeoMoiraBeamAbility section: removed stale BeamLength and IsInBeam() (both deleted in this commit), renamed RadialGrowthPerAbsorbedZone → HalfWidthGrowthPerAbsorbedZone, added DamagePerSecond/HealPerSecond/SpeedBuffEffect fields and note on GetInteractableActorsInLine usage. AbilitySystem/Lib/CLAUDE.md — added ExtraFilter overload and GetInteractableActorsInLine to team-utilities section. Actor/Deployable/CLAUDE.md — added PushAway() spawn-push helper section and bPushActorsOnSpawn to key fields. Actor/Deployable/Pillar/CLAUDE.md — added bPushActorsOnSpawn=true point and PillarData COND_InitialOnly replication note. -->

<!-- [2026-05-21] Updated: Abilities/Pattern/CLAUDE.md — FatalZonePattern section: renamed ZoneSize → SpawningZoneSize, added PillarParams (FDeployableDataParams forwarded into the spawned pillar, replaces hardcoded size), removed stale PillarEffectDataArray (no longer in header). Actor/Deployable/CLAUDE.md — fixed stale function names in the VFX/Gameplay Cue rule: ExecuteRecallCue() → ExecuteCue(RecallGameplayCueTag, ...) and OnRep_Expired() → OnRep_Active() (both renamed in 3f5330b). AI/StateTree/CLAUDE.md was already updated in 3f5330b with the STTask_MoveTo and UGeoAITask_MoveTo sections. -->

<!-- [2026-05-22] Scope: git diff --name-only HEAD returned no .h or .cpp files. No developer commits in the last 25h outside of daily TODO runs. No CLAUDE.md updates needed. -->

<!-- [2026-05-23] Updated: Abilities/Circle/CLAUDE.md — GeoChargeBeamAbility section: corrected Seed encoding from "integer 0..100" to "integer permillage 0..1000", updated SweetSpotMinRatio default 0.6 → 0.5, added note on FireGameplayCue cue params and SetChargeGaugeVisible override. Abilities/Base/CLAUDE.md — FireMode section: added ApplyChargingCurve note and SetChargeGaugeVisible virtual hook documentation. HUD/CLAUDE.md — added GeoChargeBeamGaugeWidget.h row. Characters/CLAUDE.md — updated PlayableCharacter.h row to include charge-beam gauge. Tool/CLAUDE.md — added GeoWidgetBuilderUtil.h section. -->

<!-- [2026-05-24] Scope: git diff --name-only HEAD returned no .h or .cpp files. No developer commits in the last 25h outside of daily TODO runs. No CLAUDE.md updates needed. -->

<!-- [2026-05-25] Scope: git diff --name-only HEAD returned no .h or .cpp files. No developer commits in the last 25h outside of daily TODO runs. No CLAUDE.md updates needed. -->
