"""
Tutorial map shell for GeoTrinity: BP_TutorialGameMode, the game instance's two map refs, the Training Room's
"TrainingRoom" PlayerStart, and /Game/Maps/Tutorial (the rooms' floors, walls, camera and arena volumes, entrance points).
Encounters per room are authored in tutorial_encounters.py. Idempotent: assets and actors are keyed by path / label.
The map starts as a copy of DraftMap pruned to the entrance hall's kit, so the camera volume, arena volume, floor and
wall actors are the game's own. Report: AI/Output/tutorial_map_report.txt.
Usage: headless (AI/Commands.md), editor closed or via MCP execute_script.
"""
import traceback

import unreal

REPORT_PATH = "C:/GeoTrinity/AI/Output/tutorial_map_report.txt"
DRAFT_MAP = "/Game/Maps/DraftMap"
TUTORIAL_MAP = "/Game/Maps/Tutorial"
GAME_MODE_PARENT = "/Game/Game/BP_GeoGameMode"
GAME_MODE = "/Game/Game/BP_TutorialGameMode"
GAME_INSTANCE = "/Game/Game/BP_GeoGameInstance"
TARGET_POINT_BP = "/Game/Actors/GeoTargetPoint"

TRAINING_START_TAG = "TrainingRoom"
TRAINING_START_LABEL = "PlayerStart_TrainingRoom"
TRAINING_START_LOCATION = (-6000.0, 0.0, 115.0)

# Kept from the DraftMap copy: the entrance hall's kit, used as the sources the rooms are duplicated from.
KEEP_LABELS = {"Floor", "Cube", "Cube4", "BP_GeoCam", "DirectionalLight", "GeoArenaVolumeEntrance", "PlayerStart"}
OLD_HALL = ["Floor", "Cube", "Cube4", "GeoArenaVolumeEntrance", "GeoCameraVolume"]
CAMERA_VOLUME_LOCATION = (0.0, 0.0, 0.0)

# Rooms along +X (screen up). Name -> half size in X, Y. Every room is a floor, two side walls and a camera / arena volume.
ROOM_HALF_X = 700.0
ROOM_HALF_Y = 1100.0
WALL_HALF = 35.0
WALL_SCALE_Y = 0.7
WALL_SCALE_Z = 2.0
WALL_Z = 100.0
SOUTH_END_YAW = 90.0  # travel direction round a room's centre (AI/ArenaWalls.md): E side 0, W side 180, N end -90, S end 90
NORTH_END_YAW = -90.0
CAMERA_VOLUME_HALF_Z = 1000.0
ENTRANCE_INSET = 200.0
START_INSET = 250.0
ROOM_GAP = 1500.0
VOID_WIDTH = 300.0  # the Dash room's hole: narrower than the 500 cm dash, wider than a walk-off step
NEAR_FLOOR_WIDTH = 600.0
PLATFORM_VOLUME_Z = 57.7  # the landing volume's centre height: a standing capsule is inside it
PLATFORM_VOLUME_HALF_Z = 40.0
WALL_HALO = "/Game/Art/VFX/Arena/MI_ArenaRailHalo"
WALL_RAIL = "/Game/Art/VFX/Arena/MI_ArenaRail"
TELEPORTER_BP = "/Game/Actors/TriggersActors/BP_Teleport"
TELEPORT_TAG = "Teleport.Tutorial"
PAD_INSET = 250.0
EXIT_PAD_Y = 800.0
START_Z = 115.0  # PlayerStart height: capsule clear of the floor
ROOMS = ["Landing", "Dash", "Shoot", "Deploy", "Special", "Dummies", "Zones", "Miniboss"]
REMOVED_ROOMS = ["Move"]

LOCKED_ABILITY_TAGS = ["Ability.Spell.Dash", "Ability.Spell.BasicBeam", "Ability.Spell.HealingZone",
                       "Ability.Spell.MoiraBeam"]

REPORT = []


def log(*args):
    REPORT.append(" ".join(str(a) for a in args))


