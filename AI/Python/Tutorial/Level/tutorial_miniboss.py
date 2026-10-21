"""
Tutorial miniboss assets (AI/Tutorial/Steps.md step 10): BP_TutorialMiniboss and its behaviour tree, the summoned dummy
and the ally it turns into, the abilities between them (miniboss salve, summon, ally salve), the growing zone and the
health effects. The lateral wave it fires is built by tutorial_waves.py (run it first); the room, the arena and the
encounter by tutorial_encounters.py.
Rows read from CT_Tutorial (tutorial_assets.py): MinibossHealth, DummyHealth, AllyDummyHealth, DummyLandingDamage,
DummyZoneDamage, DummyZoneGrowthTime, DummyZoneMaxRadius, MinibossProjectileDamage, MinibossProjectileSpeed,
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
CHARACTER_DIR = "/Game/Characters/Enemies/Tutorial"
ACTOR_DIR = "/Game/Actors/Tutorial"
TREE_DIR = "/Game/AI/Tutorial"
CURVE_TABLE = "/Game/AbilitySystem/Data/CurveTables/CT_Tutorial"
BP_DUMMY = "/Game/Characters/Enemies/Training/BP_Dummy"
ATTRIBUTES_SRC = ATTRIBUTE_DIR + "/GE_Dummy_Attributes"
ZONE_SRC = "/Game/Actors/BP_DamageZone"
SALVE_ABILITY_SRC = "/Game/AbilitySystem/Abilities/Enemy/ProjectileToAll/GA_FireProjectileToAllPlayers"
SALVE_PATTERN_SRC = "/Game/AbilitySystem/Abilities/Enemy/ProjectileToAll/BP_ProjectileToAll_Pattern"
SUMMON_PATTERN_SRC = "/Game/AbilitySystem/Abilities/Enemy/Training/BP_ZonePattern_BurstDamage"
TREE_SRC = "/Game/AI/ST_StarBossBehaviour"
SHOOTER_TREE = "/Game/AI/Dummy/ST_Dummy_Shooter"

MINIBOSS_SHOOT_TAG = "Ability.Spell.TutorialMinibossShoot"
SUMMON_TAG = "Ability.Spell.TutorialMinibossSummon"
WAVE_TAG = "Ability.Spell.TutorialMinibossWave"
ALLY_SHOOT_TAG = "Ability.Spell.TutorialAllyShoot"
WAVE_EVENT_TAG = "Event.StateTree.TriggerMinibossWave"
POINT_TAG = "TargetPoint.Tutorial.MinibossPoint"
ALLY_ACTOR_TAG = "TutorialMinibossAlly"

SUMMONS_PER_WAVE = 3  # CyclesRequired: Board page 13, tip 3 ("every third Dummy")
SHOOT_START_SECONDS = 0.2  # the salve keeps firing on its own; the state only starts it
SUMMON_WIND_UP = 1.5  # seconds the landing spot is telegraphed
SUMMON_SPOT_RADIUS = 400.0
ZONE_START_RADIUS = 150.0
HOSTILE_ATTITUDE = 4  # ETeamAttitudeBitflag: acts on the other team
COLUMN = 1.0  # CT_Tutorial is flat: every difficulty column holds the same value

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


# ---------------------------------------------------------------- health effects

def build_health_effect(name, row):
    """A copy of the dummy's attributes whose Health and MaxHealth read a CT_Tutorial row."""
    effect, created = dup_asset(ATTRIBUTES_SRC, "%s/%s" % (ATTRIBUTE_DIR, name))
    cdo = unreal.get_default_object(effect.generated_class())
    modifiers = []
    for modifier in cdo.get_editor_property("Modifiers"):
        text = modifier.export_text()
        if "ScalableFloatMagnitude=(Value=500.000000,Curve=(CurveTable=None,RowName=\"\")" in text:
            text = text.replace("ScalableFloatMagnitude=(Value=500.000000,Curve=(CurveTable=None,RowName=\"\")",
                                "ScalableFloatMagnitude=(Value=1.000000," + curve_row(row))
        rebuilt = unreal.GameplayModifierInfo()
        rebuilt.import_text(text)
        modifiers.append(rebuilt)
    cdo.set_editor_property("Modifiers", modifiers)
    waves.commit_bp(effect)
    log("health effect", name, row, "created" if created else "updated")
    return effect


