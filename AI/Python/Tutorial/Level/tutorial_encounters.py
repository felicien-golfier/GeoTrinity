"""
Tutorial rooms 0-7 for GeoTrinity (Steps.md steps 0-10; the Landing room holds steps 0-1, the Zones room steps 7-9,
the Miniboss room step 10): the placed content of each room (void line and far platform, spawn points, the zone room's
zone spots and wave start, the miniboss arena and its points) and one AGeoEncounter per room, chained: a
room's last step opens its exit pad; the next room's encounter starts when the player lands in it. Cat lines are the
Tutorial Board text; the player keys come through {Key:...}. Gems and Forge steps are built later (AI/Pending/Tutorial.md).
Run after tutorial_map.py, tutorial_assets.py, tutorial_waves.py and tutorial_miniboss.py. Idempotent: actors keyed by label, the encounters' steps rewritten.
Report: AI/Output/tutorial_encounters_report.txt. Usage: headless (AI/Commands.md), editor closed.
"""
import sys
import traceback

import unreal

sys.path.insert(0, "C:/GeoTrinity/AI/Python/Tutorial/Level")
from tutorial_map import size_box  # noqa: E402  (one trigger-box sizing for both scripts)

REPORT_PATH = "C:/GeoTrinity/AI/Output/tutorial_encounters_report.txt"
TUTORIAL_MAP = "/Game/Maps/Tutorial"
TARGET_POINT_BP = "/Game/Actors/GeoTargetPoint"
DUMMY_BP = "/Game/Characters/Enemies/Training/BP_Dummy"
CURVE_TABLE = "/Game/AbilitySystem/Data/CurveTables/CT_Tutorial"
SHOOTER_TREE = "/Game/AI/Dummy/ST_Dummy_Shooter"
CHASER_TREE = "/Game/AI/Dummy/ST_Dummy_Chaser"
MOVER_TREE = "/Game/AI/Dummy/ST_Dummy_Mover"
MOVER_POINTS = [(300.0, 600.0), (300.0, 250.0), (-100.0, 425.0)]  # from the Dummies room's centre: the mover's patrol
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
# The Quest's nine respawn points, (X, Y) from the room's centre, as placed by hand in the scene: three rows of three.
SHOOT_SPAWNS = [
    (134.9, -529.3), (-186.4, -528.8), (507.9, -514.8),
    (-200.0, 0.0), (150.0, 0.0), (500.0, 0.0),
    (-195.4, 516.3), (132.2, 524.5), (497.4, 514.5),
]
QUEST_CUT_SECONDS = 45.0
DASH_KEY = "{Key:InputTag.Dash}"
DASH_ABILITY_TAG = "Ability.Spell.Dash"
HURT_ALLY_START_HEALTH = 0.1
FIRST_DUMMY_DEATH_SECONDS = 2.0  # the death montage plays out before the Cat and the quest start (placeholder)
SPECIAL_ABILITY_TAG = "Ability.Spell.MoiraBeam"
SPECIAL_KEY = "{Key:InputTag.Special}"
SPECIAL_ALLY_TAG = "TutorialSpecialAlly"  # not the Deploy ally's: that one stays alive and full behind
PAGE_LINE = "An ability page is about to pop up. Read it. Or don't, I'm a cat, not a cop."