def flush():
    with open(REPORT_PATH, "w", encoding="utf-8") as report:
        report.write("\n".join(REPORT))


def actors():
    return unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()


def find_actor(label):
    return next((a for a in actors() if a.get_actor_label() == label), None)


def tag(name):
    value = unreal.GameplayTag()
    value.import_text('(TagName="%s")' % name)
    return value


def save_asset(asset):
    unreal.BlueprintEditorLibrary.compile_blueprint(asset)
    unreal.EditorAssetLibrary.save_loaded_asset(asset)


# ---------------------------------------------------------------- assets

def build_game_mode():
    """BP_TutorialGameMode: a child of BP_GeoGameMode that refuses a second player."""
    if not unreal.EditorAssetLibrary.does_asset_exist(GAME_MODE):
        factory = unreal.BlueprintFactory()
        factory.set_editor_property("parent_class", unreal.load_asset(GAME_MODE_PARENT).generated_class())
        folder, name = GAME_MODE.rsplit("/", 1)
        unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, unreal.Blueprint, factory)
    blueprint = unreal.load_asset(GAME_MODE)
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    unreal.get_default_object(blueprint.generated_class()).set_editor_property("bSoloOnly", True)
    save_asset(blueprint)
    log("game mode", GAME_MODE)


def set_instance_maps():
    blueprint = unreal.load_asset(GAME_INSTANCE)
    default = unreal.get_default_object(blueprint.generated_class())
    default.set_editor_property("TutorialMap", unreal.load_asset(TUTORIAL_MAP))
    default.set_editor_property("TrainingRoomMap", unreal.load_asset(DRAFT_MAP))
    save_asset(blueprint)
    log("game instance maps", default.get_editor_property("TutorialMap"), default.get_editor_property("TrainingRoomMap"))


# ---------------------------------------------------------------- Training Room

def build_training_start():
    """The PlayerStart the tutorial's exit lands on, inside the Training Room's arena volume."""
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(DRAFT_MAP)
    start = find_actor(TRAINING_START_LABEL)
    if start is None:
        start = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).spawn_actor_from_class(
            unreal.PlayerStart, unreal.Vector(*TRAINING_START_LOCATION))
        start.set_actor_label(TRAINING_START_LABEL)
    start.set_actor_location(unreal.Vector(*TRAINING_START_LOCATION), False, False)
    start.set_editor_property("PlayerStartTag", unreal.Name(TRAINING_START_TAG))
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    log("training start", start.get_actor_location())


# ---------------------------------------------------------------- Tutorial map

def create_map():
    """DraftMap copied once, then pruned to the entrance hall's kit."""
    if unreal.EditorAssetLibrary.does_asset_exist(TUTORIAL_MAP):
        unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(TUTORIAL_MAP)
        return False
    unreal.EditorAssetLibrary.duplicate_asset(DRAFT_MAP, TUTORIAL_MAP)
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(TUTORIAL_MAP)
    subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    removed = 0
    for actor in actors():
        label = actor.get_actor_label()
        keep = label in KEEP_LABELS or actor.get_class().get_name() in ("WorldSettings", "GeoWorldSettings", "GeoCameraVolume")
        if actor.get_class().get_name() == "GeoCameraVolume":
            keep = (abs(actor.get_actor_location().x - CAMERA_VOLUME_LOCATION[0]) < 1.0
                    and abs(actor.get_actor_location().y - CAMERA_VOLUME_LOCATION[1]) < 1.0)
        if actor.get_class().get_name() == "PlayerStart":
            keep = label == "PlayerStart"
        if not keep:
            subsystem.destroy_actor(actor)
            removed += 1
    log("pruned", removed, "actors of the DraftMap copy")
    return True


def world_settings():
    settings = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world().get_world_settings()
    settings.set_editor_property("default_game_mode", unreal.load_asset(GAME_MODE).generated_class())
    settings.set_editor_property("StartingClassOverride", unreal.PlayerClass.CIRCLE)
    tags = unreal.GameplayTagContainer()
    tags.import_text("(GameplayTags=(%s))" % ",".join('(TagName="%s")' % t for t in LOCKED_ABILITY_TAGS))
    settings.set_editor_property("LockedAbilityTags", tags)
    log("world settings", settings.get_editor_property("StartingClassOverride"))


