"""
Tutorial miniboss assets (AI/Tutorial/Steps.md step 10), only what holds a look or behaviour (AI/CodingStyle.md,
"Variants are data"): the boss's behaviour tree, the summoned dummy (zone and ally params), the shooter tree of the
summons and allies, the salve (boss, summons: at the players), its ally child (at the boss) and the summon. The boss itself is BP_Dummy + the Miniboss
arena's BossSetup (tutorial_encounters.py); the lateral wave it fires (WAVE_TAG) is built by tutorial_waves.py (run it
first). Deletes the assets of the old layout once nothing references them (re-run after tutorial_encounters.py).
Rows read from CT_Tutorial (tutorial_assets.py): DummyHealth, AllyDummyHealth, DummyLandingDamage, DummyZoneDamage,
DummyZoneGrowthTime, DummyZoneMaxRadius, DummyZoneStartRadius, MinibossProjectileDamage, MinibossProjectileSpeed,
AllyProjectileDamage.
Idempotent: assets keyed by path, properties rewritten; the behaviour tree is rebuilt from the star boss's whole.
Report: AI/Output/tutorial_miniboss_report.txt.
Usage: headless (AI/Commands.md), editor closed or via the bridge; Config/Tags/GeoGameplayTags.ini already holds the tags.
"""
import importlib
import re
import sys
import traceback

import unreal

sys.path.insert(0, "C:/GeoTrinity/AI/Python/Tutorial/Level")
import tutorial_waves as waves  # noqa: E402

importlib.reload(waves)

REPORT_PATH = "C:/GeoTrinity/AI/Output/tutorial_miniboss_report.txt"
ABILITY_DIR = "/Game/AbilitySystem/Abilities/Enemy/TutorialMiniboss"
ATTRIBUTE_DIR = "/Game/AbilitySystem/DefaultAttributes"
CATALOG = "/Game/AbilitySystem/Data/DA_AbilityInfo"
CHARACTER_DIR = "/Game/Characters/Enemies/Tutorial"
ACTOR_DIR = "/Game/Actors/Tutorial"
TREE_DIR = "/Game/AI/Tutorial"
CURVE_TABLE = "/Game/AbilitySystem/Data/CurveTables/CT_Tutorial"
BP_DUMMY = "/Game/Characters/Enemies/Training/BP_Dummy"
BP_DUMMY_ALLY = "/Game/Characters/Enemies/Training/BP_DummyAlly"  # BP_Dummy with the friendly body
SALVE_ABILITY_SRC = "/Game/AbilitySystem/Abilities/Enemy/ProjectileToAll/GA_FireProjectileToAllPlayers"
SALVE_PATTERN_SRC = "/Game/AbilitySystem/Abilities/Enemy/ProjectileToAll/BP_ProjectileToAll_Pattern"
SUMMON_PATTERN_BP = ABILITY_DIR + "/BP_TutorialPattern_Summon"
SUMMON_PATTERN = "/Game/AbilitySystem/Abilities/Enemy/Training/BP_ZonePattern_BurstDamage"
TREE_SRC = "/Game/AI/ST_StarBossBehaviour"
SHOOTER_TREE = "/Game/AI/Dummy/ST_Dummy_Shooter"
SALVE_SHOOTER_TREE = TREE_DIR + "/ST_TutorialSalveShooter"  # ST_Dummy_Shooter firing the salve: summons, allies

SALVE_TAG = "Ability.Spell.TutorialSalve"
ALLY_SALVE_TAG = SALVE_TAG + ".Ally"  # a child tag: the shooter tree firing SALVE_TAG fires the ally's salve too
SUMMON_TAG = "Ability.Spell.TutorialMinibossSummon"
SUMMON_ZONE_TAG = "Ability.Spell.TutorialSummonZone"  # passive of the dummy: its growing zone
SPAWN_DUMMY_TAG = "Ability.Spell.TutorialSpawnDummy"  # sibling of SUMMON_TAG (a child would be fired with it): lands the dummy
SUMMON_ALLY_TAG = "Ability.Spell.TutorialSummonAlly"  # the dummy's ally where it dies (spawn ability, not a passive)
PASSIVE_TAG = "Ability.Type.Passive"
DEFAULT_ZONE_CLASS = "/Game/Actors/BP_Zone.BP_Zone_C"  # Game Data Settings' DefaultZoneClass
SUMMON_ACTOR_TAG = "TutorialMinibossSummon"  # the dummy's actor tag: how the bot and the tests find it
WAVE_TAG = "Ability.Spell.LateralWave"  # tutorial_waves.py: the one wave, lanes from the living adds
WAVE_EVENT_TAG = "Event.StateTree.TriggerMinibossWave"
WAVE_PATTERN_DATA = "/Script/GeoTrinity.LateralWavePatternData(bRandomDirection=True)"  # each firing rolls its direction
POINT_TAG = "TargetPoint.Tutorial.MinibossPoint"
ARENA_TAG = "Arena.Tutorial.Miniboss"
ALLY_ACTOR_TAG = "TutorialMinibossAlly"

