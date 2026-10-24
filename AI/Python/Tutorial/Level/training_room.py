"""
Training Room builder for GeoTrinity (DraftMap, room tag Arena.TrainingRoom).

Builds the Training Room the players spawn into and the tutorial hands off to: its zone StateTree, the tint of each
zone pattern, and the room itself (floor, walls, the arena, the zone row and its target points, class-change pads,
teleporter pair, player starts). The six zone abilities (GA_Training_*: UPatternAbility launching a BP_ZonePattern_*)
are hand-kept and only referenced here by tag. No Blueprint per variant (AI/CodingStyle.md, "Variants are data"): the
caster is the native AHiddenCasterCharacter + the arena's BossSetup, every station BP_Dummy + its StartAdds setup, every
zone the project's DefaultZoneClass tinted by its pattern's ZoneParams.Color. The Training* Blueprints, zone Blueprints
and pulse-circle tints of the old layout are deleted once nothing references them.
Idempotent — assets and actors are keyed by path/label, so a re-run updates instead of duplicating.

The room is one AGeoDummyArena, not one per enemy: the hidden zone caster is its boss and every station
enemy is an add on its own TargetPoint.AddSpawn, so the whole room spawns and resets as a single unit.

The zones are ability-driven, not placed: one hidden caster holds all six zone abilities and fires them at once, each
onto the AGeoTargetPoint carrying its own TargetPoint.Training.* tag. A zone pattern telegraphs its circle, then
either leaves an AGeoEffectZone behind (lingering zones) or bursts its effects on the spot (the two one-shot zones).
Every cadence but the telegraph lives in the StateTree: FireAll (all six at once, and it waits for all six) ->
Rest(delay) -> FireAll.

The room sits outside the level's NavMeshBoundsVolume: nothing in it paths, the moving dummy chases
in a straight line. Widen that volume first if a MoveTo/firing-point patrol is ever wanted here.

Layout is written for the game camera, which looks straight down with +X up and +Y right on screen,
so the rows below read top to bottom: title, zone labels, zones, station labels, stations, entrance.
The editor's own top view renders that 180 degrees around, which is why signs look mirrored there.

Reference: AI/MCP/MCP_Blueprint.md, AI/MCP/MCP_StateTree.md, AI/MCP/MCP_NewEnemyAbility.md.
Nothing a script prints comes back through MCP, so the run report is written to REPORT_PATH.

Stages (call from execute_script, DraftMap open):
    build_assets()        # zone StateTree, zone pattern tints
    build_level()         # room geometry, stations, the zone row and its points, signs, access
    delete_old_assets()   # in a later run: the old layout's Blueprints and tints nothing references any more
"""
import sys

import unreal

sys.path.insert(0, "C:/GeoTrinity/AI/Python/Tutorial/Level")
import tutorial_waves  # noqa: E402  delete_unreferenced

REPORT_PATH = r"C:/GeoTrinity/AI/Output/training_room_report.txt"

# Names the room twice over: the teleporter pads that lead here, and the arena the zone caster belongs to,
# which is what makes its abilities resolve against this room's AGeoTargetPoints and no other arena's.
TRAINING_TAG = "Arena.TrainingRoom"

ST_SRC = "/Game/AI/ST_EnemyBehaviour"
ST_DIR = "/Game/AI/Training"
DUMMY_SHOOTER_TREE = "/Game/AI/Dummy/ST_Dummy_Shooter"
BP_DIR = "/Game/Characters/Enemies/Training"
BP_DUMMY = BP_DIR + "/BP_Dummy"
CASTER_CLASS = "/Script/GeoTrinity.HiddenCasterCharacter"
PATTERN_DIR = "/Game/AbilitySystem/Abilities/Enemy/Training"
SHOOTER_ABILITY_TAG = "Ability.Spell.ProjectileToAllPlayers"

TARGET_POINT_BP = "/Game/Actors/GeoTargetPoint"
BOSS_SPAWN_TAG = "TargetPoint.BossSpawn"
ADD_SPAWN_TAG = "TargetPoint.AddSpawn"