ZONE_X = 150.0  # from the room's centre
ZONE_SPOTS = {"Damage": (0.0, 0.0), "Shield": (ZONE_X, -550.0), "Heal": (ZONE_X, 550.0)}  # from the room's centre: Damage always there, the others where a missing point is created
ZONE_RADIUS = 250.0
DAMAGE_ZONE_START_SIZE = 100.0
COLOR_ZONE_GROW_SECONDS = 8.0  # the CT_Tutorial row ColorZoneGrowSeconds (tutorial_assets.py): Shield and Heal wait for it
ZONE_BOX_HALF_Z = 80.0  # a standing capsule is inside it
PAD_BOX_HALF = 50.0  # the exit pad's footprint
WAVE_START_INSET = 650.0  # tutorial_waves.py: the front starts here, from the room's centre, and crosses it along +X
MINIBOSS_TREE = "/Game/AI/Tutorial/ST_TutorialMiniboss"  # tutorial_miniboss.py: its spells, in BP_Dummy's base tree
MINIBOSS_ABILITIES = ["Ability.Spell.TutorialSalve", "Ability.Spell.TutorialMinibossSummon", "Ability.Spell.LateralWave"]
MINIBOSS_TAG = "TutorialMiniboss"  # the boss's BossSetup.Tag: the kill line watches it
MINIBOSS_POINTS = [(150.0, -650.0), (350.0, 0.0), (150.0, 650.0)]  # from the room's centre; the boss spawns on the middle one and waits to be hit (setup AggroRadius -1)
MINIBOSS_ALLY_TAG = "TutorialMinibossAlly"  # BP_TutorialSummonedDummy's AllySetup.Tag
MINIBOSS_ALLY_LOW_RATIO = 0.3
MINIBOSS_CHIPS = ["Power", "Guard", "Vigor", "Mend", "Swift", "Precision", "Edge"]  # Data/gems.csv, tier Chip
ZONE_ATTITUDE = 7  # ETeamAttitudeBitflag: Neutral | Friendly | Hostile, a zone acts on everyone
NO_AGGRO_RADIUS = -1.0  # GeoEnemySetup default: the enemy waits to be hit
DUMMIES_AGGRO_RADIUS = 100.0  # the Dummies room's three
DEATH_TIME = 3.0  # AGeoGameState::DeathTime
WAVE_ABILITY = "/Game/AbilitySystem/Abilities/Enemy/LateralWave/GA_LateralWave"  # tutorial_waves.py: the one wave, its safe spots given per firing
SAFE_WAVE_LANES = [[-650.0, 0.0, 650.0], [-300.0, 500.0], [250.0]]  # Y of each safe lane from the wave start, per wave
HEAL_EFFECT = ('/Script/GeoTrinity.HealEffectData(bSuppressHealProvided=False,Amount=(Value=15.000000),bIsPerSecond=True,'
               'bLimitGameplayCue=True)')
SHIELD_EFFECT = ('/Script/GeoTrinity.GameplayEffectData(GameplayEffect="/Script/Engine.BlueprintGeneratedClass\'/Game/'
                 'AbilitySystem/GameplayEffects/Buffs/GE_DamageReductionBuff.GE_DamageReductionBuff_C\'",'
                 'DataTag=(TagName="Status.Buff.DamageReduction"),Magnitude=(Value=0.500000),bInfiniteDuration=True,'
                 'Icon="/Script/GeoTrinity.GeoIcon\'/Game/HUD/Icons/DA_Icon_Status_DamageReduction.'
                 'DA_Icon_Status_DamageReduction\'")')
DAMAGE_EFFECT = ("/Script/GeoTrinity.DamageEffectData(Amount=(Value=1.000000,Curve=(CurveTable=\"/Script/Engine.CurveTable'%s.CT_Tutorial'\","
                 "RowName=\"ColorZoneDamage\")),bIsPerSecond=True,bLimitGameplayCue=True)" % CURVE_TABLE)
ZONE_EFFECTS = {"Heal": HEAL_EFFECT, "Shield": SHIELD_EFFECT, "Damage": DAMAGE_EFFECT}
ZONE_COLORS = {"Heal": "Heal", "Shield": "Override", "Damage": "Damage"}  # EGeoColor of each zone (Override = white)
ZONE_TEAMS = {"Heal": unreal.Team.PLAYER, "Shield": unreal.Team.ENEMY, "Damage": unreal.Team.ENEMY}

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
    for stale in range(len(SHOOT_SPAWNS), 10):
        remove_actor("Tut_Shoot_Spawn_%d" % stale)

    for index, (x, y) in enumerate(SHOOT_SPAWNS):
        point("Tut_Shoot_Spawn_%d" % index, (cx + x, y, 0.0), "TargetPoint.AddSpawn", "Shoot")


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
    for index, (x, y) in enumerate(MOVER_POINTS):  # the mover spawns on one and patrols between them all
        point("Tut_Dummies_Mover" + ("" if index == 0 else "_%d" % index), (cx + x, y, 0.0),
              "TargetPoint.Tutorial.Mover", "Dummies")