def dup_actor(source_label, new_label, location, rotation=None):
    """A copy of the kit actor source_label: same class, and for a static mesh actor its mesh and materials. Spawned rather
    than duplicated: the duplicate call needs a selection the headless editor does not have."""
    actor = find_actor(new_label)
    if actor is None:
        source = find_actor(source_label)
        actor = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).spawn_actor_from_class(
            source.get_class(), unreal.Vector(*location))
        actor.set_actor_label(new_label)
        if isinstance(source, unreal.StaticMeshActor):
            source_component = source.static_mesh_component
            component = actor.static_mesh_component
            component.set_static_mesh(source_component.static_mesh)
            for index, material in enumerate(source_component.get_materials()):
                component.set_material(index, material)
            actor.set_actor_scale3d(source.get_actor_scale3d())
            actor.set_actor_rotation(source.get_actor_rotation(), False)
        else:
            actor.set_actor_scale3d(source.get_actor_scale3d())
    actor.set_actor_location(unreal.Vector(*location), False, False)
    if rotation is not None:
        actor.set_actor_rotation(unreal.Rotator(*rotation), False)
    return actor


def spawn_volume(volume_class, label, location):
    """A box volume (its TriggerBox is the root, not a brush), spawned fresh."""
    actor = find_actor(label)
    if actor is None:
        actor = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).spawn_actor_from_class(
            volume_class, unreal.Vector(*location))
        actor.set_actor_label(label)
    actor.set_actor_location(unreal.Vector(*location), False, False)
    return actor


def fit(actor, extent_x, extent_y, extent_z=None):
    """Scales actor so its bounds reach the given half sizes (a rotation of 90 degrees swaps X and Y: pass the world's)."""
    _, extent = actor.get_actor_bounds(False)
    scale = actor.get_actor_scale3d()
    actor.set_actor_scale3d(unreal.Vector(scale.x * extent_x / extent.x, scale.y * extent_y / extent.y,
                                          scale.z if extent_z is None else scale.z * extent_z / extent.z))


def size_camera_volume(camera, half_x, half_y):
    """Both boxes of a camera volume (the TriggerBox and the BoundsBox) at the room size, actor scale 1."""
    camera.set_actor_scale3d(unreal.Vector(1.0, 1.0, 1.0))
    extent = unreal.Vector(half_x, half_y, CAMERA_VOLUME_HALF_Z)
    for box in ("TriggerBox", "BoundsBox"):
        camera.get_editor_property(box).set_box_extent(extent, True)


def size_box(box, half_x, half_y, half_z):
    """A trigger box at its element's size: actor scale 1, the extent set on its BoxComponent (never a scaled actor: the
    default extent is not the 100 a scale would assume). Also used by tutorial_encounters.py."""
    box.set_actor_scale3d(unreal.Vector(1.0, 1.0, 1.0))
    box.get_components_by_class(unreal.BoxComponent)[0].set_box_extent(unreal.Vector(half_x, half_y, half_z), True)


def room_center_x(index):
    return index * (2.0 * ROOM_HALF_X + ROOM_GAP)


def point(label, location, purpose, arena_tag):
    actor = find_actor(label)
    if actor is None:
        actor = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).spawn_actor_from_class(
            unreal.load_asset(TARGET_POINT_BP).generated_class(), unreal.Vector(*location))
        actor.set_actor_label(label)
    actor.set_actor_location(unreal.Vector(*location), False, False)
    tags = unreal.GameplayTagContainer()
    tags.import_text('(GameplayTags=((TagName="%s"),(TagName="%s")))' % (purpose, arena_tag))
    actor.set_editor_property("GameplayTags", tags)
    return actor


