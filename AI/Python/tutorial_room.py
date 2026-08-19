"""
Tutorial room builder for GeoTrinity (DraftMap).

Builds the training room the players spawn into: its AI StateTrees, its dummy and totem Blueprints,
the zone ability/deployable Blueprints, the zone material instances, and the room itself (floor, walls,
dummy arenas, the zone row and its target points, class-change pads, teleporter pair, player starts).
Idempotent — assets and actors are keyed by path/label, so a re-run updates instead of duplicating.

The zones are ability-driven, not placed: one hidden caster holds all six UGeoZoneAbility Blueprints and
fires them at once, each onto the AGeoTargetPoint carrying its own TargetPoint.Tutorial.* tag. An ability
telegraphs its circle for its FireDelay, then either leaves an AGeoEffectZone behind (lingering zones) or
bursts its effects on the spot (the two one-shot zones). Every cadence but the telegraph lives in the
StateTree: FireAll (all six at once, and it waits for all six) -> Rest(delay) -> FireAll.

The room sits outside the level's NavMeshBoundsVolume: nothing in it paths, the moving dummy chases
in a straight line. Widen that volume first if a MoveTo/firing-point patrol is ever wanted here.

Layout is written for the game camera, which looks straight down with +X up and +Y right on screen,
so the rows below read top to bottom: title, zone labels, zones, station labels, stations, entrance.
The editor's own top view renders that 180 degrees around, which is why signs look mirrored there.

Reference: AI/MCP/MCP_Blueprint.md, AI/MCP/MCP_StateTree.md, AI/MCP/MCP_NewEnemyAbility.md.
Nothing a script prints comes back through MCP, so the run report is written to REPORT_PATH.

Stages (call from execute_script):
    build_assets()   # StateTrees, Blueprints (zones, abilities, caster), material instances
    build_level()    # room geometry, stations, the zone row and its points, signs, access
"""
import unreal

REPORT_PATH = (r"C:/Users/Felou/AppData/Local/Temp/claude/C--GeoTrinity"
               r"/bf7fea61-cff7-43d5-afe5-31dfbde7211f/scratchpad/tutorial_report.txt")

# Names the room twice over: the teleporter pads that lead here, and the arena the zone caster belongs to,
# which is what makes its abilities resolve against this room's AGeoTargetPoints and no other arena's.
TUTORIAL_TAG = "Arena.Tutorial"
AGGRO_EVENT = "AI.Boss.AggroEvent"

ST_SRC = "/Game/AI/ST_EnemyBehaviour"
ST_DIR = "/Game/AI/Tutorial"
BP_DIR = "/Game/Characters/Enemies/Tutorial"
BP_DUMMY = "/Game/Characters/Enemies/BP_Dummy"

GA_DIR = "/Game/AbilitySystem/Abilities/Enemy/Tutorial"
GA_PARENT = "/Script/GeoTrinity.GeoZoneAbility"
ZONE_BP_DIR = "/Game/Actors/Tutorial"
ZONE_SRC_DAMAGE = "/Game/Actors/BP_DamageZone"
ZONE_SRC_HEAL = "/Game/Actors/BP_HealingZone"

TARGET_POINT_BP = "/Game/Actors/GeoTargetPoint"
BOSS_SPAWN_TAG = "TargetPoint.BossSpawn"

MI_DIR = "/Game/Art/VFX/AOE"
MI_SRC = MI_DIR + "/MI_PulseCircle"
MI_LETHAL = MI_DIR + "/MI_PulseCircle_Lethal"
MI_REDUCTION = MI_DIR + "/MI_PulseCircle_DamageReduction"

CUE_INDICATOR = "GameplayCue.Generic.ZoneIndicator"
CUE_EXPLOSION = "GameplayCue.Generic.Explosion"

# Zone cadence. TELEGRAPH is the ability's own FireDelay; ZONE_LIFE + ZONE_BLINK is how long the
# deployable stays up; REST is the StateTree delay between two casts, sized so the next telegraph
# starts as the previous zone finishes blinking out.
TELEGRAPH = 1.0
ZONE_LIFE = 5.0
ZONE_BLINK = 1.0
REST = ZONE_LIFE + ZONE_BLINK
ZONE_RADIUS = 250.0

# Only players are hostile to an Enemy-team caster, so this reaches them and leaves the dummies alone.
HOSTILE = 4

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

ZONE_Z = 150.0

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


