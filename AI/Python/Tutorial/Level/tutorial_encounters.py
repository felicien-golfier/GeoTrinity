"""
Tutorial rooms 0-7 for GeoTrinity (Steps.md steps 0-10; the Landing room holds steps 0-1, the Zones room steps 7-9,
the Miniboss room step 10): the placed content of each room (void line and far platform, spawn points, the zone room's
zone spots, wave start and caster arena, the miniboss arena and its points) and one AGeoEncounter per room, chained: a
room's last step opens its exit pad; the next room's encounter starts when the player lands in it. Cat lines are the
Tutorial Board text; the player keys come through {Key:...}. Gems and Forge steps are built later (AI/Pending/Tutorial.md).
Run after tutorial_map.py, tutorial_assets.py, tutorial_waves.py and tutorial_miniboss.py. Idempotent: actors keyed by label, the encounters' steps rewritten.
Report: AI/Output/tutorial_encounters_report.txt. Usage: headless (AI/Commands.md), editor closed.
"""
import math
import traceback

import unreal

REPORT_PATH = "C:/GeoTrinity/AI/Output/tutorial_encounters_report.txt"
TUTORIAL_MAP = "/Game/Maps/Tutorial"
TARGET_POINT_BP = "/Game/Actors/GeoTargetPoint"
DUMMY_BP = "/Game/Characters/Enemies/Training/BP_Dummy"
CURVE_TABLE = "/Game/AbilitySystem/Data/CurveTables/CT_Tutorial"
SHOOTER_TREE = "/Game/AI/Dummy/ST_Dummy_Shooter"
CHASER_TREE = "/Game/AI/Dummy/ST_Dummy_Chaser"
CAT_BP = "/Game/Actors/Cat/BP_Cat"
CAT_ANIM = "/Game/Characters/Anim/Cat"
CAT_REASSEMBLE = CAT_ANIM + "/AM_Cat_Reassemble"
CAT_PORTAL_JUMP = CAT_ANIM + "/AM_Cat_PortalJump"
CAT_SPEED = 150.0  # placeholder, tuned against the Cat_Walk stride
CAT_PORTAL_DISTANCE = 110.0  # rig_cat.py PORTAL_X: the jump clip's portal opens this far ahead, on the pad
SHOOT_ABILITY_TAG = "Ability.Spell.ProjectileToAllPlayers"

ROOMS = ["Landing", "Dash", "Shoot", "Deploy", "Special", "Dummies", "Zones", "Miniboss"]
SHARED_PAD_TAG = "Teleport.Tutorial"
FINAL_EXIT_TAG = "Teleport.TutorialExit"
ROOM_HALF_X = 700.0
ROOM_HALF_Y = 1100.0
ROOM_GAP = 1500.0  # same as tutorial_map.py
CHARACTER_Z = 50.0
LANDING_START_X = -450.0  # tutorial_map.py: PlayerStart, START_INSET from the room's near end
CAT_AHEAD = 300.0  # from the Landing start, facing it
CAT_PATH = [(100.0, -350.0), (100.0, 350.0)]  # a short line across the Landing room, walked while a line plays

IDLE_DELAY = 10.0
QUEST_RESPAWN = 5.0
QUEST_TIME_LIMIT = 1800.0
QUEST_GROUPS = 3
QUEST_GROUP_SIZE = 3
QUEST_RAY_DEGREES = 70.0  # between two neighbouring rays (>= 60)
QUEST_ENTRY_INSET = 150.0  # the rays' origin, from the room's near end
QUEST_GROUP_START = 350.0  # first dummy of a ray, from the origin
QUEST_GROUP_SPACING = 350.0
QUEST_CUT_SECONDS = 45.0
DASH_KEY = "{Key:InputTag.Dash}"
DASH_ABILITY_TAG = "Ability.Spell.Dash"
HURT_ALLY_START_HEALTH = 0.1
FIRST_DUMMY_DEATH_SECONDS = 2.0  # the death montage plays out before the Cat and the quest start (placeholder)
SPECIAL_ABILITY_TAG = "Ability.Spell.MoiraBeam"
SPECIAL_KEY = "{Key:InputTag.Special}"
PAGE_LINE = "An ability page is about to pop up. Read it. Or don't, I'm a cat, not a cop."

CASTER_BP = "/Game/Characters/Enemies/Tutorial/BP_HiddenCaster_TutorialWaves"
ZONE_X = 150.0  # from the room's centre
ZONE_SPOTS = {"Heal": -550.0, "Shield": 0.0, "Damage": 550.0}  # Y of each zone
ZONE_RADIUS = 250.0
ZONE_BOX_SCALE = ZONE_RADIUS * 0.7 / 100.0  # the square inscribed in the zone's circle: inside the box is inside the zone
WAVE_START_INSET = 650.0  # tutorial_waves.py: the front starts here, from the room's centre, and crosses it along +X
CASTER_SPAWN = (600.0, -900.0)  # from the room's centre, off the zones
MINIBOSS_BP = "/Game/Characters/Enemies/Tutorial/BP_TutorialMiniboss"
MINIBOSS_POINTS = [(150.0, -650.0), (350.0, 0.0), (150.0, 650.0)]  # from the room's centre; the boss spawns on the middle one, within its aggro radius of the entry pad
MINIBOSS_ALLY_TAG = "TutorialMinibossAlly"  # BP_TutorialSummonedDummy's AllySetup.Tag
MINIBOSS_ALLY_LOW_RATIO = 0.3
MINIBOSS_CHIPS = ["Power", "Guard", "Vigor", "Mend", "Swift", "Precision", "Edge"]  # Data/gems.csv, tier Chip
HOSTILE_ATTITUDE = 4  # ETeamAttitudeBitflag: acts on the other team
DEATH_TIME = 3.0  # AGeoGameState::DeathTime
LETHAL_WAVE_TAG = "Ability.Spell.TutorialWaveLethal"
SAFE_WAVE_TAGS = ["Ability.Spell.TutorialWaveSafe1", "Ability.Spell.TutorialWaveSafe2", "Ability.Spell.TutorialWaveSafe3"]
HEAL_EFFECT = ('/Script/GeoTrinity.HealEffectData(bSuppressHealProvided=False,Amount=(Value=15.000000),bIsPerSecond=True,'
               'bLimitGameplayCue=True)')