def build_zone_points():
    cx = room_x("Zones")
    for color, (x, y) in ZONE_SPOTS.items():
        label = "Tut_Zones_ZonePoint_" + color
        if color == "Damage" or find_actor(label) is None:  # the Damage point is the room's centre; the others are placed by the user, never moved
            point(label, (cx + x, y, 0.0), "TargetPoint.Tutorial.Zone" + color, "Zones")
    point("Tut_Zones_WaveStart", (cx - WAVE_START_INSET, 0.0, 0.0), "TargetPoint.Tutorial.WaveStart", "Zones")
    remove_actor("Tut_Zones_BossSpawn")  # earlier versions: the waves fire on the encounters' shared caster now


def build_zones_room():
    """The room's points; its waves fire on the encounters' shared caster, no arena."""
    build_zone_points()
    remove_actor("Tut_Zones_ZoneVolume_Heal")  # earlier versions: the zones raise their own entry now
    remove_actor("Tut_Zones_ZoneVolume_Shield")
    remove_actor("Tut_Zones_ZoneVolume_Damage")
    remove_actor("Tut_Zones_CasterArena")


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
    boss = Builder(arena).enemy_setup(MINIBOSS_TAG, "MinibossHealth")
    boss.set_editor_property("BehaviourStateTree", unreal.load_asset(MINIBOSS_TREE))
    boss.set_editor_property("AbilityTags", [tag(ability) for ability in MINIBOSS_ABILITIES])
    arena.set_editor_property("BossClass", unreal.load_asset(DUMMY_BP).generated_class())
    arena.set_editor_property("BossSetup", boss)
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

    def say_from_file(self, category):
        """A random line of one Data/CatLines.txt category (no repeat until the category is spent)."""
        categories = unreal.GeoCatLineCategories()
        categories.set_editor_property("Names", [unreal.Name(category)])
        return self.make(unreal.GeoCatLineFromFileAction, Categories=categories, bRandom=True)

    def count(self, name, delta=1):
        return self.make(unreal.GeoAddToCounterAction, CounterName=name, Delta=delta)

    def unlock(self, ability_tag, page_line=True):
        """The Cat's page line (the Dash room's only), the grant, then the ability's page: the Cat speaks before anything
        pops up, and the page is latent, so what follows waits for the player to close it."""
        actions = [self.say(PAGE_LINE)] if page_line else []
        return actions + [self.make(unreal.GeoUnlockAbilityAction, AbilityTag=tag(ability_tag)),
                          self.make(unreal.GeoShowAbilityPageAction, AbilityTag=tag(ability_tag))]

    def delay(self, seconds):
        """A wait inside a rule or a list: delays are actions, never a trigger."""
        return self.make(unreal.GeoDelayAction, Duration=seconds)

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

    def scalable(self, row):
        """A number that reads a CT_Tutorial row."""
        handle = unreal.CurveTableRowHandle()
        handle.set_editor_property("CurveTable", unreal.load_asset(CURVE_TABLE))
        handle.set_editor_property("RowName", row)
        value = unreal.ScalableFloat()
        value.set_editor_property("Value", 1.0)
        value.set_editor_property("Curve", handle)
        return value

    def enemy_setup(self, enemy_tag, health_row, team=unreal.Team.ENEMY, state_tree=None, ability=None,
                    start_health=1.0, aggro_radius=NO_AGGRO_RADIUS):
        """The variant of BP_Dummy: tutorial enemies die at zero, their health reads a CT_Tutorial row."""
        setup = unreal.GeoEnemySetup()
        setup.set_editor_property("Team", team)
        setup.set_editor_property("bResetToFullLife", False)
        setup.set_editor_property("AggroRadius", aggro_radius)
        setup.set_editor_property("MaxHealth", self.scalable(health_row))
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

    def zone(self, color, event=None):
        """The colour's zone on its point, until the step is left. Damage grows from the point to cover the room and
        ticks the CT_Tutorial amount; the others raise `event` when a player is in."""
        effect = unreal.InstancedStruct()
        effect.import_text(ZONE_EFFECTS[color])
        params = unreal.DeployableDataParams()
        size = DAMAGE_ZONE_START_SIZE if color == "Damage" else ZONE_RADIUS
        params.import_text("(Size=%f,Color=(Color=%s),Attitude=%d)" % (size, ZONE_COLORS[color], ZONE_ATTITUDE))
        if color == "Damage":
            params.set_editor_property("GrowToSize", self.scalable("ColorZoneMaxRadius"))
            params.set_editor_property("GrowSeconds", self.scalable("ColorZoneGrowSeconds"))
        return self.make(unreal.GeoSpawnZoneAction, PurposeTag=tag("TargetPoint.Tutorial.Zone" + color),
                         ArenaTag=tag("Arena.Tutorial.Zones"), EffectDataArray=[effect], Params=params,
                         Team=ZONE_TEAMS[color], EnteredEvent=tag(event) if event else unreal.GameplayTag())

    def wave(self, lanes=()):
        """The encounters' shared caster fires the wave in the Zones room, one safe spot per lane (Y from the wave
        start); the action waits for its end."""
        data = unreal.InstancedStruct()
        data.import_text("/Script/GeoTrinity.LateralWavePatternData(SafeSpotOffsets=(%s))"
                         % ",".join("(X=0.000000,Y=%f)" % lane for lane in lanes))
        return self.make(unreal.GeoActivateAbilityAction, AbilityClass=unreal.load_asset(WAVE_ABILITY).generated_class(),
                         ArenaTag=tag("Arena.Tutorial.Zones"), bWaitForEnd=True, PatternData=data)

    # triggers
    def event(self, event_tag):
        return self.make(unreal.GeoEventTrigger, Tag=tag(event_tag))

    def entered(self, volume_actor):
        return self.make(unreal.GeoVolumeEnteredTrigger, Volume=volume_actor)

    def died(self, watched_class, watched_tag=""):
        return self.make(unreal.GeoActorDiedTrigger, Watched=unreal.GeoActorFilter(watched_class, watched_tag))

    def dummy_died(self, watched_tag):
        return self.died(unreal.load_asset(DUMMY_BP).generated_class(), watched_tag)

    def player_died(self):
        return self.died(watched_class=unreal.PlayableCharacter.static_class())

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