# The old layout: one Blueprint or material per variant.
OLD_ASSETS = [BP_DIR + "/BP_TrainingChaser", BP_DIR + "/BP_TrainingAlly", BP_DIR + "/BP_TrainingShooter",
              BP_DIR + "/BP_TrainingZoneCaster", "/Game/Actors/Training/BP_Zone_HealOverTime",
              "/Game/Actors/Training/BP_Zone_DamageOverTime", "/Game/Actors/Training/BP_Zone_DamageReduction",
              "/Game/Actors/Training/BP_Zone_Lethal", "/Game/Art/VFX/AOE/MI_PulseCircle_Lethal",
              "/Game/Art/VFX/AOE/MI_PulseCircle_DamageReduction"]

# Zone cadence. ZONE_LIFE + ZONE_BLINK is how long a lingering zone stays up (set on the BP_ZonePattern_*); REST is
# the StateTree delay between two casts, sized so the next telegraph starts as the previous zone finishes blinking out.
ZONE_LIFE = 5.0
ZONE_BLINK = 1.0
REST = ZONE_LIFE + ZONE_BLINK

# Room: 2600 x 3800, walled, reachable only through the teleporter pair.
CX, CY = -6000.0, 0.0
HALF_X, HALF_Y = 1300.0, 1900.0

# Screen rows, top (highest X) to bottom.
ROW_TITLE = -4850.0
ROW_ZONE_SIGN = -5100.0
ROW_ZONE = -5500.0
ROW_STATION_SIGN = -6000.0
ROW_STATION = -6350.0
ROW_ENTRY_SIGN = -6800.0
ROW_ENTRY = -7050.0

# FColor is stored BGRA, so its positional constructor is not (r, g, b) — always name the channels.
GREY = unreal.Color(r=199, g=199, b=199, a=255)
RED = unreal.Color(r=230, g=60, b=60, a=255)
GREEN = unreal.Color(r=60, g=220, b=60, a=255)
BLUE = unreal.Color(r=80, g=130, b=255, a=255)
ORANGE = unreal.Color(r=255, g=120, b=0, a=255)

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


def point_tag(key):
    """Purpose tag of the AGeoTargetPoint this zone lands on. Editor-authored in Config/Tags/GeoGameplayTags.ini:
    no code names one of these, only the ability asset aiming at it."""
    return "TargetPoint.Training." + key


# key, Y, ability tag, sign lines, sign color
def zone_specs():
    return [
        ("HealOverTime", -1500.0, "Ability.Spell.TrainingHealOverTime",
         "HEAL OVER TIME\n+15 HP / sec\nwhile inside", GREEN),
        ("BurstHeal", -900.0, "Ability.Spell.TrainingBurstHeal",
         "HEAL BURST\n+25 HP\nwhen it lands", GREEN),
        ("DamageReduction", -300.0, "Ability.Spell.TrainingDamageReduction",
         "DAMAGE REDUCTION\n-50% damage taken\nwhile inside", BLUE),
        ("DamageOverTime", 300.0, "Ability.Spell.TrainingDamageOverTime",
         "DAMAGE OVER TIME\n-15 HP / sec\nwhile inside", RED),
        ("BurstDamage", 900.0, "Ability.Spell.TrainingBurstDamage",
         "DAMAGE BURST\n-25 HP\nwhen it lands", RED),
        ("Lethal", 1500.0, "Ability.Spell.TrainingLethal",
         "LETHAL\ninstant death\nwhatever your HP", ORANGE),
    ]


# Actors from earlier shapes of this room, under labels no current key rebuilds — a placed zone whose
# key was renamed keeps its old label forever otherwise, sitting under the sign that replaced it.
LEGACY_LABELS = ("Tuto_Arena_Zones", "Tuto_HazardZone",
                 "Tuto_Zone_HealOnce", "Tuto_Sign_Zone_HealOnce",
                 "Tuto_Zone_DamageOnce", "Tuto_Sign_Zone_DamageOnce",
                 "Tuto_Arena_Chaser", "Tuto_Sign_Chaser",
                 "Tuto_Arena_Ally", "Tuto_Arena_Bullets", "Tuto_Arena_Target")