def cue(cue_tag, color):
    """FGeoCueParam built from its own export form, so the EGeoColor slot is named the way C++ spells it."""
    value = unreal.GeoCueParam()
    value.import_text('(CueTag=(TagName="%s"),Color=%s,SoundTag=(TagName=""))' % (cue_tag, color))
    return value


def set_struct(owner, prop, values):
    """Read-modify-write: works for any struct the reflection system exposes, named or not."""
    struct = owner.get_editor_property(prop)
    for key, value in values.items():
        struct.set_editor_property(key, value)
    owner.set_editor_property(prop, struct)


def actors():
    return unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()


def find_actor(label):
    for actor in actors():
        if actor.get_actor_label() == label:
            return actor
    return None


# ---------------------------------------------------------------- effect entries

GE_REDUCTION = "/Game/AbilitySystem/GameplayEffects/Buffs/GE_DamageReductionBuff.GE_DamageReductionBuff_C"
REDUCTION_ICON = "/Game/Art/Textures/Gameplay/Status/MAT_ShieldStatus.MAT_ShieldStatus"

# Long enough to outlast any visit; the zone removes the handle on exit anyway, and a duration of 0
# would leave GE_DamageReductionBuff's SetByCaller duration unset and expire it on the spot.
REDUCTION_DURATION = 600.0


def effect(text):
    value = unreal.InstancedStruct()
    value.import_text(text)
    return value


def flat_effect(struct, field, amount):
    """One heal/damage entry. Inside a zone AGeoEffectZone::Tick multiplies it by DeltaSeconds, so the
    value reads as a per-second rate there and as a flat one-shot hit from a burst ability. Either way
    the per-second flag must stay off — set it too and the magnitude is scaled by delta twice."""
    return effect('/Script/GeoTrinity.%s(%s=(Value=%f),bLimitGameplayCue=True)' % (struct, field, amount))


def gameplay_effect(effect_class, data_tag, magnitude, duration=0.0, icon=None):
    """Applied once on entry and removed on exit."""
    text = ('/Script/GeoTrinity.GameplayEffectData('
            'GameplayEffect="/Script/Engine.BlueprintGeneratedClass\'%s\'",'
            'DataTag=(TagName="%s"),Magnitude=(Value=%f),Duration=(Value=%f)'
            % (effect_class, data_tag, magnitude, duration))
    if icon:
        text += ',Icon="/Script/Engine.MaterialInstanceConstant\'%s\'"' % icon
    return effect(text + ')')


def point_tag(key):
    """Purpose tag of the AGeoTargetPoint this zone lands on. Editor-authored in Config/Tags/GeoGameplayTags.ini:
    no code names one of these, only the ability asset aiming at it."""
    return "TargetPoint.Tutorial." + key


# key, Y, ability tag, zone Blueprint key (None = burst), effect entry, EGeoColor slot, sign lines, sign color
def zone_specs():
    return [
        ("HealOverTime", -1500.0, "Ability.Spell.TutorialHealOverTime", "HealOverTime",
         flat_effect("HealEffectData", "HealAmount", 15.0), "Heal",
         "HEAL OVER TIME\n+15 HP / sec\nwhile inside", GREEN),
        ("BurstHeal", -900.0, "Ability.Spell.TutorialBurstHeal", None,
         flat_effect("HealEffectData", "HealAmount", 25.0), "Heal",
         "HEAL BURST\n+25 HP\nwhen it lands", GREEN),
        ("DamageReduction", -300.0, "Ability.Spell.TutorialDamageReduction", "DamageReduction",
         gameplay_effect(GE_REDUCTION, "Status.Buff.DamageReduction", .5, REDUCTION_DURATION, REDUCTION_ICON),
         "DamageReduction", "DAMAGE REDUCTION\n-50% damage taken\nwhile inside", BLUE),
        ("DamageOverTime", 300.0, "Ability.Spell.TutorialDamageOverTime", "DamageOverTime",
         flat_effect("DamageEffectData", "DamageAmount", 15.0), "Damage",
         "DAMAGE OVER TIME\n-15 HP / sec\nwhile inside", RED),
        ("BurstDamage", 900.0, "Ability.Spell.TutorialBurstDamage", None,
         flat_effect("DamageEffectData", "DamageAmount", 25.0), "Damage",
         "DAMAGE BURST\n-25 HP\nwhen it lands", RED),
        ("Lethal", 1500.0, "Ability.Spell.TutorialLethal", "Lethal",
         effect('/Script/GeoTrinity.LethalEffectData()'), "LethalDamage",
         "LETHAL\ninstant death\nwhatever your HP", ORANGE),
    ]