def encounter_actor(room):
    label = "Tut_%s_Encounter" % room
    return place(label, unreal.GeoEncounter, (room_x(room), 0.0, 0.0))


def open_with_last_line(builder, room, lines):
    """The room's closing Cat lines, its exit pad opening as the last one shows (Flow.md, "Rooms")."""
    return lines[:-1] + [builder.pad("Tut_%s_PadExit" % room, True)] + lines[-1:]


def arrival_rule(builder, room):
    """The player lands in the next room: close this room's exit pad, start the next encounter (last: it aborts this
    room). On every step of the room, so a pad taken while the room still runs starts the next one at once. The next
    room's entry pad never opens (Flow.md, "Rooms")."""
    following = ROOMS[ROOMS.index(room) + 1]
    arrive = [builder.pad("Tut_%s_PadExit" % room, False),
              builder.make(unreal.GeoStartEncounterAction, Target=encounter_actor(following))]
    return builder.rule(builder.entered(find_actor("Tut_%s_ArenaVolume" % following)), arrive, completes=True)


def leave_steps(builder, room, extra_rules=()):
    """After a room: Leave waits for the arrival (on every step, room_steps) with the room's own extra_rules."""
    return [builder.step("Leave", list(extra_rules))]


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
        "**Ouch**. Landed. Welcome to wherever this is: flat, bright, suspiciously empty. Very on brand.",
        "Welcome to the bowels of the game you were playing five minutes ago. You and I landed somewhere safe, by pure luck. Your two other friends most likely landed in front of a boss. They're ~~dead~~ already. I'd observe a minute of silence, but I'm busy.",
        "Right. I'm stuck in here with you, so let's get us out. Rule one: To leave, you must kill all bosses. Dead. Tot. Muertos.",
        "How do I know that? I'd love to explain. I won't. Moving on.",
        "Also, you're the Healer. Enjoy it: it's probably the only time in your life you'll play one. No choice, it's the tutorial. Nobody picks Healer on purpose.",
        "Hold W, A, S or D and your shape slides around. Congratulations, you have unlocked walking.",
        "See the round portal on the floor? Walk onto it. I'll wait. I have nine lives and no schedule.",
    ]
    actions = [b.lock(True), b.hidden("Tut_Landing_Cat", False), b.cat_montage(CAT_REASSEMBLE, wait=False)]
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
    """While Leave waits for the player to step on the opened pad (armed after the unlock): the nag once, then puns."""
    return [
        b.rule(b.idle(), [b.say("Still standing there? The portal is not going to walk to you.")],
               condition=b.payload_count(EQUAL, 1), repeat=True),
        b.rule(b.idle(), [b.say_from_file("Pun")], condition=b.payload_count(GREATER_OR_EQUAL, 2), repeat=True),
    ]


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
    stunned = open_with_last_line(b, "Dash", [
        b.say("... ... WHAT. You just... in one go?! I wasn't even looking! Nobody does that on the first try. Nobody. "
              "Do it again, slowly, for me. For science. For my bet."), b.say(useless),
    ])
    relief = open_with_last_line(b, "Dash", [
        b.say("There. Finally. Only {Counter:Falls} deaths. In the story I'll tell, it took 'a moment'."), b.say(useless)])
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
        rules.append(b.rule(b.idle(), [b.say(line)], condition=b.payload_count(EQUAL, tier), repeat=True))
    rules.append(b.rule(b.idle(), [b.say_from_file("Pun")],
                        condition=b.payload_count(GREATER_OR_EQUAL, len(idle_lines) + 1), repeat=True))
    arrival = [
        b.say("You reached a portal by walking. Truly the peak of the genre. Next lesson: gravity."),
    ] + b.unlock(DASH_ABILITY_TAG)
    return [b.step("Dash", rules, on_enter=arrival)]