# lingering zone key -> its EGeoColor, written on the pattern's ZoneParams.Color (the zone and its telegraph)
ZONE_COLORS = {"HealOverTime": "Heal", "DamageOverTime": "Damage", "DamageReduction": "DamageReduction", "Lethal": "LethalDamage"}


# ---------------------------------------------------------------- assets


def dup_asset(src, dst):
    """Returns (asset, created)."""
    if unreal.EditorAssetLibrary.does_asset_exist(dst):
        return unreal.load_asset(dst), False
    return unreal.EditorAssetLibrary.duplicate_asset(src, dst), True


def build_zone_patterns():
    """Each lingering zone's pattern spawns the project's DefaultZoneClass in its own colour: no zone Blueprint."""
    for key, color in ZONE_COLORS.items():
        blueprint = unreal.load_asset("%s/BP_ZonePattern_%s" % (PATTERN_DIR, key))
        cdo = unreal.get_default_object(blueprint.generated_class())
        params = cdo.get_editor_property("ZoneParams").copy()
        params.import_text("(Color=(Color=%s))" % color)
        cdo.set_editor_property("ZoneParams", params)
        cdo.set_editor_property("ZoneClass", None)
        unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
        unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False)
        log("ZonePattern", key, "->", color)


def build_zone_tree():
    """A trim of the enemy base tree for the zone caster. The bullet totem runs the tutorial's shared shooter tree
    (tutorial_assets.py builds it).

    Every builder call compiles the asset, so a transition must be cleared *before* the state it points
    at is removed — a dangling transition fails that compile and raises a blocking ensure.
    """
    builder = unreal.GeoStateTreeBuilderUtil.get_default_object()

    # Zone caster: Dormant -> Wait -> FireAll -> Rest -> FireAll, forever. FireAll carries one fire task per zone
    # ability and a state starts all its tasks at once, so the six go off together; TasksCompletion ALL is what
    # makes the state wait for the last of them instead of ending on the first, so one Rest spaces every cycle.
    tree, created = dup_asset(ST_SRC, ST_DIR + "/ST_Training_Zones")
    if created:
        builder.clear_transitions(tree, "FireAll")
        builder.remove_state(tree, "Behaviour")
        ability_tags = [spec[2] for spec in zone_specs()]
        builder.replace_fire_ability_tag_in_state(tree, "FireAll", ability_tags[0])
        for ability_tag in ability_tags[1:]:
            builder.add_fire_ability_task_to_state(tree, "FireAll", ability_tag)
        builder.set_tasks_completion(tree, "FireAll", unreal.StateTreeTaskCompletionType.ALL)
        builder.add_state(tree, "Rest", "InitFight", -1)
        builder.add_task_to_state(tree, "Rest", "StateTreeDelayTask")
        builder.set_task_property(tree, "Rest", "StateTreeDelayTask", "Duration", str(REST))
        builder.add_transition(tree, "FireAll", "Rest",
                               unreal.StateTreeTransitionTrigger.ON_STATE_COMPLETED)
        builder.add_transition(tree, "Rest", "FireAll",
                               unreal.StateTreeTransitionTrigger.ON_STATE_COMPLETED)
    log("StateTree zones", "created" if created else "kept")


def build_assets():
    REPORT[:] = []
    try:
        build_zone_tree()
        build_zone_patterns()
        log("ASSETS OK")
    except Exception:
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


def destroy_actor(label):
    actor = find_actor(label)
    if actor is not None:
        unreal.get_editor_subsystem(unreal.EditorActorSubsystem).destroy_actor(actor)
        log("  removed", label)


def sign(label, location, text, size=55.0, color=GREY):
    """Flat floor text. The (90, 180, 0) rotation comes with the duplicated source and is what makes
    the block read upright under the game camera; lines stack downwards from the actor."""
    actor = dup_actor("TextRenderActor", label, location)
    if actor is None:
        return
    component = actor.get_editor_property("TextRender")
    component.set_text(unreal.Text(text))
    component.set_world_size(size)
    component.set_text_render_color(color)