def place_wall(label, source_label, center_xy, yaw, rail_length):
    """An arena wall per AI/ArenaWalls.md: fixed scale Y / Z, lengthened by the rail's end inset on both ends, local +X
    (the rail's travel direction) along yaw."""
    wall = dup_actor(source_label, label, (center_xy[0], center_xy[1], WALL_Z))
    wall.static_mesh_component.set_material(0, unreal.load_asset(WALL_HALO))
    wall.static_mesh_component.set_editor_property("overlay_material", unreal.load_asset(WALL_RAIL))
    wall.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw), False)
    wall.set_actor_scale3d(unreal.Vector((rail_length + 2.0 * WALL_HALF) / 100.0, WALL_SCALE_Y, WALL_SCALE_Z))
    return wall


def wall_x(label, x, yaw):
    """A wall across the full width at x (the room ends), meeting the side walls' centerlines."""
    return place_wall(label, wall_source(), (x, 0.0), yaw, 2.0 * (ROOM_HALF_Y + WALL_HALF))


def build_floor(label, source_label, left_x, right_x):
    floor = dup_actor(source_label, label, ((left_x + right_x) / 2.0, 0.0, 0.0))
    fit(floor, (right_x - left_x) / 2.0, ROOM_HALF_Y)


def build_dash_void(cx, prefix):
    """The Dash floor as two floors with a hole between; the far one is the platform to reach."""
    near_left = cx - ROOM_HALF_X
    far_left = near_left + NEAR_FLOOR_WIDTH + VOID_WIDTH
    build_floor(prefix + "_Floor", "Floor", near_left, near_left + NEAR_FLOOR_WIDTH)
    build_floor(prefix + "_FarFloor", prefix + "_Floor", far_left, cx + ROOM_HALF_X)
    platform_half_x = (cx + ROOM_HALF_X - far_left) / 2.0
    platform = spawn_volume(unreal.TriggerBox, prefix + "_PlatformVolume",
                            ((far_left + cx + ROOM_HALF_X) / 2.0, 0.0, PLATFORM_VOLUME_Z))
    size_box(platform, platform_half_x, ROOM_HALF_Y, PLATFORM_VOLUME_HALF_Z)  # the whole far floor: no edge to dash past


def build_room(index, name):
    """One room: floor, two side walls, arena volume, camera volume, entrance point."""
    cx = room_center_x(index)
    prefix = "Tut_" + name
    arena_tag = "Arena.Tutorial." + name
    if name == "Dash":
        build_dash_void(cx, prefix)
    else:
        build_floor(prefix + "_Floor", "Floor", cx - ROOM_HALF_X, cx + ROOM_HALF_X)
    for side, sign, yaw in (("E", 1.0, 0.0), ("W", -1.0, 180.0)):
        place_wall("%s_Wall_%s" % (prefix, side), wall_source(), (cx, sign * (ROOM_HALF_Y + WALL_HALF)), yaw,
                   2.0 * (ROOM_HALF_X + WALL_HALF))
    volume = spawn_volume(unreal.GeoArenaVolume, prefix + "_ArenaVolume", (cx, 0.0, 0.0))
    fit(volume, ROOM_HALF_X, ROOM_HALF_Y, 200.0)
    volume.set_editor_property("ArenaTag", tag(arena_tag))
    camera = spawn_volume(unreal.GeoCameraVolume, prefix + "_CameraVolume", (cx, 0.0, 0.0))
    size_camera_volume(camera, ROOM_HALF_X, ROOM_HALF_Y)
    point(prefix + "_Entrance", (cx - ROOM_HALF_X + ENTRANCE_INSET, 0.0, 0.0), "TargetPoint.Entrance", arena_tag)


def wall_source():
    """The kit wall to copy: the entrance hall's Cube4 on a fresh map, else the start wall."""
    return "Cube4" if find_actor("Cube4") is not None else "Tut_Wall_Start"


