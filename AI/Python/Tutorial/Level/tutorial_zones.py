"""
The three zone abilities of the Zones room (AI/Tutorial/Steps.md step 7).

Per colour: a pattern Blueprint (child of UZonePattern, bZoneUntilStopped: telegraph, then a lingering zone held until
the pattern is force-stopped) and a PatternAbility Blueprint (the zone's effects, Target = the colour's point), registered
in the enemy ability catalog. The encounter fires each with a held GeoActivateAbilityAction (tutorial_encounters.py,
Builder.zone); the wind-up is the ability's default fire delay, no row. Every amount reads a CT_Tutorial row.
Idempotent: assets keyed by path, properties rewritten. Report: AI/Output/tutorial_zones_report.txt.
Usage: through AI/Python/Runtime/run_via_bridge.py (editor open) or headless; the three ability tags are in
Config/Tags/GeoGameplayTags.ini (editor restarted since).
"""
import traceback

import unreal

REPORT_PATH = "C:/GeoTrinity/AI/Output/tutorial_zones_report.txt"
ABILITY_DIR = "/Game/AbilitySystem/Abilities/Enemy/TutorialZones"
CATALOG = "/Game/AbilitySystem/Data/DA_AbilityInfo"
CURVE_TABLE = "/Game/AbilitySystem/Data/CurveTables/CT_Tutorial"
ZONE_RADIUS = 300.0  # more than half the Shield-Heal distance: the two zones overlap
DAMAGE_ZONE_START_SIZE = 10.0
ZONE_ATTITUDE = 7  # ETeamAttitudeBitflag: Neutral | Friendly | Hostile, a zone acts on everyone
COLORS = ["Heal", "Shield", "Damage"]
ZONE_COLORS = {"Heal": "Heal", "Shield": "DamageReduction", "Damage": "Damage"}  # EGeoColor of each zone
DESCRIPTIONS = {"Heal": "A healing zone held for the whole step.",
                "Shield": "A damage-reduction zone held for the whole step.",
                "Damage": "A damage zone that grows until it covers the room, held for the whole step."}


def row_value(row):
    """A scalable float read from a CT_Tutorial row, in import_text form."""
    return "(Value=1.000000,Curve=(CurveTable=\"/Script/Engine.CurveTable'%s.CT_Tutorial'\",RowName=\"%s\"))" % (
        CURVE_TABLE, row)


HEAL_EFFECT = ('/Script/GeoTrinity.HealEffectData(bSuppressHealProvided=False,Amount=%s,bIsPerSecond=True,'
               'bLimitGameplayCue=True)' % row_value("ColorZoneHeal"))
SHIELD_EFFECT = ('/Script/GeoTrinity.GameplayEffectData(GameplayEffect="/Script/Engine.BlueprintGeneratedClass\'/Game/'
                 'AbilitySystem/GameplayEffects/Buffs/GE_DamageReductionBuff.GE_DamageReductionBuff_C\'",'
                 'DataTag=(TagName="Status.Buff.DamageReduction"),Magnitude=%s,bInfiniteDuration=True,'
                 'Icon="/Script/GeoTrinity.GeoIcon\'/Game/HUD/Icons/DA_Icon_Status_DamageReduction.'
                 'DA_Icon_Status_DamageReduction\'")' % row_value("ColorZoneShieldShare"))
DAMAGE_EFFECT = "/Script/GeoTrinity.DamageEffectData(Amount=%s,bIsPerSecond=True,bLimitGameplayCue=True)" % row_value(
    "ColorZoneDamage")
ZONE_EFFECTS = {"Heal": HEAL_EFFECT, "Shield": SHIELD_EFFECT, "Damage": DAMAGE_EFFECT}

REPORT = []


def log(*args):
    REPORT.append(" ".join(str(a) for a in args))


def flush():
    with open(REPORT_PATH, "w", encoding="utf-8") as handle:
        handle.write("\n".join(REPORT))