def build_room():
    log("-- geometry")
    dup_actor("Floor", "Tuto_Floor", (CX, CY, 0.0), scale=(HALF_X / 500.0, HALF_Y / 500.0, 1.0))
    dup_actor("Cube", "Tuto_Wall_N", (CX, CY + HALF_Y, 100.0), scale=(HALF_X / 50.0, 0.7, 2.0))
    dup_actor("Cube", "Tuto_Wall_S", (CX, CY - HALF_Y, 100.0), scale=(HALF_X / 50.0, 0.7, 2.0))
    dup_actor("Cube4", "Tuto_Wall_W", (CX - HALF_X, CY, 100.0), scale=(0.35, HALF_Y / 50.0, 2.0))
    dup_actor("Cube4", "Tuto_Wall_E", (CX + HALF_X, CY, 100.0), scale=(0.35, HALF_Y / 50.0, 2.0))
    for label, offset in (("Tuto_Light", 0.0), ("Tuto_Light_W", -1250.0), ("Tuto_Light_E", 1250.0)):
        dup_actor("PointLight", label, (CX, CY + offset, 900.0))


def station_setup(team=unreal.Team.ENEMY, state_tree=None, ability_tags=()):
    """A training dummy: BP_Dummy back at full health at zero, varied by team, tree and abilities."""
    setup = unreal.GeoEnemySetup()
    setup.set_editor_property("Team", team)
    setup.set_editor_property("bResetToFullLife", True)
    setup.set_editor_property("AggroRadius", 100.0)
    if state_tree is not None:
        setup.set_editor_property("StateTree", unreal.load_asset(state_tree))
    setup.set_editor_property("AbilityTags", [tag(t) for t in ability_tags])
    return setup


def build_stations():
    """Three dummies to shoot, heal and dodge, spawned as the room arena's adds. Returns their StartAdds in the
    order their TargetPoint.AddSpawn points are created: ResetAdds walks StartAdds and the points GetTargetPoints
    returns in step, so the two orders have to agree. The keys are listed alphabetically, which is that creation
    order, so the pairing also holds for any name-ordered lookup."""
    log("-- stations")
    stations = [
        ("Ally", 400.0, station_setup(team=unreal.Team.PLAYER), "ALLY ON FIRE\nheal it (its zone burns you too)"),
        ("Bullets", 1200.0, station_setup(state_tree=DUMMY_SHOOTER_TREE, ability_tags=[SHOOTER_ABILITY_TAG]),
         "BULLET TOTEM\ndodge the shots"),
        ("Target", -400.0, station_setup(), "TRAINING DUMMY\nshoot it"),
    ]
    dummy_class = unreal.load_asset(BP_DUMMY).generated_class()
    start_adds = []
    for key, offset, setup, text in stations:
        target_point("Tuto_AddSpawn_" + key, (ROW_STATION, offset, 0.0), ADD_SPAWN_TAG)
        sign("Tuto_Sign_" + key, (ROW_STATION_SIGN, offset, 0.0), text)
        start_add = unreal.GeoArenaAdd()
        start_add.set_editor_property("Class", dummy_class)
        start_add.set_editor_property("Setup", setup)
        start_adds.append(start_add)
        log("  ", key, "->", setup.export_text())

    # Constant drain on the ally so there is always something to heal.
    dup_actor("BP_DamageZone", "Tuto_Drain", (ROW_STATION, 400.0, 0.0))
    return start_adds


def target_point(label, location, purpose_tag):
    """An AGeoTargetPoint of this room: its purpose plus this room's arena tag, which is both halves of what
    GeoLib::GetTargetPoints matches on."""
    subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actor = find_actor(label)
    if actor is None:
        actor = subsystem.spawn_actor_from_object(unreal.load_asset(TARGET_POINT_BP), unreal.Vector(*location))
        actor.set_actor_label(label)
    actor.set_actor_location(unreal.Vector(*location), False, False)
    tags = unreal.GameplayTagContainer()
    tags.import_text('(GameplayTags=((TagName="%s"),(TagName="%s")))' % (purpose_tag, TRAINING_TAG))
    actor.set_editor_property("GameplayTags", tags)
    return actor


