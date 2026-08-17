"""
Tutorial room builder for GeoTrinity (DraftMap).

Builds the training room the players spawn into: its AI StateTrees, its dummy Blueprints, and the
room itself (floor, walls, dummy arenas, hazard zones, class-change pads, teleporter pair, player
starts). Idempotent — assets and actors are keyed by path/label, so a re-run updates instead of
duplicating.

The room sits outside the level's NavMeshBoundsVolume: nothing in it paths, the moving dummy chases
in a straight line. Widen that volume first if a MoveTo/firing-point patrol is ever wanted here.

Reference: AI/MCP/MCP_Blueprint.md, AI/MCP/MCP_StateTree.md.
Nothing a script prints comes back through MCP, so the run report is written to REPORT_PATH.

Stages (call from execute_script):
    build_assets()   # StateTrees + dummy Blueprints
    build_level()    # room geometry, arenas, zones, teleporters, player starts
    retag()          # stamps Arena.Tutorial once the tag is registered (needs an editor restart)
"""
import unreal

REPORT_PATH = r"C:/Users/Felou/AppData/Local/Temp/claude/C--GeoTrinity/2eff631f-411a-4c9e-9b77-6faa22a0b2c6/scratchpad/tutorial_report.txt"

# Arena.Tutorial only resolves after an editor restart (Config/Tags/GeoGameplayTags.ini is read at
# startup), so the teleporter pair ships on a tag that already exists and retag() moves it later.
TELEPORT_TAG = "Arena.Entrance"
TUTORIAL_TAG = "Arena.Tutorial"

ST_SRC = "/Game/AI/ST_EnemyBehaviour"
ST_DIR = "/Game/AI/Tutorial"
BP_DIR = "/Game/Characters/Enemies/Tutorial"
BP_DUMMY = "/Game/Characters/Enemies/BP_Dummy"

# Room: 1400 x 2000, walled, reachable only through the teleporter pair.
CX, CY = -2000.0, 0.0
HALF_X, HALF_Y = 700.0, 1000.0

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


def actors():
    return unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()


def find_actor(label):
    for actor in actors():
        if actor.get_actor_label() == label:
            return actor
    return None


# ---------------------------------------------------------------- assets


def dup_asset(src, dst):
    """Returns (asset, created)."""
    if unreal.EditorAssetLibrary.does_asset_exist(dst):
        return unreal.load_asset(dst), False
    return unreal.EditorAssetLibrary.duplicate_asset(src, dst), True


def make_child_bp(name, parent_class):
    """Returns (blueprint, created)."""
    path = "%s/%s" % (BP_DIR, name)
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.load_asset(path), False
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("ParentClass", parent_class)
    blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, BP_DIR, unreal.Blueprint, factory)
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    return blueprint, True


def build_state_trees():
    """Three trims of the star boss's own tree — same tasks, none of its movement or wave states.

    Every builder call compiles the asset, so a transition must be cleared *before* the state it points
    at is removed — a dangling transition fails that compile and raises a blocking ensure.
    """
    builder = unreal.GeoStateTreeBuilderUtil.get_default_object()
    trees = {}

    # Bullets totem: Dormant -> Wait -> FireAll(ProjectileToAllPlayers). That ability re-schedules its
    # own salves forever, so the state never completing is exactly the behaviour wanted.
    tree, created = dup_asset(ST_SRC, ST_DIR + "/ST_Tutorial_Bullets")
    if created:
        builder.clear_transitions(tree, "FireAll")
        builder.remove_state(tree, "Behaviour")
    trees["bullets"] = tree

    # Zones totem: same opening, but firing the pillar zone and looping back through the delay state.
    tree, created = dup_asset(ST_SRC, ST_DIR + "/ST_Tutorial_Zones")
    if created:
        builder.clear_transitions(tree, "FireAll")
        builder.remove_state(tree, "Behaviour")
        builder.replace_fire_ability_tag_in_state(tree, "FireAll", "Ability.Spell.SpawnPillar")
        builder.add_transition(tree, "FireAll", "Wait", unreal.StateTreeTransitionTrigger.ON_STATE_COMPLETED)
    trees["zones"] = tree

    # Moving dummy: chases whoever is nearest, straight line, no navmesh and no firing points needed.
    tree, created = dup_asset(ST_SRC, ST_DIR + "/ST_Tutorial_Chase")
    if created:
        builder.clear_transitions(tree, "Dormant")
        builder.clear_transitions(tree, "FireAll")
        builder.remove_state(tree, "InitFight")
        builder.remove_state(tree, "Behaviour")
        builder.add_state(tree, "Chase", "Root", -1)
        builder.add_task_to_state(tree, "Chase", "STTask_ChaseTarget")
        builder.add_transition(tree, "Dormant", "Chase", unreal.StateTreeTransitionTrigger.ON_EVENT,
                               "AI.Boss.AggroEvent")
    trees["chase"] = tree

    for key, tree in trees.items():
        log("StateTree", key, "->", tree.get_path_name())
    return trees