# Actors from earlier shapes of this room, under labels no current key rebuilds — a placed zone whose
# key was renamed keeps its old label forever otherwise, sitting under the sign that replaced it.
LEGACY_LABELS = ("Tuto_Arena_Zones", "Tuto_HazardZone",
                 "Tuto_Zone_HealOnce", "Tuto_Sign_Zone_HealOnce",
                 "Tuto_Zone_DamageOnce", "Tuto_Sign_Zone_DamageOnce")


# zone Blueprint key -> (source Blueprint, material override or None)
def zone_bp_specs():
    return {
        "HealOverTime": (ZONE_SRC_HEAL, None),
        "DamageOverTime": (ZONE_SRC_DAMAGE, None),
        "DamageReduction": (ZONE_SRC_DAMAGE, MI_REDUCTION),
        "Lethal": (ZONE_SRC_DAMAGE, MI_LETHAL),
    }


# ---------------------------------------------------------------- assets


def dup_asset(src, dst):
    """Returns (asset, created)."""
    if unreal.EditorAssetLibrary.does_asset_exist(dst):
        return unreal.load_asset(dst), False
    return unreal.EditorAssetLibrary.duplicate_asset(src, dst), True


def make_bp(name, folder, parent_class):
    """Returns (blueprint, created)."""
    path = "%s/%s" % (folder, name)
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.load_asset(path), False
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("ParentClass", parent_class)
    blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, unreal.Blueprint, factory)
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    return blueprint, True


def commit_bp(blueprint):
    """Writing to a Blueprint's CDO only reaches spawned actors once the Blueprint is recompiled — without
    this a totem spawns with none of the properties set here (no StateTree, still damageable)."""
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    unreal.EditorAssetLibrary.save_loaded_asset(blueprint)


def subobject(blueprint, name):
    """Component template of a Blueprint by component name, or None."""
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    for handle in subsystem.k2_gather_subobject_data_for_blueprint(blueprint):
        data = subsystem.k2_find_subobject_data_from_handle(handle)
        component = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(data)
        if component is not None and component.get_name().startswith(name):
            return component
    return None


def build_zone_blueprints():
    """One AGeoEffectZone Blueprint per lingering zone: same deployable, different tint and nothing else.
    Duplicated off the hand-placed zones so the pulse-circle plane and its material slot come along."""
    made = {}
    for key, (source, material) in zone_bp_specs().items():
        name = "BP_Zone_" + key
        blueprint, created = dup_asset(source, "%s/%s" % (ZONE_BP_DIR, name))
        cdo = unreal.get_default_object(blueprint.generated_class())
        cdo.set_editor_property("AttitudeBitmask", HOSTILE)
        # The spawning ability owns the effects now; the placed-zone Details fields would only confuse.
        cdo.set_editor_property("EffectDataArray", [])
        if material:
            mesh = subobject(blueprint, "Zone")
            if mesh is not None:
                mesh.set_editor_property("OverrideMaterials", [unreal.load_asset(material)])
        commit_bp(blueprint)
        made[key] = blueprint
        log("ZoneBP", name, "created" if created else "updated")
    return made


