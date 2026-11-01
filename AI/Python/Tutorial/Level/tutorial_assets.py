"""
Tutorial assets for GeoTrinity: CT_Tutorial (every tutorial number, Steps.md "Tuning") and the three dummy StateTrees
/Game/AI/Dummy/ST_Dummy_Shooter, ST_Dummy_Chaser and ST_Dummy_Mover, shared with the Training Room. Enemy variants are setups on the
spawn actions (tutorial_encounters.py), never Blueprints.
The tutorial map and its encounters come from tutorial_map.py and tutorial_encounters.py.
Idempotent: assets are keyed by path. Report: AI/Output/tutorial_assets_report.txt.
Usage: headless (AI/Commands.md), editor closed or via MCP execute_script.
"""
import traceback

import unreal

REPORT_PATH = "C:/GeoTrinity/AI/Output/tutorial_assets_report.txt"
CSV_PATH = "C:/GeoTrinity/AI/Output/CT_Tutorial.csv"

CURVE_DIR = "/Game/AbilitySystem/Data/CurveTables"
CURVE_NAME = "CT_Tutorial"
ST_SRC = "/Game/AI/ST_EnemyBehaviour"
ST_SHOOTER = "/Game/AI/Dummy/ST_Dummy_Shooter"
ST_CHASER = "/Game/AI/Dummy/ST_Dummy_Chaser"
ST_MOVER = "/Game/AI/Dummy/ST_Dummy_Mover"
MOVER_TAG = "TargetPoint.Tutorial.Mover"
MOVER_ARENA_TAG = "Arena.Tutorial.Dummies"
SHOOT_ABILITY_TAG = "Ability.Spell.ProjectileToAllPlayers"

# Placeholder values until the user tunes them; the three columns are the difficulty levels Safe, Reduced, Original
# (CT_BossHex shape); a plain number is the same on all three. The tutorial always runs Safe (column 1).
ROWS = [
    ("MinibossHealth", 500.0),
    ("DummyHealth", 100.0),
    ("AllyDummyHealth", (50.0, 200.0, 200.0)),
    ("DummyLandingDamage", 10.0),
    ("DummyZoneDamage", 3.0),
    ("DummyZoneGrowthTime", 12.0),
    ("DummyZoneMaxRadius", 700.0),
    ("ColorZoneDamage", 8.5),  # per second: D(1-s) < h < D keeps every single zone lossy and the triple overlap a small gain
    ("ColorZoneShieldShare", 0.47),  # share s of the incoming damage the Shield zone removes
    ("ColorZoneHeal", 6.25),  # per second
    ("ColorZoneGrowSeconds", 8.0),
    ("ColorZoneMaxRadius", 1350.0),  # past the Zones room's corner (half 700 x 1100)
    ("ColorLineWelcomeSeconds", 5.0),  # the timed script of the Zones room (AI/Tutorial/Steps.md): one row per beat
    ("ColorLineHouseSeconds", 6.0),
    ("ColorLineShieldSeconds", 6.0),
    ("ColorLineHealSeconds", 6.0),
    ("ColorSurviveSeconds", 6.0),
    ("ColorLineRememberSeconds", 5.0),  # first line of the lethal-wave step
    ("ColorDeathLineSeconds", 5.0),
    ("HealCallPauseSeconds", 6.0),  # Deploy room: pause before each "I need healing" line
    ("MinibossProjectileDamage", (5.0, 8.0, 8.0)),
    ("MinibossProjectileSpeed", 600.0),
    ("AllyProjectileDamage", (15.0, 8.0, 8.0)),
    ("DummyZoneStartRadius", (10.0, 150.0, 150.0)),
    ("WaveAddLaneRadius", (127.0, 190.0, 190.0)),  # half-width of a dummy's safe lane
]

REPORT = []


def log(*args):
    REPORT.append(" ".join(str(a) for a in args))


def flush():
    with open(REPORT_PATH, "w", encoding="utf-8") as report:
        report.write("\n".join(REPORT))


def tag(name):
    value = unreal.GameplayTag()
    value.import_text('(TagName="%s")' % name)
    return value