def build_blueprints(trees):
    dummy_class = unreal.load_asset(BP_DUMMY).generated_class()
    made = {}

    specs = [
        ("BP_TutorialChaser", {"StateTree": trees["chase"]}, None),
        ("BP_TutorialAlly", {"TeamId": unreal.Team.PLAYER}, None),
        ("BP_TutorialTotemBullets", {"StateTree": trees["bullets"]}, ["Ability.Spell.ProjectileToAllPlayers"]),
        ("BP_TutorialTotemZones", {"StateTree": trees["zones"]}, ["Ability.Spell.SpawnPillar"]),
    ]
    for name, props, ability_tags in specs:
        blueprint, created = make_child_bp(name, dummy_class)
        cdo = unreal.get_default_object(blueprint.generated_class())
        for prop, value in props.items():
            cdo.set_editor_property(prop, value)
        if ability_tags:
            asc = cdo.get_editor_property("AbilitySystemComponent")
            asc.set_editor_property("StartupAbilityTags", [tag(t) for t in ability_tags])
        unreal.EditorAssetLibrary.save_loaded_asset(blueprint)
        made[name] = blueprint
        log("Blueprint", name, "created" if created else "updated", blueprint.get_path_name())
    return made


def build_assets():
    REPORT[:] = []
    try:
        trees = build_state_trees()
        build_blueprints(trees)
        log("ASSETS OK")
    except Exception as error:
        import traceback
        log("ASSETS FAILED:", traceback.format_exc())
    flush()


# ---------------------------------------------------------------- level


def dup_actor(source_label, new_label, location, scale=None, rotation=None):
    subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actor = find_actor(new_label)
    if actor is None:
        source = find_actor(source_label)
        if source is None:
            log("  MISSING SOURCE ACTOR", source_label)
            return None
        actor = subsystem.duplicate_actor(source, source.get_world(), unreal.Vector(0.0, 0.0, 0.0))
        actor.set_actor_label(new_label)
    actor.set_actor_location(unreal.Vector(*location), False, False)
    if scale is not None:
        actor.set_actor_scale3d(unreal.Vector(*scale))
    if rotation is not None:
        actor.set_actor_rotation(unreal.Rotator(*rotation), False)
    return actor


def set_label_text(actor, text):
    try:
        actor.get_editor_property("TextRender").set_text(unreal.Text(text))
    except Exception as error:
        log("  label text failed:", text, error)


def build_room():
    floor_source = find_actor("Floor")
    log("-- geometry")
    dup_actor("Floor", "Tuto_Floor", (CX, CY, 0.0), scale=(HALF_X / 500.0, HALF_Y / 500.0, 1.0))
    dup_actor("Cube", "Tuto_Wall_N", (CX, CY + HALF_Y, 100.0), scale=(14.0, 0.7, 2.0))
    dup_actor("Cube", "Tuto_Wall_S", (CX, CY - HALF_Y, 100.0), scale=(14.0, 0.7, 2.0))
    dup_actor("Cube4", "Tuto_Wall_W", (CX - HALF_X, CY, 100.0), scale=(0.35, 20.0, 2.0))
    dup_actor("Cube4", "Tuto_Wall_E", (CX + HALF_X, CY, 100.0), scale=(0.35, 20.0, 2.0))
    dup_actor("PointLight", "Tuto_Light", (CX, CY, 900.0))
    return floor_source