SUMMONS_PER_WAVE = 3  # CyclesRequired: Board page 13, tip 3 ("every third Dummy")
SHOOT_START_SECONDS = 0.2  # the salve keeps firing on its own; the state only starts it
SUMMON_WIND_UP = 2.0  # seconds the landing spot is telegraphed and the boss stands still, at every level
SUMMON_MONTAGE = "/Game/Characters/Anim/Star/SK_Star_PikeNova_Montage"  # the Pillar's drop clip, on BP_Dummy's SK_Star
HOSTILE_ATTITUDE = 4  # ETeamAttitudeBitflag: acts on the other team
COLUMN = 1.0  # CT_Tutorial is flat: every difficulty column holds the same value

# The old layout: one Blueprint or effect per variant. Deleted once nothing references them.
OLD_ASSETS = [CHARACTER_DIR + "/BP_TutorialMiniboss", ABILITY_DIR + "/GA_TutorialMinibossShoot",
              ABILITY_DIR + "/GA_TutorialAllyShoot", ABILITY_DIR + "/BP_TutorialPattern_MinibossShoot",
              ABILITY_DIR + "/BP_TutorialPattern_AllyShoot", ABILITY_DIR + "/BP_ZonePattern_MinibossSummon",
              ATTRIBUTE_DIR + "/GE_TutorialMinibossAttributes", ATTRIBUTE_DIR + "/GE_TutorialSummonedDummyAttributes", CHARACTER_DIR + "/BP_TutorialSummonedDummy",
              ACTOR_DIR + "/BP_TutorialGrowingZone"]
OLD_TAGS = ["Ability.Spell.TutorialMinibossShoot", "Ability.Spell.TutorialAllyShoot"]

REPORT = []


def log(*args):
    REPORT.append(" ".join(str(a) for a in args))


def flush():
    with open(REPORT_PATH, "w", encoding="utf-8") as handle:
        handle.write("\n".join(REPORT))


def dup_asset(source, destination):
    """Returns (asset, created)."""
    if unreal.EditorAssetLibrary.does_asset_exist(destination):
        return unreal.load_asset(destination), False
    unreal.EditorAssetLibrary.duplicate_asset(source, destination)
    return unreal.load_asset(destination), True


def curve_row(table_row):
    return ('Curve=(CurveTable="/Script/Engine.CurveTable\'%s.CT_Tutorial\'",RowName="%s")'
            % (CURVE_TABLE, table_row))


def table_value(row):
    table = unreal.load_asset(CURVE_TABLE)
    result, value = unreal.DataTableFunctionLibrary.evaluate_curve_table_row(table, row, COLUMN, "tutorial_miniboss")
    if result != unreal.EvaluateCurveTableResult.ROW_FOUND:
        raise RuntimeError("CT_Tutorial has no row %s: run tutorial_assets.py first" % row)
    return value


def scalable_row(row):
    value = unreal.ScalableFloat()
    handle = unreal.CurveTableRowHandle()
    handle.set_editor_property("CurveTable", unreal.load_asset(CURVE_TABLE))
    handle.set_editor_property("RowName", row)
    value.set_editor_property("Value", 1.0)
    value.set_editor_property("Curve", handle)
    return value


def damage_effect(row, per_second=False):
    """The star boss's salve damage with its amount reading a CT_Tutorial row; per second it ticks, cue limited."""
    template = unreal.get_default_object(
        unreal.load_asset(SALVE_ABILITY_SRC).generated_class()).get_editor_property("EffectDataInstances")[0]
    text = re.sub(r'Curve=\(CurveTable="[^"]*",RowName="[^"]*"\)', curve_row(row), template.export_text())
    text = re.sub(r"Amount=\(Value=[0-9.]+", "Amount=(Value=1.000000", text)
    if per_second:
        text = text.replace("bIsPerSecond=False", "bIsPerSecond=True").replace("bLimitGameplayCue=False",
                                                                                  "bLimitGameplayCue=True")
    effect = unreal.InstancedStruct()
    effect.import_text(text)
    return effect