SHIELD_EFFECT = ('/Script/GeoTrinity.GameplayEffectData(GameplayEffect="/Script/Engine.BlueprintGeneratedClass\'/Game/'
                 'AbilitySystem/GameplayEffects/Buffs/GE_DamageReductionBuff.GE_DamageReductionBuff_C\'",'
                 'DataTag=(TagName="Status.Buff.DamageReduction"),Magnitude=(Value=0.500000),bInfiniteDuration=True,'
                 'Icon="/Script/GeoTrinity.GeoIcon\'/Game/HUD/Icons/DA_Icon_Status_DamageReduction.'
                 'DA_Icon_Status_DamageReduction\'")')
DAMAGE_EFFECT = '/Script/GeoTrinity.DamageEffectData(Amount=(Value=15.000000),bIsPerSecond=True,bLimitGameplayCue=True)'
ZONE_EFFECTS = {"Heal": HEAL_EFFECT, "Shield": SHIELD_EFFECT, "Damage": DAMAGE_EFFECT}

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


def actors():
    return unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()


def find_actor(label):
    return next((a for a in actors() if a.get_actor_label() == label), None)


def room_x(name):
    return ROOMS.index(name) * (2.0 * ROOM_HALF_X + ROOM_GAP)


# ---------------------------------------------------------------- placed content

def place(label, actor_class, location, rotation=None):
    actor = find_actor(label)
    if actor is None:
        actor = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).spawn_actor_from_class(
            actor_class, unreal.Vector(*location))
        actor.set_actor_label(label)
    actor.set_actor_location(unreal.Vector(*location), False, False)
    if rotation is not None:
        actor.set_actor_rotation(unreal.Rotator(*rotation), False)
    return actor


def point(label, location, purpose, room):
    actor = place(label, unreal.load_asset(TARGET_POINT_BP).generated_class(), location)
    tags = unreal.GameplayTagContainer()
    tags.import_text('(GameplayTags=((TagName="%s"),(TagName="Arena.Tutorial.%s")))' % (purpose, room))
    actor.set_editor_property("GameplayTags", tags)
    return actor


def remove_actor(label):
    """A character an earlier version placed: they are spawned by the steps now."""
    actor = find_actor(label)
    if actor is not None:
        unreal.get_editor_subsystem(unreal.EditorActorSubsystem).destroy_actor(actor)


def build_landing_room():
    """The Cat, facing the player's start, hidden until its reassemble, and the path it walks while it talks."""
    cat = place("Tut_Landing_Cat", unreal.load_asset(CAT_BP).generated_class(),
                (LANDING_START_X + CAT_AHEAD, 0.0, 0.0), (0.0, 0.0, 180.0))
    cat.set_editor_property("bHidden", True)
    for index, (x, y) in enumerate(CAT_PATH):
        place("Tut_Landing_CatPath_%d" % index, unreal.TargetPoint, (x, y, 0.0))


def build_shoot_room():
    cx = room_x("Shoot")
    for stale in range(QUEST_GROUPS * QUEST_GROUP_SIZE, 10):
        remove_actor("Tut_Shoot_Spawn_%d" % stale)

    origin_x = cx - ROOM_HALF_X + QUEST_ENTRY_INSET
    for group in range(QUEST_GROUPS):
        angle = math.radians((group - (QUEST_GROUPS - 1) / 2.0) * QUEST_RAY_DEGREES)
        for rank in range(QUEST_GROUP_SIZE):
            distance = QUEST_GROUP_START + rank * QUEST_GROUP_SPACING
            point("Tut_Shoot_Spawn_%d" % (group * QUEST_GROUP_SIZE + rank),
                  (origin_x + distance * math.cos(angle), distance * math.sin(angle), 0.0),
                  "TargetPoint.AddSpawn", "Shoot")


def build_shoot_first_point():
    point("Tut_Shoot_FirstDummyPoint", (room_x("Shoot"), 0.0, 0.0), "TargetPoint.Tutorial.ShootFirst", "Shoot")


def build_deploy_room():
    cx = room_x("Deploy")
    remove_actor("Tut_Deploy_Ally")
    point("Tut_Deploy_AllySpawn", (cx + 250.0, 350.0, 0.0), "TargetPoint.Tutorial.Ally", "Deploy")


def build_special_room():
    cx = room_x("Special")
    remove_actor("Tut_Special_Ally")
    remove_actor("Tut_Special_Dummy")
    point("Tut_Special_AllySpawn", (cx + 250.0, -350.0, 0.0), "TargetPoint.Tutorial.Ally", "Special")
    point("Tut_Special_Spawn_0", (cx + 250.0, 450.0, 0.0), "TargetPoint.AddSpawn", "Special")


def build_dummies_room():
    cx = room_x("Dummies")
    point("Tut_Dummies_Chaser", (cx + 300.0, -600.0, 0.0), "TargetPoint.Tutorial.Chaser", "Dummies")
    point("Tut_Dummies_Shooter", (cx + 300.0, 0.0, 0.0), "TargetPoint.Tutorial.Shooter", "Dummies")
    point("Tut_Dummies_Mover", (cx + 300.0, 600.0, 0.0), "TargetPoint.Tutorial.Mover", "Dummies")


def build_zone_points():
    cx = room_x("Zones")
    for color, y in ZONE_SPOTS.items():
        point("Tut_Zones_ZonePoint_" + color, (cx + ZONE_X, y, 0.0), "TargetPoint.Tutorial.Zone" + color, "Zones")
    point("Tut_Zones_WaveStart", (cx - WAVE_START_INSET, 0.0, 0.0), "TargetPoint.Tutorial.WaveStart", "Zones")
    point("Tut_Zones_BossSpawn", (cx + CASTER_SPAWN[0], CASTER_SPAWN[1], 0.0), "TargetPoint.BossSpawn", "Zones")


def build_zones_room():
    """The room's points, a box over each zone spot, and the arena that spawns the hidden caster."""
    cx = room_x("Zones")
    build_zone_points()
    for color, y in ZONE_SPOTS.items():
        box = place("Tut_Zones_ZoneVolume_" + color, unreal.TriggerBox, (cx + ZONE_X, y, 0.0))
        box.set_actor_scale3d(unreal.Vector(ZONE_BOX_SCALE, ZONE_BOX_SCALE, 2.0))
    arena = place("Tut_Zones_CasterArena", unreal.GeoDummyArena, (cx, 0.0, 0.0))
    arena.set_editor_property("BossClass", unreal.load_asset(CASTER_BP).generated_class())
    arena.set_editor_property("ArenaTag", tag("Arena.Tutorial.Zones"))


