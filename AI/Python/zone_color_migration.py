"""
Moves every zone onto the shared BP_Zone: the colour and the attitude filter that used to be baked into a per-effect
zone Blueprint now travel with the ability, in FDeployableDataParams.

Idempotent, so it can be run before and after the build that adds FDeployableDataParams::Attitude — the attitude pass
reports itself as skipped until that field exists.
"""

import traceback

import unreal

RESULT_PATH = r"C:\GeoTrinity\Saved\ZoneColorMigration.txt"

FRIENDLY_OR_NEUTRAL = 3
HOSTILE = 4
ALL_ATTITUDES = 7

# Ability -> what its zone (or burst) draws in and acts on. The ability's own TelegraphCue already names the slot;
# the zone now reads the same meaning off Params so one Blueprint can serve all of them.
ABILITY_ZONE_PARAMS = {
    "/Game/AbilitySystem/Abilities/Enemy/Tutorial/GA_Tutorial_DamageOverTime": ("DAMAGE", HOSTILE),
    "/Game/AbilitySystem/Abilities/Enemy/Tutorial/GA_Tutorial_HealOverTime": ("HEAL", HOSTILE),
    "/Game/AbilitySystem/Abilities/Enemy/Tutorial/GA_Tutorial_Lethal": ("LETHAL_DAMAGE", HOSTILE),
    "/Game/AbilitySystem/Abilities/Enemy/Tutorial/GA_Tutorial_DamageReduction": ("DAMAGE_REDUCTION", HOSTILE),
}

# The circle's healing zone keeps its own class, but its colour and who it reaches now come from the ability too.
HEALING_ABILITY = "/Game/AbilitySystem/Abilities/Circle/HealingZone/GA_DeployHealingZone"

# Hand-placed zones keep Details-panel fields of their own; only the colour was ever missing.
PLACED_ZONE_COLORS = {
    "/Game/Actors/BP_Zone": "NEUTRAL",
    "/Game/Actors/BP_DamageZone": "DAMAGE",
    "/Game/Actors/BP_HealingZone": "HEAL",
    "/Game/AbilitySystem/Abilities/Circle/HealingZone/BP_Deployable_HealingZone": "HEAL",
}

OBSOLETE_ZONE_BLUEPRINTS = [
    "/Game/Actors/Tutorial/BP_Zone_DamageOverTime",
    "/Game/Actors/Tutorial/BP_Zone_HealOverTime",
    "/Game/Actors/Tutorial/BP_Zone_Lethal",
    "/Game/Actors/Tutorial/BP_Zone_DamageReduction",
]

report = []


def set_params(cdo, params_name, slot, attitude):
    """Writes slot and attitude into <params_name>. PyUnreal hands out structs by value, so every level of the
    nesting has to be pushed back up. Attitude is skipped when the build predates the field."""
    params = cdo.get_editor_property(params_name)
    color_param = params.get_editor_property("Color")
    color_param.set_editor_property("Color", getattr(unreal.GeoColor, slot))
    params.set_editor_property("Color", color_param)
    written = "Color={}".format(slot)
    try:
        params.set_editor_property("Attitude", attitude)
        written += " Attitude={}".format(attitude)
    except Exception:
        written += " Attitude=SKIPPED (not in this build)"
    cdo.set_editor_property(params_name, params)
    return written


def save(asset, path):
    saved = unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)
    report.append("  saved={} {}".format(saved, path))


try:
    for path, (slot, attitude) in ABILITY_ZONE_PARAMS.items():
        blueprint = unreal.load_asset(path)
        cdo = unreal.get_default_object(blueprint.generated_class())
        report.append("{} ZoneParams: {}, ZoneClass cleared".format(path, set_params(cdo, "ZoneParams", slot, attitude)))
        cdo.set_editor_property("ZoneClass", None)
        save(blueprint, path)

    blueprint = unreal.load_asset(HEALING_ABILITY)
    cdo = unreal.get_default_object(blueprint.generated_class())
    report.append("{} Params: {}".format(HEALING_ABILITY, set_params(cdo, "Params", "HEAL", FRIENDLY_OR_NEUTRAL)))
    save(blueprint, HEALING_ABILITY)

    for path, slot in PLACED_ZONE_COLORS.items():
        blueprint = unreal.load_asset(path)
        cdo = unreal.get_default_object(blueprint.generated_class())
        color_param = cdo.get_editor_property("Color")
        color_param.set_editor_property("Color", getattr(unreal.GeoColor, slot))
        cdo.set_editor_property("Color", color_param)
        report.append("{} Color -> {}".format(path, slot))
        save(blueprint, path)

    for path in OBSOLETE_ZONE_BLUEPRINTS:
        if unreal.EditorAssetLibrary.does_asset_exist(path):
            report.append("deleted={} {}".format(unreal.EditorAssetLibrary.delete_asset(path), path))
except Exception:
    report.append(traceback.format_exc())

with open(RESULT_PATH, "w") as f:
    f.write("\n".join(report))
