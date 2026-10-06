"""Create or rewrite DA_GemCatalog, the UGeoGemCatalog every gem type lives in.

Rewrites the asset in place when it exists, so references to it survive. Writes a readback of every row to
AI/Output/gem_catalog.txt. Runs in the open editor or headless:
UnrealEditor-Cmd GeoTrinity.uproject -run=pythonscript -script=AI/Python/Asset/gem_catalog.py
"""
import traceback

import unreal

ASSET_DIR = "/Game/AbilitySystem/Data"
ASSET_NAME = "DA_GemCatalog"
REPORT = r"C:\GeoTrinity\AI\Output\gem_catalog.txt"

# (Id, tier, display name, CharacterAttributeSet attribute or None, Add|Percent, magnitude per gem)
# A gem with no attribute can be owned and slotted but does nothing yet. The tier is the list it is filed under.
GEMS = [
    ("Power", "Chip", "Power", "DamageMultiplier", "Add", 0.00667),
    ("Guard", "Chip", "Guard", "DamageReduction", "Add", 0.00667),
    ("Vigor", "Chip", "Vigor", "MaxHealth", "Percent", 0.01),
    ("Mend", "Chip", "Mend", "AppliedHealBoost", "Add", 0.00667),
    ("Swift", "Chip", "Swift", "MovementSpeedMultiplier", "Add", 0.00667),
    ("Precision", "Chip", "Precision", None, "Add", 0.0),
    ("Magazine", "Cut", "Magazine", "MaxAmmo", "Percent", 0.012),
    ("Recovery", "Cut", "Recovery", "ReceivedHealBoost", "Add", 0.007),
    ("Reflex", "Cut", "Reflex", None, "Add", 0.0),
    ("Reload", "Cut", "Reload", None, "Add", 0.0),
    ("Stride", "Cut", "Stride", None, "Add", 0.0),
    ("WindUp", "Cut", "Wind-up", None, "Add", 0.0),
    ("Flicker", "Cut", "Flicker", None, "Add", 0.0),
    ("Edge", "Cut", "Edge", None, "Add", 0.0),
    ("Focus", "Prism", "Focus", None, "Add", 0.0),
    ("Anchor", "Prism", "Anchor", None, "Add", 0.0),
    ("Linger", "Prism", "Linger", None, "Add", 0.0),
    ("Reach", "Prism", "Reach", None, "Add", 0.0),
    ("Surplus", "Core", "Surplus", None, "Add", 0.0),
    ("Overclock", "Core", "Overclock", None, "Add", 0.0),
    ("Relay", "Core", "Relay", None, "Add", 0.0),
    ("Split", "Core", "Split", None, "Add", 0.0),
    ("Critical", "Core", "Critical", None, "Add", 0.0),
    ("Bond", "Core", "Bond", None, "Add", 0.0),
    ("Wake", "Core", "Wake", None, "Add", 0.0),
    ("Kinship", "Core", "Kinship", None, "Add", 0.0),
    ("Leverage", "Core", "Leverage", None, "Add", 0.0),
]

# Per boss type at Original: (min gems, max gems, {tier: chance of each gem}). Each gem rolls from the highest tier
# down, Chip being what is left: 3% Prism, else 16% Cut gives about 1.3 Prisms and 7 Cuts out of 44. Bosses never
# drop Cores (crafted only); mini-bosses never drop Prisms either.
DROP_TABLES = {
    "Boss": (38, 50, {"Prism": 0.03, "Cut": 0.16}),
    "MiniBoss": (38, 50, {"Cut": 0.16}),
}
# Class XP for a whole health bar at Original, per boss type.
XP_PER_HEALTH_BAR = {"Boss": 400.0, "MiniBoss": 400.0}

# Enum values as the Python API names them.
BOSS_TYPES = {"Boss": "BOSS", "MiniBoss": "MINI_BOSS"}
TIERS = {"Chip": "CHIP", "Cut": "CUT", "Prism": "PRISM", "Core": "CORE"}

# Attributes declared on the base set keep it as their owner.
BASE_SET_ATTRIBUTES = {"MaxHealth"}


def attribute_literal(name):
    owner = "GeoAttributeSetBase" if name in BASE_SET_ATTRIBUTES else "CharacterAttributeSet"
    return '(AttributeName="{0}",Attribute=/Script/GeoTrinity.{1}:{0},AttributeOwner="/Script/CoreUObject.Class\'/Script/GeoTrinity.{1}\'")'.format(name, owner)


def gem_info(gem_id, display_name, attribute, operation, magnitude):
    info = unreal.GeoGemInfo()
    text = '(Id="{}",DisplayName=NSLOCTEXT("GeoGem","{}","{}"),Operation={},MagnitudePerGem={}'.format(
        gem_id, gem_id, display_name, operation, magnitude)
    if attribute:
        text += ",Attribute=" + attribute_literal(attribute)
    info.import_text(text + ")")
    return info


def gem_list(tier):
    gems = unreal.GeoGemList()
    gems.set_editor_property("gems", [gem_info(gem[0], *gem[2:]) for gem in GEMS if gem[1] == tier])
    return gems


def drop_table(min_count, max_count, chances):
    table = unreal.GeoGemDropTable()
    table.set_editor_property("min_count", min_count)
    table.set_editor_property("max_count", max_count)
    table.set_editor_property("tier_chances", {getattr(unreal.GeoGemTier, TIERS[tier]): chance
                                               for tier, chance in chances.items()})
    return table


def run():
    path = "{}/{}".format(ASSET_DIR, ASSET_NAME)
    catalog = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else \
        unreal.AssetToolsHelpers.get_asset_tools().create_asset(ASSET_NAME, ASSET_DIR, unreal.GeoGemCatalog,
                                                               unreal.DataAssetFactory())
    catalog.set_editor_property("gems_by_tier", {getattr(unreal.GeoGemTier, value): gem_list(tier)
                                                 for tier, value in TIERS.items()})
    catalog.set_editor_property("drop_tables", {getattr(unreal.GeoBossType, BOSS_TYPES[boss_type]): drop_table(*table)
                                                for boss_type, table in DROP_TABLES.items()})
    catalog.set_editor_property("xp_per_health_bar", {getattr(unreal.GeoBossType, BOSS_TYPES[boss_type]): xp
                                                      for boss_type, xp in XP_PER_HEALTH_BAR.items()})
    saved = unreal.EditorAssetLibrary.save_loaded_asset(catalog, only_if_is_dirty=False)

    lines = ["saved={}".format(saved)]
    saved_catalog = unreal.load_asset(path)
    lines += ["{}: {}".format(tier, gems.export_text())
              for tier, gems in saved_catalog.get_editor_property("gems_by_tier").items()]
    lines += ["{}: {}".format(boss_type, table.export_text())
              for boss_type, table in saved_catalog.get_editor_property("drop_tables").items()]
    lines += ["xp_per_health_bar: {}".format(dict(saved_catalog.get_editor_property("xp_per_health_bar")))]
    return lines


try:
    report = run()
except Exception:
    report = [traceback.format_exc()]
with open(REPORT, "w") as f:
    f.write("\n".join(report))