def shoot_steps(b):
    spawn_tag = "TargetPoint.AddSpawn"
    dummy = b.enemy_setup("TutorialShootDummy", "DummyHealth")
    first = b.step("Shoot", [
        b.rule(b.dummy_died("TutorialShootDummy"), [b.delay(FIRST_DUMMY_DEATH_SECONDS)], completes=True),
    ], on_enter=[
        b.spawn(dummy, 1, "TargetPoint.Tutorial.ShootFirst", "Shoot"),
        b.say("New room, new toy. This is a Dummy. It does not dodge, it does not fight back, it has no opinions. "
              "Basically your favorite raid member."),
        b.say("Your basic attack is about to be yours. Hold the attack button, release on the sweet spot and make the "
              "Dummy regret existing."),
    ] + b.unlock("Ability.Spell.BasicBeam", page_line=False))

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
        b.rule(b.counter("Cut"), stop, completes=True),
    ], on_enter=[
        b.card(unreal.GeoQuestCardOperation.SHOW, Title="Cull the Dummies", Max=100, TimeLimitSeconds=QUEST_TIME_LIMIT),
        b.spawn(dummy, len(SHOOT_SPAWNS), spawn_tag, "Shoot", respawn=QUEST_RESPAWN, kill_on_step_exit=True),
        b.say("Oh look, a quest! Nine Dummies are already here, and every one comes back after dying, so we will never run "
              "out. Other games call this 'content'."),
        b.delay(QUEST_CUT_SECONDS),
        b.count("Cut"),
    ], on_exit=[
        b.card(unreal.GeoQuestCardOperation.COUNT, bShowMax=True),
    ] + open_with_last_line(b, "Shoot", [
        b.say("Quest abandoned. Dummies, off. Quest complete! Reward: 3 copper. Mine now. Administration fee."),
        b.say("That was the first quest of every game ever, shrunk to forty seconds. You're welcome. Next toy."),
    ]) + [b.card(unreal.GeoQuestCardOperation.HIDE)])
    return [first, quest]


