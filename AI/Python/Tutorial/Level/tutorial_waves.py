"""
Lateral-wave abilities of the tutorial (AI/Tutorial/Steps.md steps 8-10) and the hidden caster that fires the zone room's.

Per wave: a pattern Blueprint (child of ULateralWavePattern, the geometry) and a PatternAbility Blueprint (the lethal
effect, the wind-up, the wave start point), registered in the enemy ability catalog. Then BP_HiddenCaster_TutorialWaves
holding the room's waves. The miniboss wave is built here too (lanes from the arena's adds) and held by the miniboss
(tutorial_miniboss.py).
Idempotent: assets keyed by path, properties rewritten. Report: AI/Output/tutorial_waves_report.txt.
Usage: headless (AI/Commands.md), editor closed; Config/Tags/GeoGameplayTags.ini already holds the tags.
"""
import traceback

import unreal

REPORT_PATH = "C:/GeoTrinity/AI/Output/tutorial_waves_report.txt"
ABILITY_DIR = "/Game/AbilitySystem/Abilities/Enemy/TutorialWaves"
CASTER_DIR = "/Game/Characters/Enemies/Tutorial"
CATALOG = "/Game/AbilitySystem/Data/DA_AbilityInfo"
BEAM_SYSTEM = "/Game/Art/VFX/Generic/Niagara/NS_Beam"
MASK_MPC = "/Game/Art/VFX/Abilities/Enemies/PillarsAOE/MPC_MaskedArea"
WAVE_START_TAG = "TargetPoint.Tutorial.WaveStart"
LETHAL_EFFECT = ("/Script/GeoTrinity.LethalEffectData(OneShotCue=(CueTag=(TagName=\"\"),Color=Neutral,"
                 "SecondaryColors=,SoundTag=(TagName=\"\")),Difficulties=7)")

# The zone room is 1400 long (X) and 2200 wide (Y): the front starts on the wave point near the entry end and crosses
# the room along +X, the lanes are Y offsets from that point.
TRAVEL = 1300.0
HALF_WIDTH = 1200.0
LANE_WIDTH = 380.0

# key: tag suffix, lanes (None = from the arena's adds), band width, speed, wind-up seconds, description
WAVES = {
    "Lethal": ([], TRAVEL, 2500.0, 3.0, "Lethal wave over the whole floor of the zone-colours room, no safe lane."),
    "Safe1": ([-650.0, 0.0, 650.0], 300.0, 1400.0, 3.5, "First safe-spot wave: three lanes."),
    "Safe2": ([-300.0, 500.0], 300.0, 1400.0, 3.0, "Second safe-spot wave: two lanes."),
    "Safe3": ([250.0], 300.0, 1400.0, 2.5, "Third safe-spot wave: one lane."),
    "MinibossWave": (None, 300.0, 1200.0, 2.5, "Lethal wave with one safe lane through each living dummy."),
}
ROOM_WAVES = ["Lethal", "Safe1", "Safe2", "Safe3"]
TAG_PREFIX = "Ability.Spell.Tutorial"

REPORT = []


def log(*args):
    REPORT.append(" ".join(str(a) for a in args))


def flush():
    with open(REPORT_PATH, "w", encoding="utf-8") as handle:
        handle.write("\n".join(REPORT))


def tag(name):
    value = unreal.GameplayTag()
    value.import_text('(TagName="%s")' % name)
    return value


def ability_tag(key):
    return TAG_PREFIX + ("Wave" + key if not key.startswith("Miniboss") else key)


def make_bp(name, folder, parent_class):
    path = "%s/%s" % (folder, name)
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.load_asset(path), False
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("ParentClass", parent_class)
    blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, unreal.Blueprint, factory)
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    return blueprint, True


def commit_bp(blueprint):
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    unreal.EditorAssetLibrary.save_loaded_asset(blueprint)


def build_wave(key):
    lanes, band, speed, wind_up, description = WAVES[key]
    pattern_bp, _ = make_bp("BP_LateralWave_" + key, ABILITY_DIR, unreal.load_class(None, "/Script/GeoTrinity.LateralWavePattern"))
    cdo = unreal.get_default_object(pattern_bp.generated_class())
    cdo.set_editor_property("Speed", speed)
    cdo.set_editor_property("TravelDistance", TRAVEL)
    cdo.set_editor_property("BandWidth", band)
    cdo.set_editor_property("HalfWidth", HALF_WIDTH)
    cdo.set_editor_property("SpotRadius", LANE_WIDTH * 0.5)
    cdo.set_editor_property("FixedSpotOffsets", [unreal.Vector2D(0.0, lane) for lane in lanes or []])
    source = unreal.SafeSpotSource.ADDS if lanes is None else unreal.SafeSpotSource.FIXED
    cdo.set_editor_property("SafeSpotSources", [source])
    cdo.set_editor_property("MaskMaterialParameterCollection", unreal.load_asset(MASK_MPC))
    cdo.set_editor_property("BeamVfxSystem", unreal.load_asset(BEAM_SYSTEM))
    color = unreal.GeoColorParam()
    color.import_text("(Color=LethalDamage)")
    cdo.set_editor_property("BeamColor", color)
    commit_bp(pattern_bp)

    ability_bp, created = make_bp("GA_TutorialWave_" + key, ABILITY_DIR,
                                  unreal.load_class(None, "/Script/GeoTrinity.PatternAbility"))
    cdo = unreal.get_default_object(ability_bp.generated_class())
    tags = unreal.GameplayTagContainer()
    tags.import_text('(GameplayTags=((TagName="%s")))' % ability_tag(key))
    cdo.set_editor_property("AbilityTags", tags)
    cdo.set_editor_property("PatternToLaunch", pattern_bp.generated_class())
    cdo.set_editor_property("TargetPointTag", tag(WAVE_START_TAG))
    cdo.set_editor_property("FireDelay", wind_up)
    effect = unreal.InstancedStruct()
    effect.import_text(LETHAL_EFFECT)
    cdo.set_editor_property("EffectDataInstances", [effect])
    commit_bp(ability_bp)
    register(ability_bp, ability_tag(key), "Tutorial wave: " + key, description)
    log("wave", key, "created" if created else "updated")
    return ability_bp


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


def build_caster():
    parent = unreal.load_class(None, "/Script/GeoTrinity.HiddenCasterCharacter")
    blueprint, created = make_bp("BP_HiddenCaster_TutorialWaves", CASTER_DIR, parent)
    cdo = unreal.get_default_object(blueprint.generated_class())
    asc = cdo.get_editor_property("AbilitySystemComponent")
    asc.set_editor_property("StartupAbilityTags", [tag(ability_tag(key)) for key in ROOM_WAVES])
    commit_bp(blueprint)
    log("caster", "created" if created else "updated")


def run():
    REPORT[:] = []
    try:
        for key in WAVES:
            build_wave(key)
        build_caster()
        log("OK")
    except Exception:
        log("FAILED", traceback.format_exc())
    flush()


if __name__ == "__main__":
    run()