def chip_shard_cost():
    """What the Forge asks for one Chip: the catalog's, so the kill hands over exactly that."""
    catalog = unreal.GameDataSettings.get_default_object().get_editor_property("GemCatalog")
    return catalog.get_editor_property("CraftCosts")[unreal.GeoGemTier.CHIP]


def build_miniboss_points():
    cx = room_x("Miniboss")
    for index, (x, y) in enumerate(MINIBOSS_POINTS):
        point("Tut_Miniboss_MinibossPoint_%d" % index, (cx + x, y, 0.0), "TargetPoint.Tutorial.MinibossPoint", "Miniboss")
    x, y = MINIBOSS_POINTS[1]
    point("Tut_Miniboss_BossSpawn", (cx + x, y, 0.0), "TargetPoint.BossSpawn", "Miniboss")
    point("Tut_Miniboss_WaveStart", (cx - WAVE_START_INSET, 0.0, 0.0), "TargetPoint.Tutorial.WaveStart", "Miniboss")
    point("Tut_Miniboss_FightLocation", (cx, 0.0, 0.0), "TargetPoint.FightLocation", "Miniboss")  # AGeoArena::CommitFight teleports to it; none = ensure


def build_miniboss_room():
    """The miniboss's points and the arena that spawns the boss and hands out the scripted loot."""
    build_miniboss_points()
    cx = room_x("Miniboss")
    arena = place("Tut_Miniboss_Arena", unreal.GeoArena, (cx, 0.0, 0.0))
    arena.set_editor_property("BossClass", unreal.load_asset(MINIBOSS_BP).generated_class())
    arena.set_editor_property("ArenaTag", tag("Arena.Tutorial.Miniboss"))
    arena.set_editor_property("BossType", unreal.GeoBossType.MINI_BOSS)
    arena.set_editor_property("bRecordsLeaderboard", False)
    loot = arena.get_editor_property("BossLoot").copy()
    loot.import_text("(bOverrideDropTable=True,DropTable=(MinCount=0,MaxCount=0),FixedGems=(%s),FixedShards=%d)"
                     % (",".join('(GemId="%s",Count=1)' % chip for chip in MINIBOSS_CHIPS), chip_shard_cost()))
    arena.set_editor_property("BossLoot", loot)


# ---------------------------------------------------------------- encounter building