def deploy_steps(b):
    """One step: the hurt ally and the Cat's lines in OnEnter, the ally's full health ends it."""
    ally = b.enemy_setup("TutorialAlly", "AllyDummyHealth", team=unreal.Team.PLAYER,
                         start_health=HURT_ALLY_START_HEALTH)
    return [b.step("Heal", [
        b.rule(b.dummy_health("TutorialAlly", unreal.GeoHealthSide.ABOVE, 1.0),
               open_with_last_line(b, "Deploy", [b.say("Wow, the healer job suits you perfectly!")]), completes=True),
    ], on_enter=[
        b.say("Second toy: the deployable. You are the Healer, so yours is a Healing Zone. Hold the button to charge "
              "the throw, release to drop it. Longer press is further. Physics, basically."),
    ] + b.unlock("Ability.Spell.HealingZone", page_line=False) + [
        b.spawn(ally, 1, "TargetPoint.Tutorial.Ally", "Deploy"),
        b.say("See the friendly Dummy over there, barely standing? Drop the Zone on it and let it heal up. You can stack "
              "several to heal faster. It won't ask nicely. Dummies never do."),
    ])]


def special_steps(b):
    """Three rounds, each spawning both dummies (leaving a round kills the survivors) and tallying one counter, Progress:
    enemy dead 1, ally full 1, Special used 4. 6 = both goals and the Special: done. 2 = both goals without the Special:
    the Cat stings, then the next round respawns them; after the third, the Cat surrenders and opens the exit anyway
    (Steps.md, step 5)."""
    ally = b.enemy_setup(SPECIAL_ALLY_TAG, "AllyDummyHealth", team=unreal.Team.PLAYER,
                         start_health=HURT_ALLY_START_HEALTH)
    enemy = b.enemy_setup("TutorialRudeDummy", "DummyHealth")
    no_special_endings = [
        [b.say("Wow. You killed one Dummy and healed the other without pressing **%s** even once. That is the button "
               "I just handed you. It is right there. Humbling. Again, and this time with the beam." % SPECIAL_KEY)],
        [b.say("Twice. Two Dummies handled, zero **%s**. I'm starting to think you're doing it on purpose. Once more. "
               "With. The. Beam." % SPECIAL_KEY)],
        open_with_last_line(b, "Special", [
            b.say("Three times, and not one press of **%s**. Fine. I surrender. Take the portal, I'll write 'Special: "
                  "mastered' in your file and nobody will ever check." % SPECIAL_KEY)]),
    ]

    def done():
        return open_with_last_line(b, "Special", [
            b.say("Done? You hurt one and helped the other with the same beam. Mixed messages, passive-aggressive, "
                  "but it works.")])

    intro = [
        b.say("Third toy: the Special. Two Dummies are on their way: a friendly one, hurt, and an enemy one, rude. The "
              "Special interacts with your deployable. The ability page explains how, and unlike me, it is not "
              "sarcastic."),
    ] + b.unlock(SPECIAL_ABILITY_TAG, page_line=False)
    orders = [
        b.say("Kill the rude one and bring the hurt one back to full health. Same beam for both. The healer's natural "
              "habitat: do everything at once and get blamed for the rest."),
        b.say("Remember: your zones have 3 charges, slow refill. Spend them wisely. Or don't, I'm a cat, I don't pay the "
              "bills."),
    ]
    rounds = []
    for index, no_special in enumerate(no_special_endings):
        spawns = [b.spawn(ally, 1, "TargetPoint.Tutorial.Ally", "Special", kill_on_step_exit=True),
                  b.spawn(enemy, 1, "TargetPoint.AddSpawn", "Special", kill_on_step_exit=True)]
        rounds.append(b.step("Round%d" % (index + 1), [
            b.rule(b.dummy_health(SPECIAL_ALLY_TAG, unreal.GeoHealthSide.ABOVE, 1.0), [b.count("Progress")]),
            b.rule(b.dummy_died("TutorialRudeDummy"), [b.count("Progress")]),
            b.rule(b.ability_used(SPECIAL_ABILITY_TAG), [b.count("Progress", 4)]),
            b.rule(b.counter("Progress"), no_special, condition=b.payload_count(EQUAL, 2), completes=True),
            b.rule(b.counter("Progress"), done(), condition=b.payload_count(GREATER_OR_EQUAL, 6), completes=True,
                   step_after="Leave"),
        ], on_enter=intro + spawns + orders if index == 0 else spawns))
    return rounds