def scalable_row(row):
    value = unreal.ScalableFloat()
    handle = unreal.CurveTableRowHandle()
    handle.set_editor_property("CurveTable", unreal.load_asset(CURVE_TABLE))
    handle.set_editor_property("RowName", row)
    value.set_editor_property("Value", 1.0)
    value.set_editor_property("Curve", handle)
    return value


def make_bp(name, parent_class):
    path = "%s/%s" % (ABILITY_DIR, name)
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.load_asset(path), False
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("ParentClass", parent_class)
    blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, ABILITY_DIR, unreal.Blueprint, factory)
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    return blueprint, True


def commit_bp(blueprint):
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    unreal.EditorAssetLibrary.save_loaded_asset(blueprint)


def register(ability_bp, ability_tag_name, display_name, description):
    """Puts the ability in the enemy ability catalog, replacing an entry of the same tag."""
    catalog = unreal.load_asset(CATALOG)
    infos = [info for info in catalog.get_editor_property("EnemyAbilityInfos")
             if '"%s"' % ability_tag_name not in info.export_text()]
    class_path = "/Script/Engine.BlueprintGeneratedClass'%s'" % ability_bp.generated_class().get_path_name()
    info = unreal.GameplayAbilityInfo()
    info.import_text('(AbilityClass="%s",AbilityTag=(TagName="%s"),AbilityDisplayName="%s",Description="%s")'
                     % (class_path, ability_tag_name, display_name, description))
    infos.append(info)
    catalog.set_editor_property("EnemyAbilityInfos", infos)
    unreal.EditorAssetLibrary.save_loaded_asset(catalog, only_if_is_dirty=False)


def build_zone(color):
    pattern_bp, _ = make_bp("BP_TutorialZone" + color, unreal.load_class(None, "/Script/GeoTrinity.ZonePattern"))
    cdo = unreal.get_default_object(pattern_bp.generated_class())
    params = cdo.get_editor_property("ZoneParams").copy()
    size = DAMAGE_ZONE_START_SIZE if color == "Damage" else ZONE_RADIUS
    params.import_text("(Size=%f,Color=(Color=%s),Attitude=%d)" % (size, ZONE_COLORS[color], ZONE_ATTITUDE))
    if color == "Damage":
        params.set_editor_property("GrowToSize", scalable_row("ColorZoneMaxRadius"))
        params.set_editor_property("GrowSeconds", scalable_row("ColorZoneGrowSeconds"))
    cdo.set_editor_property("ZoneParams", params)
    cdo.set_editor_property("bZoneUntilStopped", True)
    commit_bp(pattern_bp)

    tag_name = "Ability.Spell.TutorialZone" + color
    ability_bp, created = make_bp("GA_Tutorial_Zone" + color,
                                  unreal.load_class(None, "/Script/GeoTrinity.PatternAbility"))
    cdo = unreal.get_default_object(ability_bp.generated_class())
    tags = unreal.GameplayTagContainer()
    tags.import_text('(GameplayTags=((TagName="%s")))' % tag_name)
    cdo.set_editor_property("AbilityTags", tags)
    cdo.set_editor_property("PatternToLaunch", pattern_bp.generated_class())
    target = cdo.get_editor_property("Target").copy()
    target.import_text('(Mode=TargetPoint,TargetPointTag=(TagName="TargetPoint.Tutorial.Zone%s"))' % color)
    cdo.set_editor_property("Target", target)
    effect = unreal.InstancedStruct()
    effect.import_text(ZONE_EFFECTS[color])
    cdo.set_editor_property("EffectDataInstances", [effect])
    commit_bp(ability_bp)
    register(ability_bp, tag_name, "Tutorial %s zone" % color, DESCRIPTIONS[color])
    log(color, "created" if created else "updated", ability_bp.generated_class().get_path_name())


def run():
    REPORT[:] = []
    try:
        for color in COLORS:
            build_zone(color)
        log("OK")
    except Exception:
        log("FAILED", traceback.format_exc())
    flush()


if __name__ == "__main__":
    run()