class Builder:
    """Builds the steps of one encounter; every instanced object is owned by it."""

    def __init__(self, encounter):
        self.encounter = encounter

    def make(self, cls, **props):
        obj = unreal.new_object(cls, outer=self.encounter)
        for name, value in props.items():
            obj.set_editor_property(name, value)
        return obj

    # actions
    def say(self, line, wait_for_respawn=False):
        return self.make(unreal.GeoCatLineAction, Line=line, bWaitForRespawn=wait_for_respawn)

    def count(self, name, delta=1):
        return self.make(unreal.GeoAddToCounterAction, CounterName=name, Delta=delta)

    def unlock(self, ability_tag):
        """The Cat's page line, the grant, then the ability's page: the Cat speaks before anything pops up."""
        return [self.say(PAGE_LINE), self.make(unreal.GeoUnlockAbilityAction, AbilityTag=tag(ability_tag)),
                self.make(unreal.GeoShowAbilityPageAction, AbilityTag=tag(ability_tag))]

    def lock(self, locked):
        return self.make(unreal.GeoSetMovementLockedAction, bLocked=locked)

    def pad(self, pad_label, opened):
        return self.make(unreal.GeoSetTeleporterOpenAction, Teleporter=find_actor(pad_label), bOpen=opened)

    # Landing's Cat: one order each; wait=False lets the next line play while it carries it out.
    def cat_walk(self, path, stop_distance=0.0, wait=True):
        return self.make(unreal.GeoCatWalkAction, Cat=find_actor("Tut_Landing_Cat"),
                         Path=[find_actor(label) for label in path], Speed=CAT_SPEED, StopDistance=stop_distance,
                         bWaitForEnd=wait)

    def cat_roam(self, area_label):
        roam = unreal.GeoCatRoam()
        roam.set_editor_property("Area", find_actor(area_label))
        roam.set_editor_property("Speed", CAT_SPEED)
        return self.make(unreal.GeoCatRoamAction, Cat=find_actor("Tut_Landing_Cat"), Settings=roam)

    def cat_montage(self, montage, wait=True):
        return self.make(unreal.GeoCatMontageAction, Cat=find_actor("Tut_Landing_Cat"),
                         Montage=unreal.load_asset(montage), bWaitForEnd=wait)

    def hidden(self, label, hide):
        return self.make(unreal.GeoSetActorHiddenAction, Actor=find_actor(label), bHidden=hide)

    def enemy_setup(self, enemy_tag, health_row, team=unreal.Team.ENEMY, state_tree=None, ability=None,
                    start_health=1.0):
        """The variant of BP_Dummy: tutorial enemies die at zero, their health reads a CT_Tutorial row."""
        handle = unreal.CurveTableRowHandle()
        handle.set_editor_property("CurveTable", unreal.load_asset(CURVE_TABLE))
        handle.set_editor_property("RowName", health_row)
        max_health = unreal.ScalableFloat()
        max_health.set_editor_property("Value", 1.0)
        max_health.set_editor_property("Curve", handle)
        setup = unreal.GeoEnemySetup()
        setup.set_editor_property("Team", team)
        setup.set_editor_property("bResetToFullLife", False)
        setup.set_editor_property("MaxHealth", max_health)
        setup.set_editor_property("StartHealthRatio", start_health)
        setup.set_editor_property("Tag", enemy_tag)
        if state_tree is not None:
            setup.set_editor_property("StateTree", unreal.load_asset(state_tree))
        if ability is not None:
            setup.set_editor_property("AbilityTags", [tag(ability)])
        return setup

    def spawn(self, setup, amount, purpose, room, respawn=0.0, kill_on_step_exit=False):
        return self.make(unreal.GeoSpawnEnemiesAction,
                         EnemyClass=unreal.load_asset(DUMMY_BP).generated_class(), Setup=setup, Count=amount,
                         RespawnDelay=respawn, PurposeTag=tag(purpose), ArenaTag=tag("Arena.Tutorial." + room),
                         bKillOnStepExit=kill_on_step_exit)

    def card(self, operation, **props):
        return self.make(unreal.GeoQuestCardAction, Operation=operation, **props)

    def zone(self, color):
        """The colour's zone on its spot, until the step is left."""
        effect = unreal.InstancedStruct()
        effect.import_text(ZONE_EFFECTS[color])
        params = unreal.DeployableDataParams()
        params.import_text("(Size=%f,Color=(Color=%s),Attitude=%d)" % (ZONE_RADIUS, color, HOSTILE_ATTITUDE))
        return self.make(unreal.GeoSpawnZoneAction, PurposeTag=tag("TargetPoint.Tutorial.Zone" + color),
                         ArenaTag=tag("Arena.Tutorial.Zones"), EffectDataArray=[effect], Params=params,
                         Team=unreal.Team.ENEMY)

    def wave(self, ability_tag):
        """The zone room's hidden caster fires the wave; the action waits for its end."""
        return self.make(unreal.GeoActivateAbilityAction, Arena=find_actor("Tut_Zones_CasterArena"),
                         AbilityTag=tag(ability_tag), bWaitForEnd=True)

    # triggers
    def entered(self, volume_actor):
        return self.make(unreal.GeoVolumeEnteredTrigger, Volume=volume_actor)

    def died(self, watched_class, watched_tag=""):
        return self.make(unreal.GeoActorDiedTrigger, Watched=unreal.GeoActorFilter(watched_class, watched_tag))

    def dummy_died(self, watched_tag):
        return self.died(unreal.load_asset(DUMMY_BP).generated_class(), watched_tag)

    def player_died(self):
        return self.died(watched_class=unreal.PlayableCharacter.static_class())

    def timer(self, delay, looping=False):
        return self.make(unreal.GeoTimerTrigger, Delay=delay, bLooping=looping)

    def idle(self, delay=IDLE_DELAY):
        return self.make(unreal.GeoPlayerIdleTrigger, Delay=delay)

    def counter(self, name):
        return self.make(unreal.GeoCounterTrigger, CounterName=name)

    def ability_used(self, ability_tag):
        return self.make(unreal.GeoAbilityActivatedTrigger, AbilityTag=tag(ability_tag))

    def dummy_health(self, watched_tag, side, ratio):
        return self.make(unreal.GeoHealthThresholdTrigger,
                         Watched=unreal.GeoActorFilter(unreal.load_asset(DUMMY_BP).generated_class(), watched_tag),
                         Side=side, Ratio=ratio)

    # conditions
    def payload_count(self, comparison, operand):
        return self.make(unreal.GeoCompareCondition, Source=unreal.GeoCompareSource.PAYLOAD_COUNT,
                         Comparison=comparison, Operand=float(operand))

    def counter_is(self, name, comparison, operand):
        return self.make(unreal.GeoCompareCondition, Source=unreal.GeoCompareSource.COUNTER, CounterName=name,
                         Comparison=comparison, Operand=float(operand))

    # structure
    def rule(self, trigger, actions, condition=None, repeat=False, completes=False, step_after=""):
        rule = unreal.GeoEncounterRule()
        rule.set_editor_property("Trigger", trigger)
        rule.set_editor_property("Condition", condition)
        rule.set_editor_property("Actions", actions)
        rule.set_editor_property("bRepeat", repeat)
        rule.set_editor_property("bCompletesStep", completes)
        rule.set_editor_property("StepAfter", unreal.Name(step_after))
        return rule

    def step(self, name, rules, on_enter=(), on_exit=()):
        step = unreal.GeoEncounterStep()
        step.set_editor_property("Name", name)
        step.set_editor_property("OnEnter", list(on_enter))
        step.set_editor_property("Rules", list(rules))
        step.set_editor_property("OnExit", list(on_exit))
        return step

    def narrate(self, name, lines, first_delay=0.5):
        """A rule that says the lines one after the other once the step has started: the teaching never waits on them."""
        return self.rule(self.timer(first_delay), [self.say(line) for line in lines])


def encounter_actor(room):
    label = "Tut_%s_Encounter" % room
    return place(label, unreal.GeoEncounter, (room_x(room), 0.0, 0.0))


def hand_off(builder, room):
    """OnExit of a room's last step: once its last Cat line is gone, open its exit pad."""
    return [builder.pad("Tut_%s_PadExit" % room, True)]


def leave_steps(builder, room, extra_rules=()):
    """After a room: Leave = the player lands in the next room (start its encounter, close this room's exit pad), plus
    the room's own extra_rules while it waits. The next room's entry pad never opens (Flow.md, "Rooms")."""
    following = ROOMS[ROOMS.index(room) + 1]
    arrive = [builder.make(unreal.GeoStartEncounterAction, Target=encounter_actor(following)),
              builder.pad("Tut_%s_PadExit" % room, False)]
    return [builder.step("Leave", [builder.rule(builder.entered(find_actor("Tut_%s_ArenaVolume" % following)), arrive,
                                                completes=True)] + list(extra_rules))]


def exit_steps(builder, room):
    """After the last built room: Exit = the player steps on its exit pad, the tutorial ends (Flow.md, "Rooms")."""
    return [builder.step("Exit", [builder.rule(builder.entered(find_actor("Tut_%s_PadExitVolume" % room)),
                                               [builder.make(unreal.GeoLeaveTutorialAction)], completes=True)])]


GREATER_OR_EQUAL = unreal.GeoCompareOperator.GREATER_OR_EQUAL
EQUAL = unreal.GeoCompareOperator.EQUAL


WALK_LINE_INDEX = 5  # the "Hold W, A, S or D" line of landing_steps
CAT_PATH_LINE_INDEX = 2  # the line the Cat says while walking its path