def dummies_steps(b):
    chaser = b.enemy_setup("TutorialChaser", "DummyHealth", state_tree=CHASER_TREE, aggro_radius=DUMMIES_AGGRO_RADIUS)
    shooter = b.enemy_setup("TutorialShooter", "DummyHealth", state_tree=SHOOTER_TREE, ability=SHOOT_ABILITY_TAG,
                            aggro_radius=DUMMIES_AGGRO_RADIUS)
    mover = b.enemy_setup("TutorialMover", "DummyHealth", state_tree=MOVER_TREE, aggro_radius=DUMMIES_AGGRO_RADIUS)

    def spawner(setup, purpose):
        return b.spawn(setup, 1, purpose, "Dummies")

    def watch(enemy_tag):
        return b.rule(b.dummy_died(enemy_tag), [b.count("Dead")])

    lines = [
        b.say("Plot twist: the Dummies have been training. One chases you, one shoots, one just wanders off and acts "
              "annoying. Truly a full raid composition."),
        b.say("Use everything you learned. Dash the bullets, drop your toy, pop your Special. I'll narrate if anyone "
              "dies. I'm told I'm very supportive."),
    ]
    fight = b.step("Fight", [
        watch("TutorialChaser"), watch("TutorialShooter"), watch("TutorialMover"),
        b.rule(b.counter("Dead"), open_with_last_line(b, "Dummies", [
            b.say("All three down. You've officially beaten the cardboard. Don't let it go to your head. The floor is "
                  "about to get opinions.")]), condition=b.payload_count(EQUAL, 3), completes=True),
    ], on_enter=[lines[0],  # the first line, then the three, then the second
        spawner(chaser, "TargetPoint.Tutorial.Chaser"),
        spawner(shooter, "TargetPoint.Tutorial.Shooter"),
        spawner(mover, "TargetPoint.Tutorial.Mover"),
    ] + lines[1:])
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
    rules.append(b.rule(b.dummy_died(MINIBOSS_TAG), open_with_last_line(b, "Miniboss", [
        b.say("It's dead. You've got it: time to come back to the real world. Your snacks are cold and someone has to "
              "roll initiative. But first: loot.")]), completes=True))
    return [b.step("Fight", rules, on_enter=[
        b.say("Meet the Miniboss. Large, angry, and on a budget: it only owns three places to stand. Good luck. I'll be "
              "quiet. Mostly.")])]


