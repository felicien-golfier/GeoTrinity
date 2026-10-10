"""Core gem passives: one Blueprint ability per Core, registered in DA_AbilityInfo and wired to its gem row.

A Core's rule lives in a UGeoCorePassiveAbility subclass (C++). Native class defaults cannot carry asset tags (the
native gameplay tags do not exist yet when a CDO is built), so each Core gets a Blueprint child holding
Ability.Type.Passive + Ability.Spell.<Core>. The AbilityInfo entry is Shared with bGiveAtStartup=False: the abilities
page lists it only while the gem is slotted, and UGeoGemComponent::ApplyGems gives it (FGeoGemInfo::GrantedAbility,
the GrantedAbility column of Data/gems.csv).

Re-runnable: each Core's Blueprint is reused when it exists; its asset tags, the AbilityInfo entry and the CSV cell are set again. Add a Core to CORES once its
C++ class and Ability_Spell_<Core> tag are compiled. Headless:
UnrealEditor-Cmd.exe GeoTrinity.uproject -run=pythonscript -script=<this file> -unattended -nullrhi -NoP4
The report lands in REPORT.
"""
import csv
import io
import unreal

FOLDER = "/Game/AbilitySystem/Abilities/Gem"
ABILITY_INFO = "/Game/AbilitySystem/Data/DA_AbilityInfo"
CSV = "C:/GeoTrinity/Data/gems.csv"
REPORT = "C:/GeoTrinity/AI/Output/gem_core_passives_report.txt"

# (gem Id, C++ class, display name)
CORES = [
    ("Bond", "GeoBondPassiveAbility", "Bond"),
    ("Relay", "GeoRelayPassiveAbility", "Relay"),
    ("Wake", "GeoWakePassiveAbility", "Wake"),
    ("Split", "GeoSplitPassiveAbility", "Split"),
    ("Kinship", "GeoKinshipPassiveAbility", "Kinship"),
]

lines = []


def tag_container(names):
    container = unreal.GameplayTagContainer()
    container.import_text("(GameplayTags=(%s))" % ",".join('(TagName="%s")' % n for n in names))
    return container


def create_blueprint(gem_id, cpp_class):
    name = "GA_Core_" + gem_id
    path = FOLDER + "/" + name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        blueprint = unreal.load_asset(path)
    else:
        factory = unreal.BlueprintFactory()
        factory.set_editor_property("parent_class", unreal.load_class(None, "/Script/GeoTrinity." + cpp_class))
        blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, FOLDER, unreal.Blueprint, factory)
    cdo = unreal.get_default_object(blueprint.generated_class())
    cdo.set_editor_property("AbilityTags", tag_container(["Ability.Spell." + gem_id, "Ability.Type.Passive"]))
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False)
    lines.append("blueprint %s tags %s" % (path, cdo.get_editor_property("AbilityTags").export_text()))
    return "%s.%s_C" % (path, name)


def register(info, entries):
    """Shared entries replaced per Core, by tag or display name; no Description: Content/Data/AbilityDescriptions.txt owns it."""
    shared = list(info.get_editor_property("SharedAbilities"))
    for gem_id, class_path, display_name in entries:
        tag = "Ability.Spell." + gem_id
        shared = [e for e in shared
                  if tag not in e.export_text() and e.get_editor_property("AbilityDisplayName") != display_name]
        entry = unreal.PlayersGameplayAbilityInfo()
        entry.import_text(
            '(TypeOfAbilityTag=(TagName="Ability.Type.Passive"),bGiveAtStartup=False,'
            'AbilityClass="/Script/Engine.BlueprintGeneratedClass\'%s\'",AbilityTag=(TagName="%s"),'
            'AbilityDisplayName="%s")' % (class_path, tag, display_name))
        shared.append(entry)
    info.set_editor_property("SharedAbilities", shared)
    info.populate_ability_tags()
    unreal.EditorAssetLibrary.save_loaded_asset(info, only_if_is_dirty=False)


def point_csv_at(class_paths):
    """Sets the GrantedAbility cell of each Core's row; a quoted cell with a comma keeps its place."""
    with io.open(CSV, encoding="utf-8", newline="") as source:
        rows = list(csv.reader(source))
    for row in rows:
        if row[1] in class_paths:
            row[8:] = ["" if row[8].startswith("/Game") else row[8], class_paths[row[1]]]
    with io.open(CSV, "w", encoding="utf-8", newline="") as target:
        csv.writer(target, lineterminator="\n").writerows(rows)


def run():
    class_paths = {}
    entries = []
    for gem_id, cpp_class, display_name in CORES:
        class_paths[gem_id] = create_blueprint(gem_id, cpp_class)
        entries.append((gem_id, class_paths[gem_id], display_name))
    register(unreal.load_asset(ABILITY_INFO), entries)
    point_csv_at(class_paths)
    lines.append("csv imported=%s" % unreal.get_editor_subsystem(unreal.GeoGemCsvSync).import_csv())


try:
    run()
except Exception as error:
    lines.append("ERR %r" % error)
io.open(REPORT, "w", encoding="utf-8").write("\n".join(lines))