def pad(label, name, location):
    """A tutorial teleporter, closed. Its actor name, not its label, is the sort key that pairs the pads."""
    actor = find_actor(label)
    if actor is None:
        actor = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).spawn_actor_from_class(
            unreal.load_asset(TELEPORTER_BP).generated_class(), unreal.Vector(*location))
        actor.set_actor_label(label)
    if actor.get_name() != name:
        actor.rename(name)
    actor.set_actor_location(unreal.Vector(*location), False, False)
    actor.set_editor_property("TeleportTag", tag(TELEPORT_TAG))
    actor.set_editor_property("bOpen", False)
    actor.set_editor_property("DisplayText", "")  # the Blueprint default reads "Hex"
    return actor


def build_pads(index, name):
    """Exit pad near the room's +X end, off to the side (a task ends in the middle: a pad opening under the player would teleport them unasked) (the last room's is the final exit, tutorial_encounters.py), entry pad at its entrance (not the first)."""
    cx = room_center_x(index)
    prefix = "Tut_Pad_%02d" % index
    label = "Tut_%s_Pad" % name
    pad(label + "Exit", prefix + "_Exit", (cx + ROOM_HALF_X - PAD_INSET, EXIT_PAD_Y, 0.0))
    if index > 0:
        entry_x = cx - ROOM_HALF_X + ENTRANCE_INSET
        pad(label + "Entry", prefix + "_Entry", (entry_x, 0.0, 0.0))
        build_jump_start(name, entry_x)


def build_jump_start(name, entry_x):
    """The room's jump start on its entry pad, tagged with the room name: `Geo.Tutorial.Jump <name>` lands here (Flow.md)."""
    label = "Tut_%s_JumpStart" % name
    start = find_actor(label)
    if start is None:
        start = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).spawn_actor_from_class(
            unreal.PlayerStart, unreal.Vector(entry_x, 0.0, START_Z))
        start.set_actor_label(label)
    start.set_actor_location(unreal.Vector(entry_x, 0.0, START_Z), False, False)
    start.set_editor_property("PlayerStartTag", unreal.Name(name))


def build_rooms():
    subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for actor in actors():
        if any(actor.get_actor_label().startswith("Tut_%s_" % room) for room in REMOVED_ROOMS):
            actor.rename("Removed_" + actor.get_actor_label())  # frees the name: a destroyed actor keeps it until GC
            subsystem.destroy_actor(actor)
    wall_x("Tut_Wall_Start", room_center_x(0) - ROOM_HALF_X - WALL_HALF, SOUTH_END_YAW)
    wall_x("Tut_Wall_End", room_center_x(len(ROOMS) - 1) + ROOM_HALF_X + WALL_HALF, NORTH_END_YAW)
    for index, name in enumerate(ROOMS):
        build_room(index, name)
        build_pads(index, name)
        # Fixed end walls: a room is sealed, the pad is the only way on.
        if index > 0:
            wall_x("Tut_%s_Wall_S" % name, room_center_x(index) - ROOM_HALF_X - WALL_HALF, SOUTH_END_YAW)
        if index < len(ROOMS) - 1:
            wall_x("Tut_%s_Wall_N" % name, room_center_x(index) + ROOM_HALF_X + WALL_HALF, NORTH_END_YAW)
    for actor in actors():
        label = actor.get_actor_label()
        if label.startswith("Tut_") and (label.endswith("_Door") or label.endswith("_Barrier")):
            subsystem.destroy_actor(actor)
    # The copied entrance hall's kit goes: the rooms replace it.
    for label in OLD_HALL:
        actor = find_actor(label)
        if actor is not None:
            subsystem.destroy_actor(actor)
    start = find_actor("PlayerStart")
    start.set_actor_location(unreal.Vector(room_center_x(0) - ROOM_HALF_X + START_INSET, 0.0, START_Z), False, False)
    log("rooms", len(ROOMS))


def run():
    REPORT[:] = []
    try:
        build_game_mode()
        build_training_start()
        fresh = create_map()
        world_settings()
        log("map created" if fresh else "map existed")
        build_rooms()
        unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
        set_instance_maps()
        log("OK")
    except Exception:
        log("FAILED", traceback.format_exc())
    flush()


if __name__ == "__main__":
    run()