def landing_steps(b):
    """Steps 0-1 in one step: movement locked while the Cat talks; after the last line the exit pad opens and the lock
    lifts (the pad is the Move target; Leave waits for it). Cat staging: AI/Tutorial/Steps.md."""
    lines = [
        "Ouch. Landed. Welcome to wherever this is: flat, bright, suspiciously empty. Very on brand.",
        "Welcome to the bowels of the game you were playing five minutes ago. You and I landed somewhere safe, by pure luck. Your two other friends most likely landed in front of a boss. They're ~~dead~~ already. I'd observe a minute of silence, but I'm busy.",
        "Right. I'm stuck in here with you, so let's get us out. Rule one: To leave, you must kill all bosses. Dead. Tot. Muertos.",
        "How do I know that? I'd love to explain. I won't. Moving on.",
        "Also, you're the Healer. Enjoy it: it's probably the only time in your life you'll play one. No choice, it's the tutorial. Nobody picks Healer on purpose.",
        "Hold W, A, S or D and your shape slides around. Congratulations, you have unlocked walking.",
        "See the round portal on the floor? Walk onto it. I'll wait. I have nine lives and no schedule.",
    ]
    actions = [b.lock(True), b.hidden("Tut_Landing_Cat", False), b.cat_montage(CAT_REASSEMBLE)]
    for index, line in enumerate(lines):
        if index == CAT_PATH_LINE_INDEX:
            actions.append(b.cat_walk(["Tut_Landing_CatPath_%d" % i for i in range(len(CAT_PATH))], wait=False))
        if index == WALK_LINE_INDEX:
            actions += [b.lock(False), b.cat_roam("Tut_Landing_ArenaVolume")]
        if index == len(lines) - 1:
            actions += [b.pad("Tut_Landing_PadExit", True), b.count("CatExit")]
        actions.append(b.say(line))
    actions.append(b.count("Done"))
    # The exit runs beside the last line (a rule's list runs alongside OnEnter); the step ends once both are done.
    cat_exit = [b.cat_walk(["Tut_Landing_PadExit"], stop_distance=CAT_PORTAL_DISTANCE),
                b.cat_montage(CAT_PORTAL_JUMP), b.hidden("Tut_Landing_Cat", True), b.count("Done")]
    return [b.step("Landing", [
        b.rule(b.counter("CatExit"), cat_exit),
        b.rule(b.counter("Done"), [], condition=b.payload_count(EQUAL, 2), completes=True),
    ], on_enter=actions)]


def landing_nag(b):
    """While Leave waits for the player to step on the opened pad (armed after the unlock)."""
    return [b.rule(b.idle(), [b.say("Still standing there? The portal is not going to walk to you.")], repeat=True)]


def dash_steps(b):
    platform = find_actor("Tut_Dash_PlatformVolume")
    if platform is None:
        raise RuntimeError("Tut_Dash_PlatformVolume missing: run tutorial_map.py first")
    useless = "Even in these fancy shapes, you stay useless humans. Useless humans with a dash. Progress."
    first_fall = [
        b.say("Oh, you fell and died? What a shame... I didn't see it coming. Truly.", True),
        b.say("Here's the trick: press **%s** just before the edge. You burst forward and cross the gap in one go. "
              "Tap it, don't pray." % DASH_KEY),
        b.say("The dash does NOT make you invincible. Bullets still hurt mid-dash. Only the falling is skipped, and that "
              "is a loophole, not a superpower."),
    ]
    second_fall = [b.say("Dead again. In the same hole. Impressive consistency. The key is **%s**. Press **%s** before "
                         "the edge. I can't make it clearer without drawing it." % (DASH_KEY, DASH_KEY), True)]
    later_falls = [
        "~~**DEAD.**~~ Again. Press **%s**. **%s!** The hole is not going to move out of respect." % (DASH_KEY, DASH_KEY),
        "That's {Counter:Falls} deaths. A hole is beating you. It doesn't even have hands. **%s**. Please. "
        "~~**DYING**~~ is not a movement option." % DASH_KEY,
        "Should I carve it on a plaque? Edge, then **%s**, then... ~~**DEAD**~~ is optional. I'd go with the first two."
        % DASH_KEY,
        "I started a tally. You are winning, if the goal was dying. If the goal was the other side: **%s**." % DASH_KEY,
    ]
    rules = [
        b.rule(b.player_died(), [b.count("Falls")], repeat=True),
        b.rule(b.player_died(), first_fall, condition=b.payload_count(EQUAL, 1), repeat=True),
        b.rule(b.player_died(), second_fall, condition=b.payload_count(EQUAL, 2), repeat=True),
    ]
    for index, line in enumerate(later_falls):
        fall = 3 + index
        operator = EQUAL if index < len(later_falls) - 1 else GREATER_OR_EQUAL
        rules.append(b.rule(b.player_died(), [b.say(line, True)], condition=b.payload_count(operator, fall), repeat=True))
    stunned = [
        b.say("... ... WHAT. You just... in one go?! I wasn't even looking! Nobody does that on the first try. Nobody. "
              "Do it again, slowly, for me. For science. For my bet."), b.say(useless),
    ]
    relief = [b.say("There. Finally. Only {Counter:Falls} deaths. In the story I'll tell, it took 'a moment'."),
              b.say(useless)]
    rules.append(b.rule(b.entered(platform), stunned, condition=b.counter_is("Falls", EQUAL, 0), completes=True))
    rules.append(b.rule(b.entered(platform), relief, condition=b.counter_is("Falls", GREATER_OR_EQUAL, 1),
                        completes=True))
    idle_lines = [
        "Take your time. I'm just here, whistling. Casually. Like a cat with no stake in this.",
        "Staring at a hole doesn't fill it. Neither does hoping. I checked, I have the time.",
        "Fun fact: your keyboard has more than four keys. Dramatic gasp. Some of them do things.",
        "I've started naming the pixels of the Void. This one is Kevin. Kevin is more motivated than you.",
    ]
    for tier, line in enumerate(idle_lines, start=1):
        operator = EQUAL if tier < len(idle_lines) else GREATER_OR_EQUAL
        rules.append(b.rule(b.idle(), [b.say(line)], condition=b.payload_count(operator, tier), repeat=True))
    arrival = [
        b.say("You reached a portal by walking. Truly the peak of the genre. Next lesson: gravity."),
    ] + b.unlock(DASH_ABILITY_TAG)
    return [b.step("Dash", rules, on_enter=arrival, on_exit=hand_off(b, "Dash"))]