def enemy_setup(team, health_row, salve_tag, actor_tag=""):
    """A summon or an ally: the salve shooter tree and its salve, dies at zero, health from a CT_Tutorial row."""
    setup = unreal.GeoEnemySetup()
    setup.set_editor_property("Team", team)
    setup.set_editor_property("bResetToFullLife", False)
    setup.set_editor_property("AggroRadius", 500.0)
    setup.set_editor_property("MaxHealth", scalable_row(health_row))
    setup.set_editor_property("StateTree", unreal.load_asset(SALVE_SHOOTER_TREE))
    setup.set_editor_property("AbilityTags", [waves.tag(salve_tag)])
    setup.set_editor_property("Tag", actor_tag)
    return setup


# ---------------------------------------------------------------- abilities

def build_salve():
    """The salve of the boss and its summons, a copy of the star boss's with its numbers in CT_Tutorial: at every player
    (Target AllPlayers), its bullets hit the other side only. Returns the ability Blueprint."""
    pattern_bp, _ = dup_asset(SALVE_PATTERN_SRC, ABILITY_DIR + "/BP_TutorialPattern_Salve")
    cdo = unreal.get_default_object(pattern_bp.generated_class())
    params = cdo.get_editor_property("ProjectileParams").copy()
    params.import_text("(OverrideSpeed=OverrideValue,ProjectileSpeed=%f)" % table_value("MinibossProjectileSpeed"))
    params.import_text("(OverrideOverlapAttitude=OverrideValue,OverlapAttitude=%d)" % HOSTILE_ATTITUDE)
    cdo.set_editor_property("ProjectileParams", params)
    waves.commit_bp(pattern_bp)

    ability_bp, created = dup_asset(SALVE_ABILITY_SRC, ABILITY_DIR + "/GA_TutorialSalve")
    cdo = unreal.get_default_object(ability_bp.generated_class())
    tags = unreal.GameplayTagContainer()
    tags.import_text('(GameplayTags=((TagName="%s")))' % SALVE_TAG)
    cdo.set_editor_property("AbilityTags", tags)
    cdo.set_editor_property("PatternToLaunch", pattern_bp.generated_class())
    cdo.set_editor_property("EffectDataInstances", [damage_effect("MinibossProjectileDamage")])
    set_target(cdo, "(Mode=AllPlayers)")
    waves.commit_bp(ability_bp)
    waves.register(ability_bp, SALVE_TAG, "Tutorial salve",
                   "Salve of the tutorial miniboss fight: the boss and its summons at the players.")
    log("salve", "created" if created else "updated")
    return ability_bp


def set_target(cdo, text):
    """Writes the members text names into the ability's Target, keeping the others."""
    target = cdo.get_editor_property("Target").copy()
    target.import_text(text)
    cdo.set_editor_property("Target", target)


def build_ally_salve(salve_bp):
    """The salve's child for the summons' allies: its tag (a child of the salve's), Target Boss, the AllyDamage colour
    on its own pattern child and its own damage row differ."""
    salve_cdo = unreal.get_default_object(salve_bp.generated_class())
    pattern_bp, _ = waves.make_bp("BP_TutorialPattern_AllySalve", ABILITY_DIR, salve_cdo.get_editor_property("PatternToLaunch"))
    pattern_cdo = unreal.get_default_object(pattern_bp.generated_class())
    params = pattern_cdo.get_editor_property("ProjectileParams").copy()
    for part in ("Head", "Trail"):
        params.import_text("(Override%sColor=OverrideValue,%sColor=(Color=AllyDamage))" % (part, part))
    pattern_cdo.set_editor_property("ProjectileParams", params)
    waves.commit_bp(pattern_bp)

    ability_bp, created = waves.make_bp("GA_TutorialAllySalve", ABILITY_DIR, salve_bp.generated_class())
    cdo = unreal.get_default_object(ability_bp.generated_class())
    tags = unreal.GameplayTagContainer()
    tags.import_text('(GameplayTags=((TagName="%s")))' % ALLY_SALVE_TAG)
    cdo.set_editor_property("AbilityTags", tags)
    cdo.set_editor_property("PatternToLaunch", pattern_bp.generated_class())
    cdo.set_editor_property("EffectDataInstances", [damage_effect("AllyProjectileDamage")])
    set_target(cdo, "(Mode=Boss)")
    waves.commit_bp(ability_bp)
    waves.register(ability_bp, ALLY_SALVE_TAG, "Tutorial ally salve",
                   "Salve of the tutorial miniboss fight: the summons' allies at the boss.")
    log("ally salve", "created" if created else "updated")


