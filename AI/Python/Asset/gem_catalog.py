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

# (Id, tier, display name, attribute or None, Add|Percent, magnitude per gem, effect, colour)
# A gem with no attribute can be owned and slotted but does nothing yet; its magnitude is only what the menus show. The
# effect is the stat the menus name before the magnitude, or a Core's whole rule (a Core has no magnitude). The colour
# is the EGeoColor of the gem's stat family. The tier is the list it is filed under.
GEMS = [
    ("Power", "Chip", "Power", "DamageMultiplier", "Add", 0.00667, "Damage", "DamageBoost"),
    ("Guard", "Chip", "Guard", "DamageReduction", "Add", 0.00667, "Damage reduction", "DamageReduction"),
    ("Vigor", "Chip", "Vigor", "MaxHealth", "Percent", 0.01, "Max health", "Heal"),
    ("Mend", "Chip", "Mend", "AppliedHealBoost", "Add", 0.00667, "Healing done", "HealBoost"),
    ("Swift", "Chip", "Swift", "MovementSpeedMultiplier", "Add", 0.00667, "Move speed", "MoveSpeed"),
    ("Precision", "Chip", "Precision", "CritChance", "Add", 0.01, "Crit chance", "AllyDamage"),
    ("Magazine", "Cut", "Magazine", "MaxAmmo", "Percent", 0.012, "Max ammo", "AllyDamage"),
    ("Recovery", "Cut", "Recovery", "ReceivedHealBoost", "Add", 0.007, "Healing received", "HealBoost"),
    ("Reflex", "Cut", "Reflex", None, "Add", -0.008, "Dash cooldown", "MoveSpeed"),
    ("Reload", "Cut", "Reload", None, "Add", 0.01, "Reload speed", "AllyDamage"),
    ("Stride", "Cut", "Stride", "DashDistanceMultiplier", "Add", 0.01, "Dash distance", "MoveSpeed"),
    ("WindUp", "Cut", "Wind-up", None, "Add", -0.006, "Ability wind-up time", "Neutral"),
    ("Flicker", "Cut", "Flicker", "DeployableBlinkMultiplier", "Add", 0.03, "Deployable blinking time", "DeployableNotBlocking"),
    ("Edge", "Cut", "Edge", "CritDamage", "Add", 0.02, "Crit damage", "DamageBoost"),
    ("Focus", "Prism", "Focus", None, "Add", -0.02, "Alternate Special cooldown", "Neutral"),
    ("Anchor", "Prism", "Anchor", "DeployableHealthMultiplier", "Add", 0.03, "Deployable health", "DeployableBlockingEnemies"),
    ("Linger", "Prism", "Linger", "DeployableDrainMultiplier", "Add", -0.03, "Deployable self drain", "DeployableNotBlocking"),
    ("Reach", "Prism", "Reach", None, "Add", 0.02, "Spell distance", "Neutral"),
    ("Surplus", "Core", "Surplus", None, "Add", 0.0,
     "+1 deployable charge: Wall, Healing zone or Turret", "DeployableNotBlocking"),
    ("Overclock", "Core", "Overclock", None, "Add", 0.0, "Deployable recharge -15%", "DeployableNotBlocking"),
    ("Relay", "Core", "Relay", None, "Add", 0.0,
     "Dash snaps to your nearest deployable within 30 degrees of the dash; no direction moves you toward the nearest one",
     "MoveSpeed"),
    ("Split", "Core", "Split", None, "Add", 0.0,
     "Deploy has a 25% chance to place a half-health copy next to it", "DeployableNotBlocking"),
    ("Critical", "Core", "Critical", None, "Add", 0.0, "Unlocks crits: 10% chance for 150% damage", "DamageBoost"),
    ("Bond", "Core", "Bond", None, "Add", 0.0, "Link to allies: you absorb 15% of the damage they take",
     "DamageReduction"),
    ("Wake", "Core", "Wake", None, "Add", 0.0, "Dash spawns your deployable at the start point (max 1 per 8 s)",
     "MoveSpeed"),
    ("Kinship", "Core", "Kinship", None, "Add", 0.0,
     "When your deployable dies, it spawns one of your ally's (max 1 per 5 s)", "DeployableNotBlocking"),
    ("Leverage", "Core", "Leverage", None, "Add", 0.0,
     "Deployable health x0.7 close up to x1.3 at max deploy distance", "DeployableBlockingEnemies"),
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

# Tag each rule gem grants while slotted, which the code of its rule checks.
GRANTED_TAGS = {"Critical": "Gem.Core.Critical", "Leverage": "Gem.Core.Leverage"}

# Attribute set owning each attribute that is not on CharacterAttributeSet.
ATTRIBUTE_OWNERS = {
    "MaxHealth": "GeoAttributeSetBase",
    "CritChance": "GeoGemAttributeSet",
    "CritDamage": "GeoGemAttributeSet",
    "DashDistanceMultiplier": "GeoGemAttributeSet",
    "DeployableBlinkMultiplier": "GeoGemAttributeSet",
    "DeployableHealthMultiplier": "GeoGemAttributeSet",
    "DeployableDrainMultiplier": "GeoGemAttributeSet",
}


def attribute_literal(name):
    owner = ATTRIBUTE_OWNERS.get(name, "CharacterAttributeSet")
    return '(AttributeName="{0}",Attribute=/Script/GeoTrinity.{1}:{0},AttributeOwner="/Script/CoreUObject.Class\'/Script/GeoTrinity.{1}\'")'.format(name, owner)


def gem_info(gem_id, display_name, attribute, operation, magnitude, effect, color):
    info = unreal.GeoGemInfo()
    text = ('(Id="{0}",DisplayName=NSLOCTEXT("GeoGem","{0}","{1}"),Operation={2},MagnitudePerGem={3},'
            'Effect=NSLOCTEXT("GeoGem","{0}_Effect","{4}"),Color=(Color={5})').format(
        gem_id, display_name, operation, magnitude, effect, color)
    if attribute:
        text += ",Attribute=" + attribute_literal(attribute)
    if gem_id in GRANTED_TAGS:
        text += ',GrantedTag=(TagName="{}")'.format(GRANTED_TAGS[gem_id])
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