# ---------------------------------------------------------------- abilities

def build_salve(key, ability_tag, damage_row, speed, aim_at_boss):
    """A salve at every player (or, aimed at the arena's boss, an ally's), a copy of the star boss's with its numbers
    in CT_Tutorial."""
    pattern_bp, _ = dup_asset(SALVE_PATTERN_SRC, "%s/BP_TutorialPattern_%s" % (ABILITY_DIR, key))
    cdo = unreal.get_default_object(pattern_bp.generated_class())
    params = cdo.get_editor_property("ProjectileParams").copy()
    params.import_text("(OverrideSpeed=OverrideValue,ProjectileSpeed=%f)" % speed)
    if aim_at_boss:
        params.import_text("(OverrideOverlapAttitude=OverrideValue,OverlapAttitude=%d)" % HOSTILE_ATTITUDE)
    cdo.set_editor_property("ProjectileParams", params)
    waves.commit_bp(pattern_bp)

    ability_bp, created = dup_asset(SALVE_ABILITY_SRC, "%s/GA_Tutorial%s" % (ABILITY_DIR, key))
    cdo = unreal.get_default_object(ability_bp.generated_class())
    tags = unreal.GameplayTagContainer()
    tags.import_text('(GameplayTags=((TagName="%s")))' % ability_tag)
    cdo.set_editor_property("AbilityTags", tags)
    cdo.set_editor_property("PatternToLaunch", pattern_bp.generated_class())
    cdo.set_editor_property("EffectDataInstances", [damage_effect(damage_row)])
    cdo.set_editor_property("bAimAtArenaBoss", aim_at_boss)
    waves.commit_bp(ability_bp)
    waves.register(ability_bp, ability_tag, "Tutorial " + key, "Salve of the tutorial miniboss fight.")
    log("salve", key, "created" if created else "updated")


def build_summon(add_class):
    """Telegraphs the landing spot in the Damage colour, hits there, then the dummy lands."""
    pattern_bp, _ = dup_asset(SUMMON_PATTERN_SRC, ABILITY_DIR + "/BP_ZonePattern_MinibossSummon")
    waves.commit_bp(pattern_bp)

    ability_bp, created = waves.make_bp("GA_TutorialMinibossSummon", ABILITY_DIR,
                                        unreal.load_class(None, "/Script/GeoTrinity.GeoSummonAddAbility"))
    cdo = unreal.get_default_object(ability_bp.generated_class())
    tags = unreal.GameplayTagContainer()
    tags.import_text('(GameplayTags=((TagName="%s")))' % SUMMON_TAG)
    cdo.set_editor_property("AbilityTags", tags)
    cdo.set_editor_property("PatternToLaunch", pattern_bp.generated_class())
    cdo.set_editor_property("AddClass", add_class)
    cdo.set_editor_property("SpotRadius", SUMMON_SPOT_RADIUS)
    cdo.set_editor_property("FireDelay", SUMMON_WIND_UP)
    cdo.set_editor_property("EffectDataInstances", [damage_effect("DummyLandingDamage")])
    waves.commit_bp(ability_bp)
    waves.register(ability_bp, SUMMON_TAG, "Tutorial miniboss summon", "Telegraphs a damage hit near a player, then a dummy lands there.")
    log("summon", "created" if created else "updated")


# ---------------------------------------------------------------- characters