def native_child(name, parent_path):
    """The Blueprint under parent_path; one that exists is reparented, so whatever references it stays valid."""
    parent_class = unreal.load_class(None, parent_path)
    blueprint, created = waves.make_bp(name, ABILITY_DIR, parent_class)
    if not created:
        unreal.BlueprintEditorLibrary.reparent_blueprint(blueprint, parent_class)
    return blueprint, created


def build_summon_pattern():
    """The look of the Training Room's burst (SUMMON_PATTERN) and the Pillar clip: telegraph and hit only."""
    pattern_bp, _ = native_child("BP_TutorialPattern_Summon", "/Script/GeoTrinity.ZonePattern")
    cdo = unreal.get_default_object(pattern_bp.generated_class())
    look = unreal.get_default_object(unreal.load_asset(SUMMON_PATTERN).generated_class())
    for name in ("ZoneParams", "TeamAttitude", "StartCue"):
        cdo.set_editor_property(name, look.get_editor_property(name))
    cdo.set_editor_property("AnimMontage", unreal.load_asset(SUMMON_MONTAGE))
    waves.commit_bp(pattern_bp)
    return pattern_bp


def build_summon():
    """Telegraphs a player's place in the Damage colour and hits there (build_summon_pattern)."""
    pattern_bp = build_summon_pattern()
    ability_bp, created = native_child("GA_TutorialMinibossSummon", "/Script/GeoTrinity.PatternAbility")
    cdo = unreal.get_default_object(ability_bp.generated_class())
    cdo.set_editor_property("AbilityTags", tag_container([SUMMON_TAG]))
    cdo.set_editor_property("PatternToLaunch", pattern_bp.generated_class())
    set_target(cdo, "(Mode=RandomPlayers,Count=1)")
    cdo.set_editor_property("FireDelay", SUMMON_WIND_UP)
    cdo.set_editor_property("EffectDataInstances", [damage_effect("DummyLandingDamage")])
    waves.commit_bp(ability_bp)
    waves.register(ability_bp, SUMMON_TAG, "Tutorial miniboss summon", "Telegraphs a damage hit near a player.")
    log("summon", "created" if created else "updated")


def build_spawn_dummy():
    """Lands the summoned dummy (BP_Dummy + summon_setup) on the spot the summon telegraphed."""
    ability_bp, created = native_child("GA_TutorialSpawnDummy", "/Script/GeoTrinity.GeoSpawnEnemyCharacterAbility")
    cdo = unreal.get_default_object(ability_bp.generated_class())
    cdo.set_editor_property("AbilityTags", tag_container([SPAWN_DUMMY_TAG]))
    cdo.set_editor_property("EnemyClass", unreal.load_asset(BP_DUMMY).generated_class())
    cdo.set_editor_property("Setup", summon_setup())
    set_target(cdo, '(Mode=PatternTargets,PatternClass="%s.%s_C")' % (SUMMON_PATTERN_BP, SUMMON_PATTERN_BP.rsplit("/", 1)[1]))
    waves.commit_bp(ability_bp)
    waves.register(ability_bp, SPAWN_DUMMY_TAG, "Tutorial spawn dummy", "Lands the miniboss's summoned dummy on the telegraphed spot.")
    log("spawn dummy", "created" if created else "updated")


# ---------------------------------------------------------------- characters and trees

def reparented_copy(source, destination, parent_path):
    blueprint, created = dup_asset(source, destination)
    if created:
        unreal.BlueprintEditorLibrary.reparent_blueprint(blueprint, unreal.load_class(None, parent_path))
    return blueprint, created


def tag_container(names):
    tags = unreal.GameplayTagContainer()
    tags.import_text("(GameplayTags=(%s))" % ",".join('(TagName="%s")' % name for name in names))
    return tags