def build_curve_table():
    """CT_Tutorial: one row per knob, a value or a (Safe, Reduced, Original) triple."""
    with open(CSV_PATH, "w") as handle:
        handle.write(",1,2,3\n")
        for name, value in ROWS:
            safe, reduced, original = value if isinstance(value, tuple) else (value,) * 3
            handle.write("%s,%s,%s,%s\n" % (name, safe, reduced, original))
    settings = unreal.CSVImportSettings()
    settings.set_editor_property("ImportType", unreal.CSVImportType.ECSV_CURVE_TABLE)
    settings.set_editor_property("ImportCurveInterpMode", unreal.RichCurveInterpMode.RCIM_CONSTANT)
    factory = unreal.CSVImportFactory()
    factory.set_editor_property("AutomatedImportSettings", settings)
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", CSV_PATH)
    task.set_editor_property("destination_path", CURVE_DIR)
    task.set_editor_property("destination_name", CURVE_NAME)
    task.set_editor_property("factory", factory)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    table = unreal.load_asset("%s/%s" % (CURVE_DIR, CURVE_NAME))
    log("curve table rows", [str(n) for n in unreal.DataTableFunctionLibrary.get_curve_table_row_names(table)])


def build_shooter_tree():
    """Shooter outside any arena: Root -> Shoot, the ability firing for as long as the dummy lives. The base tree's
    InitFight never opens without an arena."""
    if not unreal.EditorAssetLibrary.does_asset_exist(ST_SHOOTER):
        tree = unreal.EditorAssetLibrary.duplicate_asset(ST_SRC, ST_SHOOTER)
        builder = unreal.GeoStateTreeBuilderUtil.get_default_object()
        builder.remove_state(tree, "InitFight")
        builder.add_state(tree, "Shoot", "Root", 0)
        builder.add_fire_ability_task_to_state(tree, "Shoot", SHOOT_ABILITY_TAG)
        builder.remove_state(tree, "WaitFightToStart")
        unreal.EditorAssetLibrary.save_loaded_asset(tree)
        log("state tree", ST_SHOOTER)


def build_chaser_tree():
    """Chaser outside any arena: Root -> Chase, straight at the nearest player, no navmesh and no firing points."""
    if not unreal.EditorAssetLibrary.does_asset_exist(ST_CHASER):
        tree = unreal.EditorAssetLibrary.duplicate_asset(ST_SRC, ST_CHASER)
        builder = unreal.GeoStateTreeBuilderUtil.get_default_object()
        builder.remove_state(tree, "InitFight")
        builder.add_state(tree, "Chase", "Root", 0)
        builder.add_task_to_state(tree, "Chase", "STTask_ChaseTarget")
        builder.remove_state(tree, "WaitFightToStart")
        unreal.EditorAssetLibrary.save_loaded_asset(tree)
        log("state tree", ST_CHASER)


def build_mover_tree():
    """Mover outside any arena gate: Root -> Patrol, walking from one `TargetPoint.Tutorial.Mover` point of its room to
    another, forever (the state re-enters itself, each entry picks a new point)."""
    if not unreal.EditorAssetLibrary.does_asset_exist(ST_MOVER):
        tree = unreal.EditorAssetLibrary.duplicate_asset(ST_SRC, ST_MOVER)
        builder = unreal.GeoStateTreeBuilderUtil.get_default_object()
        builder.remove_state(tree, "InitFight")
        builder.add_state(tree, "Patrol", "Root", 0)
        builder.add_task_to_state(tree, "Patrol", "STTask_MoveToTargetPoint")
        builder.set_task_property(tree, "Patrol", "STTask_MoveToTargetPoint", "PointTag", '(TagName="%s")' % MOVER_TAG)
        builder.set_task_property(tree, "Patrol", "STTask_MoveToTargetPoint", "ArenaTag", '(TagName="%s")' % MOVER_ARENA_TAG)
        builder.add_transition(tree, "Patrol", "Patrol", unreal.StateTreeTransitionTrigger.ON_STATE_COMPLETED)
        builder.remove_state(tree, "WaitFightToStart")
        unreal.EditorAssetLibrary.save_loaded_asset(tree)
        log("state tree", ST_MOVER)


def run():
    REPORT[:] = []
    try:
        build_curve_table()
        build_shooter_tree()
        build_chaser_tree()
        build_mover_tree()
        log("OK")
    except Exception:
        log("FAILED", traceback.format_exc())
    flush()


if __name__ == "__main__":
    run()