def build_ability_blueprints(zone_blueprints):
    """One UGeoZoneAbility Blueprint per zone, and its entry in the global ability catalog."""
    parent = unreal.load_class(None, GA_PARENT)
    catalog = unreal.load_asset("/Game/AbilitySystem/Data/DA_AbilityInfo")
    infos = catalog.get_editor_property("EnemyAbilityInfos")
    known = "".join(info.export_text() for info in infos)

    made = {}
    for key, _, ability_tag, zone_key, entry, color, _, _ in zone_specs():
        name = "GA_Tutorial_" + key
        blueprint, created = make_bp(name, GA_DIR, parent)
        cdo = unreal.get_default_object(blueprint.generated_class())

        tags = unreal.GameplayTagContainer()
        tags.import_text('(GameplayTags=((TagName="%s")))' % ability_tag)
        cdo.set_editor_property("AbilityTags", tags)

        cdo.set_editor_property("bUseGeneralChargeTimeForFireDelay", False)
        cdo.set_editor_property("FireDelay", TELEGRAPH)
        cdo.set_editor_property("EffectDataInstances", [entry])
        cdo.set_editor_property("TargetPointTag", tag(point_tag(key)))
        cdo.set_editor_property("BurstAttitude", HOSTILE)
        cdo.set_editor_property("TelegraphCue", cue(CUE_INDICATOR, color))
        set_struct(cdo, "ZoneParams", {"Size": ZONE_RADIUS})

        if zone_key:
            cdo.set_editor_property("ZoneClass", zone_blueprints[zone_key].generated_class())
            set_struct(cdo, "ZoneParams", {"LifeDrainMaxDuration": ZONE_LIFE, "BlinkDuration": ZONE_BLINK})
        else:
            # A burst leaves nothing behind, so the explosion is the only thing that marks where it landed.
            cdo.set_editor_property("BurstCue", cue(CUE_EXPLOSION, color))

        commit_bp(blueprint)
        made[key] = blueprint
        log("Ability", name, "created" if created else "updated")

        if '(TagName="%s")' % ability_tag not in known:
            info = unreal.GameplayAbilityInfo()
            info.import_text('(AbilityClass="/Script/Engine.BlueprintGeneratedClass\'%s\'",'
                             'AbilityTag=(TagName="%s"),AbilityDisplayName="%s",'
                             'Description="Tutorial zone demonstration.")'
                             % (blueprint.generated_class().get_path_name(), ability_tag, key))
            infos.append(info)
            log("  registered in DA_AbilityInfo")

    catalog.set_editor_property("EnemyAbilityInfos", infos)
    unreal.EditorLoadingAndSavingUtils.save_packages([catalog.get_outermost()], False)
    return made


