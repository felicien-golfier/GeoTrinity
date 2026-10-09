"""Create or rewrite DA_GemCatalog, the UGeoGemCatalog every gem type lives in.

Rewrites the drop tables and class XP below in place when the asset exists, so references to it survive. The gems
themselves come from Data/gems.csv, which UGeoGemCsvSync keeps equal to the asset both ways in the open editor;
this script only imports it, for edits made while no editor was open. Writes a readback of every row to
AI/Output/gem_catalog.txt. Runs in the open editor or headless:
UnrealEditor-Cmd GeoTrinity.uproject -run=pythonscript -script=AI/Python/Asset/gem_catalog.py
"""
import traceback

import unreal

ASSET_DIR = "/Game/AbilitySystem/Data"
ASSET_NAME = "DA_GemCatalog"
REPORT = r"C:\GeoTrinity\AI\Output\gem_catalog.txt"
GEMS_CSV = r"C:\GeoTrinity\AI\Data\gems.csv"

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
    catalog.set_editor_property("drop_tables", {getattr(unreal.GeoBossType, BOSS_TYPES[boss_type]): drop_table(*table)
                                                for boss_type, table in DROP_TABLES.items()})
    catalog.set_editor_property("xp_per_health_bar", {getattr(unreal.GeoBossType, BOSS_TYPES[boss_type]): xp
                                                      for boss_type, xp in XP_PER_HEALTH_BAR.items()})
    saved = unreal.EditorAssetLibrary.save_loaded_asset(catalog, only_if_is_dirty=False)
    imported = unreal.get_editor_subsystem(unreal.GeoGemCsvSync).import_csv()

    lines = ["saved={}".format(saved), "gems imported from {}={}".format(GEMS_CSV, imported)]
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