def shoot_steps(b):
    spawn_tag = "TargetPoint.AddSpawn"
    dummy = b.enemy_setup("TutorialShootDummy", "DummyHealth")
    first = b.step("Shoot", [
        b.rule(b.timer(0.1), [b.spawn(dummy, 1, "TargetPoint.Tutorial.ShootFirst", "Shoot")]),
        b.rule(b.timer(0.5), [
            b.say("New room, new toy. This is a Dummy. It does not dodge, it does not fight back, it has no opinions. "
                  "Basically your favorite raid member."),
            b.say("Your basic attack is about to be yours. Hold the attack button and make the Dummy regret existing."),
        ] + b.unlock("Ability.Spell.BasicBeam")),
        b.rule(b.dummy_died("TutorialShootDummy"), [b.make(unreal.GeoDelayAction, Duration=FIRST_DUMMY_DEATH_SECONDS)],
               completes=True),
    ])

    def quip(kills, line):
        return b.rule(b.dummy_died("TutorialShootDummy"), [b.say(line)], condition=b.payload_count(EQUAL, kills),
                      repeat=True)

    stop = [b.say("... Okay, no. I can't. I'm bored and I'm only watching. This is a tutorial, not a part-time job.")]
    quest = b.step("Quest", [
        b.rule(b.dummy_died("TutorialShootDummy"),
               [b.card(unreal.GeoQuestCardOperation.COUNT)], repeat=True),
        quip(3, "3 out of 100. At this pace you finish after dinner. Possibly after next week's dinner. Don't do the "
                "math, it's rude."),
        quip(6, "Don't mind the respawn. In the industry it's called 'retention'. Somebody got promoted for inventing it."),
        quip(9, "There is a guy named Greg who has been stuck at 99 out of 100 since the beta. The last one never "
                "spawned. I think about Greg a lot."),
        b.rule(b.dummy_died("TutorialShootDummy"), stop, condition=b.payload_count(EQUAL, 12), completes=True),
        b.rule(b.timer(QUEST_CUT_SECONDS), stop, completes=True),
        b.rule(b.timer(0.1), [b.spawn(dummy, QUEST_GROUPS * QUEST_GROUP_SIZE, spawn_tag, "Shoot",
                                      respawn=QUEST_RESPAWN, kill_on_step_exit=True)]),
    ], on_enter=[
        b.card(unreal.GeoQuestCardOperation.SHOW, Title="Cull the Dummies", Max=100, TimeLimitSeconds=QUEST_TIME_LIMIT),
        b.say("Dummy destroyed. Somewhere, a wooden target is filing a complaint with HR."),
        b.say("Oh look, a quest! Nine Dummies are already here, and every one comes back after dying, so we will never run "
              "out. Other games call this 'content'."),
    ], on_exit=[
        b.card(unreal.GeoQuestCardOperation.COUNT, bShowMax=True),
        b.say("Quest abandoned. Dummies, off. Quest complete! Reward: 3 copper. Mine now. Administration fee."),
        b.say("That was the first quest of every game ever, shrunk to forty seconds. You're welcome. Next toy."),
        b.card(unreal.GeoQuestCardOperation.HIDE),
    ] + hand_off(b, "Shoot"))
    return [first, quest]


def settle_step(b, spawns):
    """First step of a room with a hurt ally: spawns it, then leaves. The ally passes through full health while it starts,
    so the step that watches its health arms only after that."""
    return b.step("Setup", [b.rule(b.timer(0.5), [], completes=True)], on_enter=spawns)


def deploy_steps(b):
    ally = b.enemy_setup("TutorialAlly", "AllyDummyHealth", team=unreal.Team.PLAYER,
                         start_health=HURT_ALLY_START_HEALTH)
    setup = settle_step(b, [b.spawn(ally, 1, "TargetPoint.Tutorial.Ally", "Deploy")])
    heal = b.step("Heal", [
        b.rule(b.timer(0.5), [
            b.say("Second toy: the deployable. You are the Healer, so yours is a Healing Zone. Hold the button to charge "
                  "the throw, release to drop it. Longer press is further. Physics, basically."),
            b.say("See the friendly Dummy over there, barely standing? Drop the Zone on it and let it heal up. It can't "
                  "ask nicely. Dummies never do."),
        ] + b.unlock("Ability.Spell.HealingZone")),
        b.rule(b.dummy_health("TutorialAlly", unreal.GeoHealthSide.ABOVE, 1.0),
               [b.say("Keep it in the Zone until it's full. Congratulations, you've healed. The most thankless job in "
                      "gaming, and you chose it. Well, I chose it for you."),
                b.say("You have 3 charges and they refill slowly, so aim like you mean it.")],
               completes=True),
    ], on_exit=hand_off(b, "Deploy"))
    return [setup, heal]


def special_steps(b):
    """Intro (lines, then the unlock) then Fight. Fight spawns both dummies itself and tallies one counter: enemy dead 1,
    ally full 1, Special used 10. 2 = both goals without the Special (sting, the step re-enters itself: leaving kills the
    survivors), 12 = done (Steps.md, step 5)."""
    ally = b.enemy_setup("TutorialAlly", "AllyDummyHealth", team=unreal.Team.PLAYER,
                         start_health=HURT_ALLY_START_HEALTH)
    enemy = b.enemy_setup("TutorialRudeDummy", "DummyHealth")
    intro = b.step("Intro", [
        b.rule(b.timer(0.5), [
            b.say("Third toy: the Special. Two Dummies are on their way: a friendly one, hurt, and an enemy one, rude. "
                  "The Special interacts with your deployable. The ability page explains how, and unlike me, it is not "
                  "sarcastic."),
            b.say("Kill the rude one and bring the hurt one back to full health. Same beam for both. The healer's natural "
                  "habitat: do everything at once and get blamed for the rest."),
            b.say("Remember: your zones have 3 charges, slow refill. Spend them wisely. Or don't, I'm a cat, I don't pay "
                  "the bills."),
        ] + b.unlock(SPECIAL_ABILITY_TAG), completes=True),
    ])
    sting = [b.say("Wow. You killed one Dummy and healed the other without pressing **%s** even once. That is the button "
                   "I just handed you. It is right there. Humbling. Again, and this time with the beam." % SPECIAL_KEY)]
    done = [b.say("Done? You hurt one and helped the other with the same beam. Mixed messages, but it works."),
            b.pad("Tut_Special_PadExit", True)]
    fight = b.step("Fight", [
        b.rule(b.timer(0.1), [b.spawn(ally, 1, "TargetPoint.Tutorial.Ally", "Special", kill_on_step_exit=True)]),
        b.rule(b.timer(0.1), [b.spawn(enemy, 1, "TargetPoint.AddSpawn", "Special", kill_on_step_exit=True)]),
        b.rule(b.dummy_health("TutorialAlly", unreal.GeoHealthSide.ABOVE, 1.0), [b.count("Progress")]),
        b.rule(b.dummy_died("TutorialRudeDummy"), [b.count("Progress")]),
        b.rule(b.ability_used(SPECIAL_ABILITY_TAG), [b.count("Progress", 10)]),
        b.rule(b.counter("Progress"), sting, condition=b.payload_count(EQUAL, 2), completes=True, step_after="Fight"),
        b.rule(b.counter("Progress"), done, condition=b.payload_count(EQUAL, 12), completes=True),
    ])
    return [intro, fight]


