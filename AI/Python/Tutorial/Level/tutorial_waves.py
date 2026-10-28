"""
The lateral-wave ability of the tutorial (AI/Tutorial/Steps.md steps 8-10).

One wave for every firing: a pattern Blueprint (child of ULateralWavePattern, the geometry and look) and a PatternAbility
Blueprint (the lethal effect, the wind-up, the wave start point), registered in the enemy ability catalog. What changes
per firing is runtime data, never an asset: the zone room's safe spots are the encounter action's PatternData
(tutorial_encounters.py), the miniboss's lanes come from its living adds. Zones fires it on the encounters' shared
caster (UGeoEncounterSubsystem), the miniboss from its own BossSetup. The per-wave assets of the old layout (TutorialWaves/) are
deleted with their catalog entries, the old caster Blueprint once nothing references it (re-run after
tutorial_encounters.py).
Idempotent: assets keyed by path, properties rewritten. Report: AI/Output/tutorial_waves_report.txt.
Usage: headless (AI/Commands.md), editor closed; Config/Tags/GeoGameplayTags.ini already holds the tag.
"""
import traceback

import unreal

REPORT_PATH = "C:/GeoTrinity/AI/Output/tutorial_waves_report.txt"
ABILITY_DIR = "/Game/AbilitySystem/Abilities/Enemy/LateralWave"
OLD_ABILITY_DIR = "/Game/AbilitySystem/Abilities/Enemy/TutorialWaves"
OLD_CASTER = "/Game/Characters/Enemies/Tutorial/BP_HiddenCaster_TutorialWaves"
CATALOG = "/Game/AbilitySystem/Data/DA_AbilityInfo"
BEAM_SYSTEM = "/Game/Art/VFX/Generic/Niagara/NS_Beam"
MASK_COLLECTION = "/Game/Art/VFX/Abilities/Enemies/PillarsAOE/MPC_MaskedArea"
WAVE_START_TAG ="TargetPoint.Tutorial.WaveStart"
WAVE_TAG = "Ability.Spell.LateralWave"
LETHAL_EFFECT = ("/Script/GeoTrinity.LethalEffectData(OneShotCue=(CueTag=(TagName=\"\"),Color=Neutral,"
                 "SecondaryColors=,SoundTag=(TagName=\"\")),Difficulties=7)")

# The zone room is 1400 long (X) and 2200 wide (Y): the front starts on the wave point near the entry end and crosses
# the room along +X; safe spots are offsets from that point (tutorial_encounters.py).
TRAVEL = 1300.0
HALF_WIDTH = 1200.0
LANE_WIDTH = 380.0
BAND_WIDTH = 300.0
SPEED = 1400.0
WIND_UP = 3.0

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


def build_wave():
    pattern_bp, _ = make_bp("BP_LateralWave", ABILITY_DIR, unreal.load_class(None, "/Script/GeoTrinity.LateralWavePattern"))
    cdo = unreal.get_default_object(pattern_bp.generated_class())
    cdo.set_editor_property("Speed", SPEED)
    cdo.set_editor_property("TravelDistance", TRAVEL)
    cdo.set_editor_property("BandWidth", BAND_WIDTH)
    cdo.set_editor_property("HalfWidth", HALF_WIDTH)
    cdo.set_editor_property("SpotRadius", LANE_WIDTH * 0.5)
    cdo.set_editor_property("SafeSpotSources", [unreal.SafeSpotSource.FIXED, unreal.SafeSpotSource.ADDS])
    cdo.set_editor_property("BeamVfxSystem", unreal.load_asset(BEAM_SYSTEM))
    color = unreal.GeoColorParam()
    color.import_text("(Color=LethalDamage)")
    cdo.set_editor_property("BeamColor", color)
    cdo.set_editor_property("MaskMaterialParameterCollection", unreal.load_asset(MASK_COLLECTION))
    commit_bp(pattern_bp)

    ability_bp, created = make_bp("GA_LateralWave", ABILITY_DIR, unreal.load_class(None, "/Script/GeoTrinity.PatternAbility"))
    cdo = unreal.get_default_object(ability_bp.generated_class())
    tags = unreal.GameplayTagContainer()
    tags.import_text('(GameplayTags=((TagName="%s")))' % WAVE_TAG)
    cdo.set_editor_property("AbilityTags", tags)
    cdo.set_editor_property("PatternToLaunch", pattern_bp.generated_class())
    cdo.set_editor_property("TargetPointTag", tag(WAVE_START_TAG))
    cdo.set_editor_property("FireDelay", WIND_UP)
    effect = unreal.InstancedStruct()
    effect.import_text(LETHAL_EFFECT)
    cdo.set_editor_property("EffectDataInstances", [effect])
    commit_bp(ability_bp)
    register(ability_bp, WAVE_TAG, "Lateral wave",
             "Lethal wave crossing the room sideways; safe lanes given per firing or through living adds.")
    log("wave", "created" if created else "updated")
    return ability_bp


def delete_old_waves():
    """Drops the per-wave assets of the old layout and their catalog entries."""
    catalog = unreal.load_asset(CATALOG)
    infos = catalog.get_editor_property("EnemyAbilityInfos")
    kept = [info for info in infos if OLD_ABILITY_DIR + "/" not in info.export_text()]
    if len(kept) != len(infos):
        catalog.set_editor_property("EnemyAbilityInfos", kept)
        unreal.EditorAssetLibrary.save_loaded_asset(catalog, only_if_is_dirty=False)
        log("catalog entries removed", len(infos) - len(kept))
    if unreal.EditorAssetLibrary.does_directory_exist(OLD_ABILITY_DIR):
        for path in unreal.EditorAssetLibrary.list_assets(OLD_ABILITY_DIR, recursive=True, include_folder=False):
            log("deleted" if unreal.EditorAssetLibrary.delete_asset(path) else "NOT deleted", path)
        unreal.EditorAssetLibrary.delete_directory(OLD_ABILITY_DIR)


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


def delete_unreferenced(paths, log_to):
    """Deletes each asset of paths that nothing outside paths references; logs what it kept and why."""
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    options = unreal.AssetRegistryDependencyOptions(include_soft_package_references=True,
                                                    include_hard_package_references=True)
    for path in paths:
        if unreal.EditorAssetLibrary.does_asset_exist(path):
            referencers = [str(name) for name in (registry.get_referencers(path, options) or [])
                           if str(name) not in paths]
            if referencers:
                log_to("kept, still referenced by", referencers, path)
            else:
                log_to("deleted" if unreal.EditorAssetLibrary.delete_asset(path) else "NOT deleted", path)


def run():
    REPORT[:] = []
    try:
        build_wave()
        delete_old_waves()
        delete_unreferenced([OLD_CASTER], log)
        log("OK")
    except Exception:
        log("FAILED", traceback.format_exc())
    flush()


if __name__ == "__main__":
    run()