def enemy_setup(health_row, state_tree, ability_tag, actor_tag):
    handle = unreal.CurveTableRowHandle()
    handle.set_editor_property("CurveTable", unreal.load_asset(CURVE_TABLE))
    handle.set_editor_property("RowName", health_row)
    max_health = unreal.ScalableFloat()
    max_health.set_editor_property("Value", 1.0)
    max_health.set_editor_property("Curve", handle)
    setup = unreal.GeoEnemySetup()
    setup.set_editor_property("Team", unreal.Team.PLAYER)
    setup.set_editor_property("MaxHealth", max_health)
    setup.set_editor_property("StateTree", unreal.load_asset(state_tree))
    setup.set_editor_property("AbilityTags", [waves.tag(ability_tag)])
    setup.set_editor_property("Tag", actor_tag)
    return setup


def scalable_row(row):
    value = unreal.ScalableFloat()
    handle = unreal.CurveTableRowHandle()
    handle.set_editor_property("CurveTable", unreal.load_asset(CURVE_TABLE))
    handle.set_editor_property("RowName", row)
    value.set_editor_property("Value", 1.0)
    value.set_editor_property("Curve", handle)
    return value


def reparented_copy(source, destination, parent_path):
    blueprint, created = dup_asset(source, destination)
    if created:
        unreal.BlueprintEditorLibrary.reparent_blueprint(blueprint, unreal.load_class(None, parent_path))
    return blueprint, created


def build_growing_zone():
    """The damage zone of the tutorial rooms, grown over DummyZoneGrowthTime to DummyZoneMaxRadius."""
    blueprint, created = reparented_copy(ZONE_SRC, ACTOR_DIR + "/BP_TutorialGrowingZone", "/Script/GeoTrinity.GeoGrowingZone")
    cdo = unreal.get_default_object(blueprint.generated_class())
    cdo.set_editor_property("AttitudeBitmask", HOSTILE_ATTITUDE)
    cdo.set_editor_property("EffectDataArray", [])
    cdo.set_editor_property("GrowthSeconds", scalable_row("DummyZoneGrowthTime"))
    cdo.set_editor_property("MaxRadius", scalable_row("DummyZoneMaxRadius"))
    waves.commit_bp(blueprint)
    log("growing zone", "created" if created else "updated")
    return blueprint


def build_summoned_dummy(zone_bp, health_effect):
    blueprint, created = reparented_copy(BP_DUMMY, CHARACTER_DIR + "/BP_TutorialSummonedDummy",
                                         "/Script/GeoTrinity.GeoSummonedDummy")
    cdo = unreal.get_default_object(blueprint.generated_class())
    zone_params = unreal.DeployableDataParams()
    zone_params.import_text("(Size=%f,Color=(Color=Damage),Attitude=%d)" % (ZONE_START_RADIUS, HOSTILE_ATTITUDE))
    cdo.set_editor_property("ZoneClass", zone_bp.generated_class())
    cdo.set_editor_property("ZoneEffects", [damage_effect("DummyZoneDamage", per_second=True)])
    cdo.set_editor_property("ZoneParams", zone_params)
    cdo.set_editor_property("AllyClass", unreal.load_asset(BP_DUMMY).generated_class())
    cdo.set_editor_property("AllySetup", enemy_setup("AllyDummyHealth", SHOOTER_TREE, ALLY_SHOOT_TAG, ALLY_ACTOR_TAG))
    cdo.set_editor_property("StateTree", unreal.load_asset(SHOOTER_TREE))
    cdo.set_editor_property("ResetToFullLifeWhenReachingZero", False)
    asc = cdo.get_editor_property("AbilitySystemComponent")
    asc.set_editor_property("DefaultAttributes", health_effect.generated_class())
    asc.set_editor_property("StartupAbilityTags", [waves.tag(MINIBOSS_SHOOT_TAG)])
    waves.commit_bp(blueprint)
    log("summoned dummy", "created" if created else "updated")
    return blueprint