def dummies_steps(b):
    chaser = b.enemy_setup("TutorialChaser", "DummyHealth", state_tree=CHASER_TREE)
    shooter = b.enemy_setup("TutorialShooter", "DummyHealth", state_tree=SHOOTER_TREE, ability=SHOOT_ABILITY_TAG)
    mover = b.enemy_setup("TutorialMover", "DummyHealth")

    def spawner(setup, purpose):
        return b.rule(b.timer(0.5), [b.spawn(setup, 1, purpose, "Dummies")])

    def watch(enemy_tag):
        return b.rule(b.dummy_died(enemy_tag), [b.count("Dead")])

    fight = b.step("Fight", [
        b.narrate("Fight", [
            "Plot twist: the Dummies have been training. One chases you, one shoots, one just wanders off and acts "
            "annoying. Truly a full raid composition.",
            "Use everything you learned. Dash the bullets, drop your toy, pop your Special. I'll narrate if anyone dies. "
            "I'm told I'm very supportive.",
        ]),
        spawner(chaser, "TargetPoint.Tutorial.Chaser"),
        spawner(shooter, "TargetPoint.Tutorial.Shooter"),
        spawner(mover, "TargetPoint.Tutorial.Mover"),
        watch("TutorialChaser"), watch("TutorialShooter"), watch("TutorialMover"),
        b.rule(b.counter("Dead"),
               [b.say("All three down. You've officially beaten the cardboard. Don't let it go to your head. The floor is "
                      "about to get opinions.")], condition=b.payload_count(EQUAL, 3), completes=True),
    ], on_exit=hand_off(b, "Dummies"))
    return [fight]


def miniboss_steps(b):
    """Step 10. The fight is the arena's; the encounter only talks: an intro, one strategy tip per death (in order), the
    ally call-out at low health (three lines), the kill line. Done when the miniboss is dead, by any means."""
    deaths = [
        "Tip: those Dummies land hard, then a zone grows around each one. It does little, but it never stops, until the "
        "whole map is covered. Kill the Dummies. Revolutionary, I know. Take notes.",
        "A dead Dummy switches sides. You are the Healer: heal it, hide behind it, and it shoots the boss for you. Allies "
        "mean damage, and no growing zone. You're welcome. A thank-you note would be fine, but I'll settle for survival.",
        "Every third Dummy, a lethal wave sweeps the room sideways. Find the safe line and slip through. You remember "
        "lethal. You were there. Very memorable. For all the wrong reasons.",
    ]
    calls = [
        "'I need healing.' Reminds me of my old friend Genji. Jumped around the whole map, barking it at anyone who'd "
        "listen. Nobody ever came.",
        "Poor dummy, yawning 'I need healing' with no one around to help. If ONLY we had a healer in the team...",
        "I need healing. **I NEED HEALING.** ~~I NEED HEALING~~",
    ]
    rules = []
    for count, line in enumerate(deaths, start=1):
        rules.append(b.rule(b.player_died(), [b.say(line, True)], condition=b.payload_count(EQUAL, count), repeat=True))
    for count, line in enumerate(calls, start=1):
        rules.append(b.rule(b.dummy_health(MINIBOSS_ALLY_TAG, unreal.GeoHealthSide.BELOW, MINIBOSS_ALLY_LOW_RATIO),
                            [b.say(line)], condition=b.payload_count(EQUAL, count), repeat=True))
    rules.append(b.rule(b.died(unreal.load_asset(MINIBOSS_BP).generated_class()),
                        [b.say("It's dead. You've got it: time to come back to the real world. Your snacks are cold and "
                               "someone has to roll initiative. But first: loot.")], completes=True))
    return [b.step("Fight", rules, on_enter=[
        b.say("Meet the Miniboss. Large, angry, and on a budget: it only owns three places to stand. Good luck. I'll be "
              "quiet. Mostly.")], on_exit=hand_off(b, "Miniboss"))]