def build_summon_zone():
    """Passive of the dummy: a growing damage zone at its feet (Target Self), alive as long as the dummy."""
    ability_bp, created = native_child("GA_TutorialDummyZone", "/Script/GeoTrinity.GeoPlaceDeployableAbility")
    cdo = unreal.get_default_object(ability_bp.generated_class())
    cdo.set_editor_property("AbilityTags", tag_container([SUMMON_ZONE_TAG, PASSIVE_TAG]))
    zone_params = unreal.DeployableDataParams()
    zone_params.import_text("(Color=(Color=Damage),Attitude=%d)" % HOSTILE_ATTITUDE)
    zone_params.set_editor_property("GrowToSize", scalable_row("DummyZoneMaxRadius"))
    zone_params.set_editor_property("GrowSeconds", scalable_row("DummyZoneGrowthTime"))
    cdo.set_editor_property("DeployableActorClass", unreal.load_class(None, DEFAULT_ZONE_CLASS))
    cdo.set_editor_property("Params", zone_params)
    cdo.set_editor_property("Size", scalable_row("DummyZoneStartRadius"))
    cdo.set_editor_property("EffectDataInstances", [damage_effect("DummyZoneDamage", per_second=True)])
    set_target(cdo, "(Mode=Self)")
    cdo.set_editor_property("bUseGeneralChargeTimeForFireDelay", False)
    cdo.set_editor_property("FireDelay", 0.0)  # the zone exists the moment the dummy does
    waves.commit_bp(ability_bp)
    waves.register(ability_bp, SUMMON_ZONE_TAG, "Tutorial summon zone", "Growing damage zone under the summoned dummy.")
    log("summon zone", "created" if created else "updated")


def build_summon_ally():
    """The dummy's ability: leaves the salve-shooting ally (BP_DummyAlly) where it dies. No Passive tag: it would spawn the ally at once."""
    ability_bp, created = native_child("GA_TutorialSpawnAlly", "/Script/GeoTrinity.GeoSpawnEnemyCharacterAbility")
    cdo = unreal.get_default_object(ability_bp.generated_class())
    cdo.set_editor_property("AbilityTags", tag_container([SUMMON_ALLY_TAG]))
    cdo.set_editor_property("EnemyClass", unreal.load_asset(BP_DUMMY_ALLY).generated_class())
    cdo.set_editor_property("Setup", enemy_setup(unreal.Team.PLAYER, "AllyDummyHealth", ALLY_SALVE_TAG, ALLY_ACTOR_TAG))
    cdo.set_editor_property("bSpawnOnOwnerDying", True)
    set_target(cdo, "(Mode=Self)")
    waves.commit_bp(ability_bp)
    waves.register(ability_bp, SUMMON_ALLY_TAG, "Tutorial summon ally", "Leaves an ally where the summoned dummy dies.")
    log("summon ally", "created" if created else "updated")


def summon_setup():
    """The summoned dummy: a plain BP_Dummy that never shoots (no tree), its zone and its ally as passives."""
    setup = enemy_setup(unreal.Team.ENEMY, "DummyHealth", SUMMON_ZONE_TAG, SUMMON_ACTOR_TAG)
    setup.set_editor_property("StateTree", None)
    setup.set_editor_property("AbilityTags", [waves.tag(SUMMON_ZONE_TAG), waves.tag(SUMMON_ALLY_TAG)])
    return setup


def build_salve_shooter_tree():
    """ST_Dummy_Shooter's one Shoot state, firing the salve: what the summons and their allies run."""
    tree, created = dup_asset(SHOOTER_TREE, SALVE_SHOOTER_TREE)
    unreal.GeoStateTreeBuilderUtil.get_default_object().replace_fire_ability_tag_in_state(tree, "Shoot", SALVE_TAG)
    unreal.EditorAssetLibrary.save_loaded_asset(tree, only_if_is_dirty=False)
    log("salve shooter tree", "created" if created else "updated")