def build_arena(start_adds):
    """The one GeoDummyArena the whole room hangs off. Its boss is the hidden zone caster, spawned on the room's
    TargetPoint.BossSpawn, which is what makes the caster's abilities resolve against this room's points and no
    other arena's; the station enemies are its adds. IsBoss() is false on this class, so walking up to any of them
    starts no match — and one arena means one ResetBoss brings the entire room back."""
    arena = dup_actor("EntranceArena", "Tuto_ZoneArena", (ROW_ZONE, 0.0, 0.0))
    if arena is None:
        return
    caster = unreal.GeoEnemySetup()
    caster.set_editor_property("StateTree", unreal.load_asset(ST_DIR + "/ST_Training_Zones"))
    caster.set_editor_property("AbilityTags", [tag(spec[2]) for spec in zone_specs()])
    arena.set_editor_property("BossClass", unreal.load_class(None, CASTER_CLASS))
    arena.set_editor_property("BossSetup", caster)
    arena.set_editor_property("ArenaTag", tag(TRAINING_TAG))
    arena.set_editor_property("StartAdds", start_adds)
    log("  arena adds ->", len(start_adds))


def build_zones(start_adds):
    """One hidden caster in the middle of the row, dropping every zone at once on its own tagged point."""
    log("-- zone caster")
    for label in LEGACY_LABELS:
        destroy_actor(label)
    sign("Tuto_Sign_Zones", (ROW_TITLE, 0.0, 0.0),
         "EFFECT ZONES\nthey all land together on a loop, step in to see what each one does", size=90.0)
    target_point("Tuto_ZoneSpawn", (ROW_ZONE, 0.0, 0.0), BOSS_SPAWN_TAG)
    build_arena(start_adds)
    for key, offset, _, text, color in zone_specs():
        target_point("Tuto_ZonePoint_" + key, (ROW_ZONE, offset, 0.0), point_tag(key))
        sign("Tuto_Sign_Zone_" + key, (ROW_ZONE_SIGN, offset, 0.0), text, size=45.0, color=color)
        log("  ", key)


def build_access():
    log("-- access")
    pad_in = dup_actor("BP_TeleportToHex", "Tuto_TeleportToHub", (ROW_ENTRY, 0.0, 47.0))
    pad_out = dup_actor("BP_TeleportToHex", "Tuto_TeleportFromHub", (-580.0, -270.0, 47.0))
    for pad, text in ((pad_in, "Hub"), (pad_out, "Training")):
        if pad is None:
            continue
        pad.set_editor_property("TeleportTag", tag(TRAINING_TAG))
        pad.set_editor_property("DisplayText", text)
    sign("Tuto_Sign_Teleport", (ROW_ENTRY_SIGN, 0.0, 0.0), "BACK TO THE HUB", size=60.0)

    dup_actor("BP_Triangle_ChangeClassTrigger", "Tuto_Class_Triangle", (ROW_ENTRY, 500.0, 52.5))
    dup_actor("BP_Square_ChangeClassTrigger", "Tuto_Class_Square", (ROW_ENTRY, 800.0, 70.0))
    dup_actor("BP_Circle_ChangeClassTrigger", "Tuto_Class_Circle", (ROW_ENTRY, 1100.0, 80.0))
    sign("Tuto_Sign_Class", (ROW_ENTRY_SIGN, 800.0, 0.0), "CHANGE CLASS", size=60.0)

    sign("Tuto_Sign_Room", (ROW_ENTRY_SIGN, -1300.0, 0.0), "TRAINING ROOM", size=130.0)

    # First entry into the level lands here; the pad above is the way out and back.
    for index, label in enumerate(("PlayerStart", "PlayerStart2", "PlayerStart3")):
        start = find_actor(label)
        if start is None:
            log("  missing", label)
            continue
        start.set_actor_location(unreal.Vector(ROW_ENTRY, -500.0 - index * 200.0, 115.0), False, False)
        log("  moved", label)


def build_level():
    REPORT[:] = []
    try:
        build_room()
        build_zones(build_stations())
        build_access()
        unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
        log("LEVEL OK")
    except Exception:
        import traceback
        log("LEVEL FAILED:", traceback.format_exc())
    flush()


def delete_old_assets():
    """In its own run after build_level saved the room, so the asset registry no longer sees the old references."""
    REPORT[:] = []
    tutorial_waves.delete_unreferenced(OLD_ASSETS, log)
    flush()