def zones_steps(b):
    """Steps 7-9. One step per colour (its zone spawns on entry, a box over it completes), the lethal wave (the player's
    death leaves), then the safe-spot waves; a death leaves SafeSpots for SafeSpotsWait, which comes back after DeathTime."""
    intro = [
        "Welcome to the Color Room. The floor will now heal, shield and hurt you. Think of it as customer service.",
        "Green heals. Blue shields. Orange hurts. The color tells you what the floor will do before it does it. "
        "Remember that. It is the whole game.",
        "Now stand in each one. Yes, including the orange. It's a small hurt. An educational hurt.",
    ]
    colors_learned = [b.say("Colors learned? Good. There is one more. I'll show you in a minute. Stretch.")]
    steps = []
    for color in ZONE_SPOTS:
        rules = [b.rule(b.entered(find_actor("Tut_Zones_ZoneVolume_" + color)),
                        colors_learned if color == "Damage" else [], completes=True)]
        if color == "Heal":
            rules.insert(0, b.narrate(color, intro))
        steps.append(b.step("Zones_" + color, rules, on_enter=[b.zone(color)]))
    steps.append(b.step("LethalWave", [
        b.rule(b.timer(0.5), [b.say("Big wave is coming! Avoid it! Quick, quick, you can do it!")]),
        b.rule(b.player_died(), [
            b.say("Oh, it hurts? Upsy, I forgot to leave you room to survive...", True),
            b.say("But get used to it. Dying is part of the job, you will die a lot. At least statistically."),
            b.say("Okay, that was a bit unfair. Yes, on purpose. I'm sorry. A little."),
            b.say("Look, I added safe spots in the wave. For you. Out of the goodness of my heart. Don't tell the other "
                  "cats."),
        ], completes=True),
    ], on_enter=[b.wave(LETHAL_WAVE_TAG)]))
    steps.append(b.step("SafeSpotsWait", [b.rule(b.timer(DEATH_TIME), [], completes=True)]))
    # The exit pad opens from the finishing rule, not OnExit: a death leaves this step too.
    steps.append(b.step("SafeSpots", [
        b.rule(b.player_died(), [b.say("Get into a spot before the wave lands. Then another wave. The spots move, so you "
                                       "move. It's like dancing, but you die.", True)],
               completes=True, step_after="SafeSpotsWait"),
        b.rule(b.counter("WavesDone"),
               [b.say("Survived? Don't get cocky: this wave was nicely labeled. The real ones are not.")]
               + hand_off(b, "Zones"), condition=b.payload_count(EQUAL, 1), completes=True),
    ], on_enter=[b.wave(wave_tag) for wave_tag in SAFE_WAVE_TAGS] + [b.count("WavesDone")]))
    return steps


ROOM_STEPS = {
    "Landing": landing_steps, "Dash": dash_steps, "Shoot": shoot_steps, "Deploy": deploy_steps,
    "Special": special_steps, "Dummies": dummies_steps, "Zones": zones_steps, "Miniboss": miniboss_steps,
}


WAITING_RULES = {"Landing": landing_nag}


def build_final_exit():
    """The last built room's exit pad leaves the tutorial: its own tag moves nobody, a volume over it ends the run.
    A room that held the role before returns to the shared tag."""
    for room in ROOMS[:-1]:
        former = find_actor("Tut_%s_PadExit" % room)
        former.set_editor_property("TeleportTag", tag(SHARED_PAD_TAG))
        remove_actor("Tut_%s_PadExitVolume" % room)
    exit_pad = find_actor("Tut_%s_PadExit" % ROOMS[-1])
    exit_pad.set_editor_property("TeleportTag", tag(FINAL_EXIT_TAG))
    volume = place("Tut_%s_PadExitVolume" % ROOMS[-1], unreal.TriggerBox, exit_pad.get_actor_location().to_tuple())
    volume.set_actor_scale3d(unreal.Vector(1.0, 1.0, 2.0))


def build_encounters():
    for room in ROOMS:
        encounter = encounter_actor(room)
        builder = Builder(encounter)
        waiting = WAITING_RULES[room](builder) if room in WAITING_RULES else []
        after = exit_steps(builder, room) if room == ROOMS[-1] else leave_steps(builder, room, waiting)
        encounter.set_editor_property("Steps", ROOM_STEPS[room](builder) + after)
        encounter.set_editor_property("bStartOnBeginPlay", room == ROOMS[0])
        encounter.set_editor_property("JumpStart", find_actor("Tut_%s_JumpStart" % room))
        log("encounter", room, len(encounter.get_editor_property("Steps")), "steps")


# Outliner folders: Tutorial/Rooms/<NN_Room>/<Category>; camera, lighting and layout stay global.
CATEGORIES = [
    ("Encounter", "Encounter"), ("Pad", "Teleporters"), ("JumpStart", "Teleporters"), ("CameraVolume", "Camera"),
    ("ArenaVolume", "Arena"), ("Entrance", "Points"), ("Spawn", "Points"), ("Chaser", "Points"),
    ("Shooter", "Points"), ("Mover", "Points"), ("ZonePoint", "Points"), ("WaveStart", "Points"),
    ("PlatformVolume", "Gameplay"), ("ZoneVolume", "Gameplay"), ("CasterArena", "Arena"), ("Cat", "Gameplay"),
    ("MinibossPoint", "Points"), ("BossSpawn", "Points"), ("FightLocation", "Points"), ("Arena", "Arena"),
    ("Floor", "Geometry"), ("Wall", "Geometry"),
]
GLOBAL_FOLDERS = {"BP_GeoCam": "Camera", "DirectionalLight": "Lighting", "PlayerStart": "Layout",
                  "Tut_Wall_Start": "Layout", "Tut_Wall_End": "Layout"}


def folder_of(label):
    if label in GLOBAL_FOLDERS:
        return "Tutorial/" + GLOBAL_FOLDERS[label]
    parts = label.split("_")
    if len(parts) >= 3 and parts[0] == "Tut" and parts[1] in ROOMS:
        room = "%02d_%s" % (ROOMS.index(parts[1]), parts[1])
        if parts[2] == "CameraVolume":
            return "Tutorial/Camera/" + room
        for key, category in CATEGORIES:
            if key in parts[2]:
                return "Tutorial/Rooms/%s/%s" % (room, category)
    return None


def organize_folders():
    """Every level actor without a folder into one (a folder set by hand stays); whatever matches no rule is reported, never left silently loose."""
    loose = []
    for actor in actors():
        folder = folder_of(actor.get_actor_label())
        if str(actor.get_folder_path()) not in ("", "None"):
            continue
        if folder is None:
            loose.append("%s (%s)" % (actor.get_actor_label(), actor.get_class().get_name()))
        else:
            actor.modify()  # a folder change alone leaves the level clean, so the save would skip it
            actor.set_folder_path(folder)
    log("actors without a folder:", loose)


def run():
    REPORT[:] = []
    try:
        unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(TUTORIAL_MAP)
        for room in ROOMS[1:]:
            remove_actor("Tut_%s_ArrivalZone" % room)
        build_landing_room()
        build_shoot_room()
        build_shoot_first_point()
        build_deploy_room()
        build_special_room()
        build_dummies_room()
        build_zones_room()
        build_miniboss_room()
        build_final_exit()
        build_encounters()
        organize_folders()
        log("saved", unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level())
        log("OK")
    except Exception:
        log("FAILED", traceback.format_exc())
    flush()


if __name__ == "__main__":
    run()