def build_stations(blueprints):
    log("-- stations")
    stations = [
        ("Tuto_Arena_Target", (-1750.0, -700.0, 0.0), unreal.load_asset(BP_DUMMY)),
        ("Tuto_Arena_Chaser", (-2250.0, -700.0, 0.0), blueprints["BP_TutorialChaser"]),
        ("Tuto_Arena_Ally", (-1750.0, 700.0, 0.0), blueprints["BP_TutorialAlly"]),
        ("Tuto_Arena_Bullets", (-2550.0, 400.0, 0.0), blueprints["BP_TutorialTotemBullets"]),
        ("Tuto_Arena_Zones", (-2550.0, -400.0, 0.0), blueprints["BP_TutorialTotemZones"]),
    ]
    for label, location, blueprint in stations:
        arena = dup_actor("EntranceArena", label, location)
        if arena is None:
            continue
        arena.set_editor_property("BossClass", blueprint.generated_class())
        # No Arena tag: with no TargetPoint.BossSpawn matching, the arena spawns its dummy on itself,
        # which is what puts each station where its arena actor stands.
        arena.set_editor_property("ArenaTag", unreal.GameplayTag())
        log("  ", label, "->", blueprint.get_name())

    # Constant drain on the ally so there is always something to heal; players standing in it burn too.
    dup_actor("BP_DamageZone", "Tuto_AllyDrain", (-1750.0, 700.0, 150.0))
    # Static "do not stand here" patch, off the walking line between the pad and the dummies.
    dup_actor("BP_DamageZone", "Tuto_HazardZone", (-2150.0, 250.0, 150.0))


def build_access():
    log("-- access")
    pad_in = dup_actor("BP_TeleportToHex", "Tuto_TeleportToHub", (-1400.0, 0.0, 47.0))
    pad_out = dup_actor("BP_TeleportToHex", "Tuto_TeleportFromHub", (-580.0, -270.0, 47.0))
    for pad, text in ((pad_in, "Hub"), (pad_out, "Tutorial")):
        if pad is None:
            continue
        pad.set_editor_property("TeleportTag", tag(TELEPORT_TAG))
        pad.set_editor_property("DisplayText", text)

    dup_actor("BP_Triangle_ChangeClassTrigger", "Tuto_Class_Triangle", (-1400.0, 250.0, 52.5))
    dup_actor("BP_Square_ChangeClassTrigger", "Tuto_Class_Square", (-1400.0, 450.0, 70.0))
    dup_actor("BP_Circle_ChangeClassTrigger", "Tuto_Class_Circle", (-1400.0, 650.0, 80.0))

    # First entry into the level lands here; the pad above is the way out and back.
    for index, label in enumerate(("PlayerStart", "PlayerStart2", "PlayerStart3")):
        start = find_actor(label)
        if start is None:
            log("  missing", label)
            continue
        start.set_actor_location(unreal.Vector(-1750.0, -150.0 + index * 150.0, 115.0), False, False)
        log("  moved", label)


def build_signs():
    log("-- signs")
    signs = [
        ("Tuto_Sign_Target", (-1750.0, -520.0, 0.0), "SHOOT ME"),
        ("Tuto_Sign_Chaser", (-2250.0, -520.0, 0.0), "IT MOVES"),
        ("Tuto_Sign_Ally", (-1750.0, 520.0, 0.0), "HEAL ME"),
        ("Tuto_Sign_Bullets", (-2400.0, 400.0, 0.0), "BULLETS"),
        ("Tuto_Sign_Zones", (-2400.0, -400.0, 0.0), "FIRE ZONES"),
        ("Tuto_Sign_Room", (-1750.0, 0.0, 0.0), "TUTORIAL"),
    ]
    for label, location, text in signs:
        sign = dup_actor("TextRenderActor", label, location)
        if sign is not None:
            set_label_text(sign, text)


def build_level():
    REPORT[:] = []
    try:
        blueprints = {}
        for name in ("BP_TutorialChaser", "BP_TutorialAlly", "BP_TutorialTotemBullets", "BP_TutorialTotemZones"):
            blueprints[name] = unreal.load_asset("%s/%s" % (BP_DIR, name))
        build_room()
        build_stations(blueprints)
        build_access()
        build_signs()
        unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
        log("LEVEL OK")
    except Exception as error:
        import traceback
        log("LEVEL FAILED:", traceback.format_exc())
    flush()


# ---------------------------------------------------------------- post-restart


def retag():
    """Moves the tutorial room onto its own Arena.Tutorial tag; needs the tag registered (restart)."""
    REPORT[:] = []
    try:
        tutorial = tag(TUTORIAL_TAG)
        if not tutorial.export_text().count(TUTORIAL_TAG):
            log("RETAG SKIPPED — %s does not resolve yet, restart the editor first" % TUTORIAL_TAG)
        else:
            for label in ("Tuto_TeleportToHub", "Tuto_TeleportFromHub"):
                pad = find_actor(label)
                if pad is not None:
                    pad.set_editor_property("TeleportTag", tutorial)
                    log("  retagged", label)
            unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
            log("RETAG OK")
    except Exception as error:
        import traceback
        log("RETAG FAILED:", traceback.format_exc())
    flush()
