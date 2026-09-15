"""
Splits the boss StateTrees into one shared base and one behaviour tree per boss.

ST_EnemyBehaviour becomes the base every boss runs: InitFight, entered once the blackboard has the enemy aggroed and
its fight started, holds a single Linked Asset state, Behaviour, tagged AI.Boss.Behaviour; until then
WaitFightToStart loops a short delay back to the root. Each boss Blueprint names its own tree in BehaviourStateTree,
which AGeoEnemyAIController hands to that tag when the logic starts.

The star's spell chain moves to ST_StarBossBehaviour, a copy of the old base without its gate. The hex tree keeps
its asset and drops its own gate, which the base now holds.

Every builder call compiles the asset, so a state is removed only once nothing outside it transitions into it.
Re-runnable: a boss whose Blueprint already names a BehaviourStateTree is skipped.

Reference: AI/MCP/MCP_StateTree.md. Nothing a script prints comes back through MCP, so the run report is written to
REPORT_PATH.

Stages (call from execute_script):
    build()   # star tree, base tree, hex tree, boss Blueprints
"""
import traceback

import unreal

REPORT_PATH = "C:/GeoTrinity/Saved/boss_behaviour_trees_report.txt"

BASE = "/Game/AI/ST_EnemyBehaviour"
STAR = "/Game/AI/ST_StarBossBehaviour"
HEX = "/Game/AI/ST_HexBossBehaviour"
BP_STAR = "/Game/Characters/Enemies/BP_StarBoss"
BP_HEX = "/Game/Characters/Enemies/HEX/BP_HexBoss"

BEHAVIOUR_TAG = "AI.Boss.Behaviour"
BASE_GATE = "InitFight"
BASE_WAIT = "WaitFightToStart"
BEHAVIOUR = "Behaviour"
STAR_ENTRY = "FireAll"
HEX_GATE = "WaitForFightToStart"

REPORT = []


def log(*args):
    REPORT.append(" ".join(str(a) for a in args))


def flush():
    with open(REPORT_PATH, "w", encoding="utf-8") as handle:
        handle.write("\n".join(REPORT))


def builder():
    return unreal.GeoStateTreeBuilderUtil.get_default_object()


def boss_cdo(blueprint):
    return unreal.get_default_object(blueprint.generated_class())


def commit_bp(blueprint):
    """A CDO write only reaches spawned actors once the Blueprint is recompiled."""
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    unreal.EditorAssetLibrary.save_loaded_asset(blueprint)


def point_boss_at(blueprint, base, behaviour):
    cdo = boss_cdo(blueprint)
    cdo.set_editor_property("StateTree", base)
    cdo.set_editor_property("BehaviourStateTree", behaviour)
    commit_bp(blueprint)
    log("Blueprint", blueprint.get_name(), "->", base.get_name(), "+", behaviour.get_name())


def build_star_and_base(star_bp):
    """The star copy is taken before the base loses the states it copies."""
    if unreal.EditorAssetLibrary.does_asset_exist(STAR):
        star = unreal.load_asset(STAR)
        log("StateTree", STAR, "exists")
    else:
        star = unreal.EditorAssetLibrary.duplicate_asset(BASE, STAR)
        builder().clear_enter_conditions(star, BASE_GATE)
        builder().remove_state(star, BASE_WAIT)
        log("StateTree", STAR, "copied from the base, gate removed")

    base = unreal.load_asset(BASE)
    builder().remove_state(base, STAR_ENTRY)
    builder().remove_state(base, BEHAVIOUR)
    builder().add_state(base, BEHAVIOUR, BASE_GATE, -1)
    builder().set_linked_asset_state(base, BEHAVIOUR, None, BEHAVIOUR_TAG)
    log("StateTree", BASE, "reduced to its gate and the", BEHAVIOUR_TAG, "linked state")

    point_boss_at(star_bp, base, star)


def build_hex(hex_bp):
    hex_tree = unreal.load_asset(HEX)
    builder().remove_state(hex_tree, HEX_GATE)
    log("StateTree", HEX, "gate removed")

    point_boss_at(hex_bp, unreal.load_asset(BASE), hex_tree)


def build_is_loaded():
    """Without the rebuilt modules the split would stop halfway, after the base has lost its states."""
    return (hasattr(unreal.GeoStateTreeBuilderUtil, "set_linked_asset_state")
            and hasattr(unreal.EnemyCharacter, "behaviour_state_tree"))


def build():
    REPORT[:] = []
    try:
        if not build_is_loaded():
            log("ABORTED: the editor runs a build without SetLinkedAssetState / BehaviourStateTree")
            flush()
            return
        star_bp = unreal.load_asset(BP_STAR)
        if boss_cdo(star_bp).get_editor_property("BehaviourStateTree"):
            log("Blueprint", BP_STAR, "already split, skipped")
        else:
            build_star_and_base(star_bp)

        hex_bp = unreal.load_asset(BP_HEX)
        if boss_cdo(hex_bp).get_editor_property("BehaviourStateTree"):
            log("Blueprint", BP_HEX, "already split, skipped")
        else:
            build_hex(hex_bp)
    except Exception:
        log("FAILED:", traceback.format_exc())
    flush()