def build_state_trees():
    """Trims of the star boss's own tree — same tasks, none of its movement or wave states.

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

    # Moving dummy: chases whoever is nearest, straight line, no navmesh and no firing points needed.
    tree, created = dup_asset(ST_SRC, ST_DIR + "/ST_Tutorial_Chase")
    if created:
        builder.clear_transitions(tree, "Dormant")
        builder.clear_transitions(tree, "FireAll")
        builder.remove_state(tree, "InitFight")
        builder.remove_state(tree, "Behaviour")
        builder.add_state(tree, "Chase", "Root", -1)
        builder.add_task_to_state(tree, "Chase", "STTask_ChaseTarget")
        builder.add_transition(tree, "Dormant", "Chase", unreal.StateTreeTransitionTrigger.ON_EVENT, AGGRO_EVENT)
    trees["chase"] = tree

    # Zone caster: Dormant -> Wait -> FireAll -> Rest -> FireAll, forever. FireAll carries one fire task per zone
    # ability and a state starts all its tasks at once, so the six go off together; TasksCompletion ALL is what
    # makes the state wait for the last of them instead of ending on the first, so one Rest spaces every cycle.
    tree, created = dup_asset(ST_SRC, ST_DIR + "/ST_Tutorial_Zones")
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
    trees["zones"] = tree

    for key, tree in trees.items():
        log("StateTree", key, "->", tree.get_name())
    return trees


def build_blueprints(trees):
    dummy_class = unreal.load_asset(BP_DUMMY).generated_class()
    made = {}

    specs = [
        ("BP_TutorialChaser", {"StateTree": trees["chase"]}, None),
        ("BP_TutorialAlly", {"TeamId": unreal.Team.PLAYER}, None),
        ("BP_TutorialTotemBullets", {"StateTree": trees["bullets"]}, ["Ability.Spell.ProjectileToAllPlayers"]),
    ]
    # The caster is nothing but what fires the six zone abilities: it never moves and never takes a hit, so it is
    # hidden and undamageable rather than a target the room invites you to shoot. Its capsule still overlaps
    # rather than blocks (the GeoCapsule profile), so nobody bumps into what they cannot see. Its aggro radius
    # is what wakes the whole row at once: walk up to the zones and every one of them starts.
    specs.append(("BP_TutorialZoneCaster",
                  {"StateTree": trees["zones"], "bCanBeDamaged": False, "bHidden": True},
                  [spec[2] for spec in zone_specs()]))

    for name, props, ability_tags in specs:
        blueprint, created = make_bp(name, BP_DIR, dummy_class)
        cdo = unreal.get_default_object(blueprint.generated_class())
        for prop, value in props.items():
            cdo.set_editor_property(prop, value)
        if ability_tags:
            asc = cdo.get_editor_property("AbilitySystemComponent")
            asc.set_editor_property("StartupAbilityTags", [tag(t) for t in ability_tags])
        commit_bp(blueprint)
        made[name] = blueprint
        log("Blueprint", name, "created" if created else "updated")
    return made


def build_materials():
    """Two tints of the shared pulse circle, parented to MI_PulseCircle so its ring and pulse tuning
    stays in one place. Colors are the EGeoColor palette values for LethalDamage and DamageReduction."""
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    library = unreal.MaterialEditingLibrary
    parent = unreal.load_asset(MI_SRC)

    for path, color in ((MI_LETHAL, unreal.LinearColor(1.0, .262451, 0.0, 1.0)),
                        (MI_REDUCTION, unreal.LinearColor(0.0, 0.0, 1.0, 1.0))):
        if unreal.EditorAssetLibrary.does_asset_exist(path):
            log("Material", path, "exists")
            continue
        name = path.rsplit("/", 1)[1]
        instance = tools.create_asset(name, MI_DIR, unreal.MaterialInstanceConstant,
                                      unreal.MaterialInstanceConstantFactoryNew())
        library.set_material_instance_parent(instance, parent)
        library.set_material_instance_vector_parameter_value(instance, "InsideColor", color)
        library.set_material_instance_vector_parameter_value(instance, "OutlineColor", color)
        unreal.EditorAssetLibrary.save_asset(path)
        log("Material", path, "created")


def build_assets():
    REPORT[:] = []
    try:
        build_materials()
        zone_blueprints = build_zone_blueprints()
        build_ability_blueprints(zone_blueprints)
        trees = build_state_trees()
        build_blueprints(trees)
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


def station(label, location, blueprint, text, sign_location, arena_tag=None):
    """A GeoDummyArena spawning one enemy. Untagged, no TargetPoint.BossSpawn matches and it spawns its dummy
    on its own actor, which is what puts a station where it stands; an arena_tag names the encounter instead,
    so the enemy spawns on that tag's BossSpawn point and its abilities can find the room's other points.
    IsBoss() is false on this arena class, so walking up to one starts no match."""
    arena = dup_actor("EntranceArena", label, location)
    if arena is None:
        return
    arena.set_editor_property("BossClass", blueprint.generated_class())
    arena.set_editor_property("ArenaTag", tag(arena_tag) if arena_tag else unreal.GameplayTag())
    if text:
        sign(label.replace("Arena", "Sign"), sign_location, text)


def build_room():
    log("-- geometry")
    dup_actor("Floor", "Tuto_Floor", (CX, CY, 0.0), scale=(HALF_X / 500.0, HALF_Y / 500.0, 1.0))
    dup_actor("Cube", "Tuto_Wall_N", (CX, CY + HALF_Y, 100.0), scale=(HALF_X / 50.0, 0.7, 2.0))
    dup_actor("Cube", "Tuto_Wall_S", (CX, CY - HALF_Y, 100.0), scale=(HALF_X / 50.0, 0.7, 2.0))
    dup_actor("Cube4", "Tuto_Wall_W", (CX - HALF_X, CY, 100.0), scale=(0.35, HALF_Y / 50.0, 2.0))
    dup_actor("Cube4", "Tuto_Wall_E", (CX + HALF_X, CY, 100.0), scale=(0.35, HALF_Y / 50.0, 2.0))
    for label, offset in (("Tuto_Light", 0.0), ("Tuto_Light_W", -1250.0), ("Tuto_Light_E", 1250.0)):
        dup_actor("PointLight", label, (CX, CY + offset, 900.0))


def build_stations(blueprints):
    """Four dummies to shoot, heal, dodge and run from."""
    log("-- stations")
    stations = [
        ("Tuto_Arena_Chaser", -1200.0, blueprints["BP_TutorialChaser"], "CHASER\nit runs straight at you"),
        ("Tuto_Arena_Target", -400.0, unreal.load_asset(BP_DUMMY), "TRAINING DUMMY\nshoot it"),
        ("Tuto_Arena_Ally", 400.0, blueprints["BP_TutorialAlly"], "ALLY ON FIRE\nheal it (its zone burns you too)"),
        ("Tuto_Arena_Bullets", 1200.0, blueprints["BP_TutorialTotemBullets"], "BULLET TOTEM\ndodge the shots"),
    ]
    for label, offset, blueprint, text in stations:
        station(label, (ROW_STATION, offset, 0.0), blueprint, text, (ROW_STATION_SIGN, offset, 0.0))
        log("  ", label, "->", blueprint.get_name())

    # Constant drain on the ally so there is always something to heal.
    dup_actor("BP_DamageZone", "Tuto_AllyDrain", (ROW_STATION, 400.0, ZONE_Z))


def target_point(label, location, purpose_tag):
    """An AGeoTargetPoint the zone caster aims at: its purpose plus this room's arena tag, which is both halves
    of what GeoLib::GetTargetPoints matches on."""
    subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actor = find_actor(label)
    if actor is None:
        actor = subsystem.spawn_actor_from_object(unreal.load_asset(TARGET_POINT_BP), unreal.Vector(*location))
        actor.set_actor_label(label)
    actor.set_actor_location(unreal.Vector(*location), False, False)
    tags = unreal.GameplayTagContainer()
    tags.import_text('(GameplayTags=((TagName="%s"),(TagName="%s")))' % (purpose_tag, TUTORIAL_TAG))
    actor.set_editor_property("GameplayTags", tags)
    return actor


def prune_zone_totems():
    """The six per-zone totems the one caster replaces: their arenas first, then the assets behind them."""
    for key, *_ in zone_specs():
        destroy_actor("Tuto_ZoneArena_" + key)
        for asset in ("%s/BP_TutorialTotem_%s" % (BP_DIR, key), "%s/ST_Tutorial_%s" % (ST_DIR, key)):
            if unreal.EditorAssetLibrary.does_asset_exist(asset):
                unreal.EditorAssetLibrary.delete_asset(asset)
                log("  deleted", asset)


def build_zones():
    """One hidden caster in the middle of the row, dropping every zone at once on its own tagged point."""
    log("-- zone caster")
    for label in LEGACY_LABELS:
        destroy_actor(label)
    prune_zone_totems()
    sign("Tuto_Sign_Zones", (ROW_TITLE, 0.0, 0.0),
         "EFFECT ZONES\nthey all land together on a loop, step in to see what each one does", size=90.0)
    target_point("Tuto_ZoneSpawn", (ROW_ZONE, 0.0, 0.0), BOSS_SPAWN_TAG)
    station("Tuto_ZoneArena", (ROW_ZONE, 0.0, 0.0), unreal.load_asset(BP_DIR + "/BP_TutorialZoneCaster"),
            None, None, arena_tag=TUTORIAL_TAG)
    for key, offset, _, _, _, _, text, color in zone_specs():
        target_point("Tuto_ZonePoint_" + key, (ROW_ZONE, offset, 0.0), point_tag(key))
        sign("Tuto_Sign_Zone_" + key, (ROW_ZONE_SIGN, offset, 0.0), text, size=45.0, color=color)
        log("  ", key)


def build_access():
    log("-- access")
    pad_in = dup_actor("BP_TeleportToHex", "Tuto_TeleportToHub", (ROW_ENTRY, 0.0, 47.0))
    pad_out = dup_actor("BP_TeleportToHex", "Tuto_TeleportFromHub", (-580.0, -270.0, 47.0))
    for pad, text in ((pad_in, "Hub"), (pad_out, "Tutorial")):
        if pad is None:
            continue
        pad.set_editor_property("TeleportTag", tag(TUTORIAL_TAG))
        pad.set_editor_property("DisplayText", text)
    sign("Tuto_Sign_Teleport", (ROW_ENTRY_SIGN, 0.0, 0.0), "BACK TO THE HUB", size=60.0)

    dup_actor("BP_Triangle_ChangeClassTrigger", "Tuto_Class_Triangle", (ROW_ENTRY, 500.0, 52.5))
    dup_actor("BP_Square_ChangeClassTrigger", "Tuto_Class_Square", (ROW_ENTRY, 800.0, 70.0))
    dup_actor("BP_Circle_ChangeClassTrigger", "Tuto_Class_Circle", (ROW_ENTRY, 1100.0, 80.0))
    sign("Tuto_Sign_Class", (ROW_ENTRY_SIGN, 800.0, 0.0), "CHANGE CLASS", size=60.0)

    sign("Tuto_Sign_Room", (ROW_ENTRY_SIGN, -1300.0, 0.0), "TUTORIAL", size=130.0)

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
        blueprints = {}
        for name in ("BP_TutorialChaser", "BP_TutorialAlly", "BP_TutorialTotemBullets"):
            blueprints[name] = unreal.load_asset("%s/%s" % (BP_DIR, name))
        build_room()
        build_stations(blueprints)
        build_zones()
        build_access()
        unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
        log("LEVEL OK")
    except Exception:
        import traceback
        log("LEVEL FAILED:", traceback.format_exc())
    flush()