def build_tree():
    """Start (the salve begins, keeps firing) -> Move -> Summon -> Land -> CountSummons -> [Wave] -> Move ..."""
    tree, created = dup_asset(TREE_SRC, TREE_DIR + "/ST_TutorialMiniboss")
    builder = unreal.GeoStateTreeBuilderUtil.get_default_object()
    if created:
        for stale in ("FireAll", "DevastatingWave", "SpawnPillar", "Spiral"):
            builder.clear_transitions(tree, stale)
        builder.remove_state(tree, "FireAll")
        builder.remove_state(tree, "Behaviour")

        completed = unreal.StateTreeTransitionTrigger.ON_STATE_COMPLETED
        for name in ("Start", "Move", "Summon", "Land", "CountSummons", "Wave"):
            builder.add_state(tree, name, "Root", -1)

        builder.add_fire_ability_task_to_state(tree, "Start", SALVE_TAG)
        builder.add_task_to_state(tree, "Start", "StateTreeDelayTask")
        builder.set_task_property(tree, "Start", "StateTreeDelayTask", "Duration", str(SHOOT_START_SECONDS))
        builder.set_tasks_completion(tree, "Start", unreal.StateTreeTaskCompletionType.ANY)
        builder.add_task_to_state(tree, "Move", "STTask_MoveToTargetPoint")
        builder.add_fire_ability_task_to_state(tree, "Summon", SUMMON_TAG)
        builder.add_fire_ability_task_to_state(tree, "Land", SPAWN_DUMMY_TAG)
        builder.add_send_event_after_n_cycles_task(tree, "CountSummons", SUMMONS_PER_WAVE, WAVE_EVENT_TAG)
        builder.add_fire_ability_task_to_state(tree, "Wave", WAVE_TAG)
        builder.set_required_event_to_enter(tree, "Wave", WAVE_EVENT_TAG)

        # A transition into Wave fails without the event and falls through to the next one.
        builder.add_transition(tree, "Start", "Move", completed)
        builder.add_transition(tree, "Move", "Summon", completed)
        builder.add_transition(tree, "Summon", "Land", completed)
        builder.add_transition(tree, "Land", "CountSummons", completed)
        builder.add_transition(tree, "CountSummons", "Wave", completed)
        builder.add_transition(tree, "CountSummons", "Move", completed)
        builder.add_transition(tree, "Wave", "Move", completed)
    else:
        if not builder.has_state(tree, "Land"):
            builder.add_state(tree, "Land", "Root", -1)
            builder.add_fire_ability_task_to_state(tree, "Land", SPAWN_DUMMY_TAG)
            builder.clear_transitions(tree, "Summon")
            builder.add_transition(tree, "Summon", "Land", unreal.StateTreeTransitionTrigger.ON_STATE_COMPLETED)
            builder.add_transition(tree, "Land", "CountSummons", unreal.StateTreeTransitionTrigger.ON_STATE_COMPLETED)
        builder.replace_fire_ability_tag_in_state(tree, "Start", SALVE_TAG)
        builder.replace_fire_ability_tag_in_state(tree, "Wave", WAVE_TAG)
    # The wave's direction is rolled per firing (one of the 4 basic ones); its lanes come from the living adds.
    builder.set_task_property(tree, "Wave", "STTask_FireAbility", "PatternData", WAVE_PATTERN_DATA)
    for name, value in (("PointTag", POINT_TAG), ("ArenaTag", ARENA_TAG)):
        builder.set_task_property(tree, "Move", "STTask_MoveToTargetPoint", name, '(TagName="%s")' % value)
    unreal.EditorAssetLibrary.save_loaded_asset(tree, only_if_is_dirty=False)
    log("behaviour tree", "created" if created else "kept")


def delete_old_assets():
    """Drops the old layout's catalog entries, then each of its assets nothing outside the old layout references."""
    catalog = unreal.load_asset(CATALOG)
    infos = catalog.get_editor_property("EnemyAbilityInfos")
    kept = [info for info in infos
            if not any(path + "." in info.export_text() for path in OLD_ASSETS)
            and not any('"%s"' % old_tag in info.export_text() for old_tag in OLD_TAGS)]
    if len(kept) != len(infos):
        catalog.set_editor_property("EnemyAbilityInfos", kept)
        unreal.EditorAssetLibrary.save_loaded_asset(catalog, only_if_is_dirty=False)
        log("catalog entries removed", len(infos) - len(kept))
    waves.delete_unreferenced(OLD_ASSETS, log)


def run():
    REPORT[:] = []
    try:
        build_ally_salve(build_salve())
        build_salve_shooter_tree()
        build_summon_zone()
        build_summon_ally()
        build_summon()
        build_spawn_dummy()
        build_tree()
        delete_old_assets()
        log("OK")
    except Exception:
        log("FAILED", traceback.format_exc())
    flush()


if __name__ == "__main__":
    run()
