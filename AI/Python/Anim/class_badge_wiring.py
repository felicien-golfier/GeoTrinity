"""Swap the playable characters onto the class badges: meshes, AnimBlueprints, ability montages, death montages.

    DA_PlayerClassData           each class's Mesh, AnimClass and DeathMontage
    BP_GeoPlayableCharacter      the mesh component's defaults, which show until a class is applied
    ability Blueprints           each one's AnimMontage
    SKM_<Class>Badge             material slot 0, the class's alive material, for the editor's own previews

At runtime a class's AliveMaterial replaces slot 0 anyway, so the mesh's own material only shows in the editor.
The data asset's class is not exposed to Python, but its map is reachable under its C++ name. Its entries are cloned
through exported text, since EditDefaultsOnly fields refuse the setter even on a copy.

Run AFTER every AI/Python/Anim/class_badge_*.py clip script, via mcp-unreal execute_script. Re-runnable.
Report written to Saved/class_badge_wiring.txt.
"""
import unreal

MESH_FOLDER = "/Game/Characters/Meshes/Class"
ANIM_FOLDER = "/Game/Characters/Anim/ClassBadge"
CLASS_DATA = "/Game/Characters/Playable/DA_PlayerClassData"
CHARACTER_BP = "/Game/Characters/Playable/BP_GeoPlayableCharacter"
DEFAULT_BADGE = "Square"  # the class the character Blueprint shows before one is applied
REPORT = unreal.Paths.project_saved_dir() + "class_badge_wiring.txt"

BADGES = {
    "Square": unreal.PlayerClass.SQUARE,
    "Triangle": unreal.PlayerClass.TRIANGLE,
    "Circle": unreal.PlayerClass.CIRCLE,
}

# Ability Blueprint -> the montage it plays, both by path.
ABILITY_MONTAGES = {
    "/Game/AbilitySystem/Abilities/Square/GA_Square_AutoProjectile":
        ANIM_FOLDER + "/Square/SK_SquareBadge_Montage_FirePiston",
    "/Game/AbilitySystem/Abilities/Square/SacrificeBeam/GA_Square_SpecAlt_SacrificeBeam":
        ANIM_FOLDER + "/Square/SK_SquareBadge_Montage_Sacrifice",
    "/Game/AbilitySystem/Abilities/Square/SacrificeBeam/GA_Square_SpecAlt_SacrificeDetonate":
        ANIM_FOLDER + "/Square/SK_SquareBadge_Montage_SacrificeSpit",
    "/Game/AbilitySystem/Abilities/Triangle/GA_Triangle_AutoProjectile":
        ANIM_FOLDER + "/Triangle/SK_TriangleBadge_Montage_FireCrossbow",
    "/Game/AbilitySystem/Abilities/Triangle/TurretRecall/GA_TurretRecall":
        ANIM_FOLDER + "/Triangle/SK_TriangleBadge_Montage_Recall",
    "/Game/AbilitySystem/Abilities/Triangle/Reload/GA_Reload":
        ANIM_FOLDER + "/Triangle/SK_TriangleBadge_Montage_Reload",
    "/Game/AbilitySystem/Abilities/Circle/ChargeBeam/GA_Circle_ChargeBeam":
        ANIM_FOLDER + "/Circle/SK_CircleBadge_Montage_ChargeOrbit",
    "/Game/AbilitySystem/Abilities/Circle/MoiraBeam/GA_MoiraBeam":
        ANIM_FOLDER + "/Circle/SK_CircleBadge_Montage_MoiraBeam",
}

LOG = []


def badge_assets(badge):
    return {"Mesh": unreal.load_asset("{}/SKM_{}Badge".format(MESH_FOLDER, badge)),
            "AnimClass": unreal.load_asset("{0}/{1}/SK_{1}Badge_AnimBlueprint".format(ANIM_FOLDER, badge))
            .generated_class(),
            "DeathMontage": unreal.load_asset("{0}/{1}/SK_{1}Badge_Montage_Death".format(ANIM_FOLDER, badge))}


def literal(asset):
    """An object reference as exported text, which is the only form an import merges in."""
    return '"{}\'{}\'"'.format(asset.get_class().get_path_name(), asset.get_path_name())


def wire_class_data():
    data_asset = unreal.load_asset(CLASS_DATA)
    table = data_asset.get_editor_property("ClassData")
    rebuilt = {}
    for player_class, entry in table.items():
        clone = unreal.PlayerClassData()
        clone.import_text(entry.export_text())
        badge = next((name for name, value in BADGES.items() if value == player_class), None)
        if badge:
            for field, asset in badge_assets(badge).items():
                clone.import_text("({}={})".format(field, literal(asset)))
        rebuilt[player_class] = clone
    data_asset.set_editor_property("ClassData", rebuilt)
    unreal.EditorAssetLibrary.save_loaded_asset(data_asset, only_if_is_dirty=False)

    for player_class, entry in unreal.load_asset(CLASS_DATA).get_editor_property("ClassData").items():
        LOG.append("class data {}: mesh {}, anim {}, death {}, alive material {}".format(
            player_class, entry.get_editor_property("mesh").get_name(), entry.get_editor_property("anim_class"),
            entry.get_editor_property("death_montage").get_name(),
            entry.get_editor_property("alive_material").get_name()))
    return table


def wire_materials(table):
    for badge, player_class in BADGES.items():
        mesh = unreal.load_asset("{}/SKM_{}Badge".format(MESH_FOLDER, badge))
        slots = list(mesh.get_editor_property("materials"))
        # A slot edited inside the array never writes back: clone it, set the clone, reassign the array.
        slot = unreal.SkeletalMaterial()
        slot.import_text(slots[0].export_text())
        slot.set_editor_property("material_interface", table[player_class].get_editor_property("alive_material"))
        mesh.set_editor_property("materials", [slot] + slots[1:])
        unreal.EditorAssetLibrary.save_loaded_asset(mesh, only_if_is_dirty=False)
        LOG.append("{} slot 0: {}".format(mesh.get_name(), mesh.get_editor_property("materials")[0]
                                          .get_editor_property("material_interface").get_name()))


def wire_abilities():
    for blueprint_path, montage_path in ABILITY_MONTAGES.items():
        blueprint = unreal.load_asset(blueprint_path)
        montage = unreal.load_asset(montage_path)
        if montage is None:
            raise RuntimeError("{} is missing — run its clip script first".format(montage_path))
        unreal.get_default_object(blueprint.generated_class()).set_editor_property("anim_montage", montage)
        # Instances copy only the defaults the last compile recorded, so an uncompiled default never reaches them.
        unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
        unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False)
        LOG.append("{}: {}".format(blueprint.get_name(), unreal.get_default_object(
            blueprint.generated_class()).get_editor_property("anim_montage").get_name()))


def wire_character():
    blueprint = unreal.load_asset(CHARACTER_BP)
    component = unreal.get_default_object(blueprint.generated_class()).get_editor_property("mesh")
    assets = badge_assets(DEFAULT_BADGE)
    component.set_editor_property("skeletal_mesh_asset", assets["Mesh"])
    component.set_editor_property("anim_class", assets["AnimClass"])
    unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False)
    LOG.append("{} mesh component: {}, {}".format(blueprint.get_name(),
                                                  component.get_editor_property("skeletal_mesh_asset").get_name(),
                                                  component.get_editor_property("anim_class")))


try:
    class_table = wire_class_data()
    wire_materials(class_table)
    wire_abilities()
    wire_character()
except Exception:
    import traceback
    LOG.append(traceback.format_exc())

with open(REPORT, "w") as handle:
    handle.write("\n".join(LOG))