def build_tree():
    """Start (the salve begins, keeps firing) -> Move -> Summon -> CountSummons -> [Wave] -> Move ..."""
    tree, created = dup_asset(TREE_SRC, TREE_DIR + "/ST_TutorialMiniboss")
    builder = unreal.GeoStateTreeBuilderUtil.get_default_object()
    if created:
        for stale in ("FireAll", "DevastatingWave", "SpawnPillar", "Spiral"):
            builder.clear_transitions(tree, stale)
        builder.remove_state(tree, "FireAll")
        builder.remove_state(tree, "Behaviour")

        completed = unreal.StateTreeTransitionTrigger.ON_STATE_COMPLETED
        for name in ("Start", "Move", "Summon", "CountSummons", "Wave"):
            builder.add_state(tree, name, "Root", -1)

        builder.add_fire_ability_task_to_state(tree, "Start", MINIBOSS_SHOOT_TAG)
        builder.add_task_to_state(tree, "Start", "StateTreeDelayTask")
        builder.set_task_property(tree, "Start", "StateTreeDelayTask", "Duration", str(SHOOT_START_SECONDS))
        builder.set_tasks_completion(tree, "Start", unreal.StateTreeTaskCompletionType.ANY)
        builder.add_task_to_state(tree, "Move", "STTask_MoveToTargetPoint")
        builder.set_task_property(tree, "Move", "STTask_MoveToTargetPoint", "PointTag", '(TagName="%s")' % POINT_TAG)
        builder.add_fire_ability_task_to_state(tree, "Summon", SUMMON_TAG)
        builder.add_send_event_after_n_cycles_task(tree, "CountSummons", SUMMONS_PER_WAVE, WAVE_EVENT_TAG)
        builder.add_fire_ability_task_to_state(tree, "Wave", WAVE_TAG)
        builder.set_required_event_to_enter(tree, "Wave", WAVE_EVENT_TAG)

        # A transition into Wave fails without the event and falls through to the next one.
        builder.add_transition(tree, "Start", "Move", completed)
        builder.add_transition(tree, "Move", "Summon", completed)
        builder.add_transition(tree, "Summon", "CountSummons", completed)
        builder.add_transition(tree, "CountSummons", "Wave", completed)
        builder.add_transition(tree, "CountSummons", "Move", completed)
        builder.add_transition(tree, "Wave", "Move", completed)
        unreal.EditorAssetLibrary.save_loaded_asset(tree)
    log("behaviour tree", "created" if created else "kept")
    return tree


def build_miniboss(tree, health_effect):
    blueprint, created = dup_asset(BP_DUMMY, CHARACTER_DIR + "/BP_TutorialMiniboss")
    cdo = unreal.get_default_object(blueprint.generated_class())
    cdo.set_editor_property("BehaviourStateTree", tree)
    cdo.set_editor_property("ResetToFullLifeWhenReachingZero", False)
    asc = cdo.get_editor_property("AbilitySystemComponent")
    asc.set_editor_property("DefaultAttributes", health_effect.generated_class())
    asc.set_editor_property("StartupAbilityTags", [waves.tag(t) for t in (MINIBOSS_SHOOT_TAG, SUMMON_TAG, WAVE_TAG)])
    waves.commit_bp(blueprint)
    log("miniboss", "created" if created else "updated")


def run():
    REPORT[:] = []
    try:
        miniboss_health = build_health_effect("GE_TutorialMinibossAttributes", "MinibossHealth")
        dummy_health = build_health_effect("GE_TutorialSummonedDummyAttributes", "DummyHealth")
        build_salve("MinibossShoot", MINIBOSS_SHOOT_TAG, "MinibossProjectileDamage",
                    table_value("MinibossProjectileSpeed"), aim_at_boss=False)
        build_salve("AllyShoot", ALLY_SHOOT_TAG, "AllyProjectileDamage", table_value("MinibossProjectileSpeed"),
                    aim_at_boss=True)
        dummy = build_summoned_dummy(build_growing_zone(), dummy_health)
        build_summon(dummy.generated_class())
        build_miniboss(build_tree(), miniboss_health)
        log("OK")
    except Exception:
        log("FAILED", traceback.format_exc())
    flush()


if __name__ == "__main__":
    run()