def zones_steps(b):
    """Steps 7-9. One step for the colours (the Damage zone grows and stays, then Shield and Heal appear inside it; the
    player stood in both once, 2 s, the step is left and every zone goes), the lethal wave (the player's death leaves),
    then the safe-spot waves; a death leaves SafeSpots for SafeSpotsWait, which comes back after DeathTime."""
    intro = [
        "Welcome to the Color Room. The floor will now heal, shield and hurt you. Think of it as customer service.",
        "Orange hurts. It starts small and spreads until it owns the whole floor, so don't get attached to your "
        "position. Green heals. Blue shields. The color tells you what the floor will do before it does it. Remember "
        "that. It is the whole game.",
        "Now stand in the shield, then in the heal. Yes, while the orange is nibbling at you. An educational nibble.",
    ]
    colors_learned = [b.delay(2.0), b.say("Colors learned? Good. There is one more. I'll show you in a minute. Stretch.")]
    rules = [b.rule(b.event(event), [b.count("Stood")]) for event in ("Encounter.Event.ZoneShield",
                                                                       "Encounter.Event.ZoneHeal")]
    rules.append(b.rule(b.counter("Stood"), colors_learned, condition=b.counter_is("Stood", EQUAL, 2), completes=True))
    steps = [b.step("ZoneColors", rules, on_enter=[
        b.say(intro[0]), b.zone("Damage"), b.say(intro[1]), b.say(intro[2]), b.delay(COLOR_ZONE_GROW_SECONDS),
        b.zone("Shield", "Encounter.Event.ZoneShield"), b.zone("Heal", "Encounter.Event.ZoneHeal")])]
    steps.append(b.step("LethalWave", [
        b.rule(b.player_died(), [
            b.say("Oh, it hurts? Upsy, I forgot to leave you room to survive...", True),
            b.say("But get used to it. Dying is part of the job, you will die a lot. At least statistically."),
            b.say("Okay, that was a bit unfair. Yes, on purpose. I'm sorry. A little."),
            b.say("Look, I added safe spots in the wave. For you. Out of the goodness of my heart. Don't tell the other "
                  "cats."),
        ], completes=True),
    ], on_enter=[b.delay(0.5), b.say("Big wave is coming! Avoid it! Quick, quick, you can do it!"),
                 b.wave()]))
    steps.append(b.step("SafeSpotsWait", [b.rule(b.counter("Waited"), [], completes=True)],
                        on_enter=[b.delay(DEATH_TIME), b.count("Waited")]))
    # The exit pad opens from the finishing rule, not OnExit: a death leaves this step too.
    steps.append(b.step("SafeSpots", [
        b.rule(b.player_died(), [b.say("Get into a spot before the wave lands. Then another wave. The spots move, so you "
                                       "move. It's like dancing, but you die.", True)],
               completes=True, step_after="SafeSpotsWait"),
        b.rule(b.counter("WavesDone"), open_with_last_line(b, "Zones", [
            b.say("Survived? Don't get cocky: this wave was nicely labeled. The real ones are not.")]),
               condition=b.payload_count(EQUAL, 1), completes=True),
    ], on_enter=[b.wave(lanes) for lanes in SAFE_WAVE_LANES] + [b.count("WavesDone")]))
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
    size_box(volume, PAD_BOX_HALF, PAD_BOX_HALF, ZONE_BOX_HALF_Z)


def room_steps(builder, room):
    """Every step of the room: its own, then Leave (the last room: Exit); before the last room, each step also watches
    the arrival in the next room (arrival_rule)."""
    waiting = WAITING_RULES[room](builder) if room in WAITING_RULES else []
    after = exit_steps(builder, room) if room == ROOMS[-1] else leave_steps(builder, room, waiting)
    steps = ROOM_STEPS[room](builder) + after
    if room != ROOMS[-1]:
        for step in steps:
            step.set_editor_property("Rules", list(step.get_editor_property("Rules")) + [arrival_rule(builder, room)])
    return steps


def build_encounters():
    for room in ROOMS:
        encounter = encounter_actor(room)
        builder = Builder(encounter)
        encounter.set_editor_property("Steps", room_steps(builder, room))
        encounter.set_editor_property("bStartOnBeginPlay", room == ROOMS[0])
        encounter.set_editor_property("JumpStart", find_actor("Tut_%s_JumpStart" % room))
        log("encounter", room, len(encounter.get_editor_property("Steps")), "steps")


# Outliner folders: Tutorial/Rooms/<NN_Room>/<Category>; camera, lighting and layout stay global.
CATEGORIES = [
    ("Encounter", "Encounter"), ("Pad", "Teleporters"), ("JumpStart", "Teleporters"), ("CameraVolume", "Camera"),
    ("ArenaVolume", "Arena"), ("Entrance", "Points"), ("Spawn", "Points"), ("Chaser", "Points"),
    ("Shooter", "Points"), ("Mover", "Points"), ("ZonePoint", "Points"), ("WaveStart", "Points"),
    ("PlatformVolume", "Gameplay"), ("Cat", "Gameplay"),
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
