"""Triangle "Momentum" passive: every hit the character or its deployables land stacks a damage boost.

`create_effect()` only needs the DamageMultiplier attribute, so it runs any time. `create_ability()` needs
UGeoTriggeredEffectAbility compiled and Ability.Spell.Momentum resolvable — the tag lands in
Config/Tags/GeoGameplayTags.ini, which only takes effect after an editor restart.

Run via mcp-unreal execute_script; results land in REPORT since nothing printed comes back through the tool.
"""
import unreal

FOLDER = "/Game/AbilitySystem/Abilities/Triangle/Passive"
EFFECT_PATH = FOLDER + "/GE_Triangle_Momentum"
ABILITY_PATH = FOLDER + "/GA_Triangle_Momentum"
# Source of the DamageMultiplier modifier and duration structs, cloned so the attribute literal is never hand-written.
TEMPLATE_EFFECT = "/Game/AbilitySystem/GameplayEffects/Buffs/GE_DamageMultiplierBuff"
ABILITY_INFO = "/Game/AbilitySystem/Data/DA_AbilityInfo"

ABILITY_TAG = "Ability.Spell.Momentum"
DISPLAY_NAME = "Momentum"
# Empty + inverted + Any source = every hit the character or anything it deployed lands, nothing excluded.
HIT_TRIGGERS = []
INVERT_HIT_TRIGGERS = True
ACTIVATION_TRIGGERS = []

DAMAGE_PER_STACK = 0.01
DURATION = 3.0
# Same SetByCaller tag as the template GE_DamageMultiplierBuff modifier — must match so the ability's
# EffectDataInstances wrapper (DataTag+Magnitude) is what actually drives the modifier, not a hardcoded value.
# That's also the only thing a {Token} in the description can read, so this is what makes {EffectDataInstances}
# resolve to the real per-stack damage boost instead of 0.
MAGNITUDE_DATA_TAG = "Status.Buff.DamageBoost"

REPORT = "C:/GeoTrinity/Saved/triangle_momentum_report.txt"
lines = []


def note(text):
    lines.append(text)
    unreal.log_warning("MOMENTUM:: " + text)


def free_path(path):
    """Creating over a name already in use breaks in the editor, and a held path refuses deletion."""
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        return
    if not unreal.EditorAssetLibrary.delete_asset(path):
        unreal.EditorAssetLibrary.rename_asset(path, path + "_old")


def fresh(struct, text):
    """A default-constructed struct of the same type holding only `text`'s fields."""
    out = type(struct)()
    out.import_text(text)
    return out


def cdo_of(path):
    return unreal.get_default_object(unreal.load_asset(path).generated_class())


def make_blueprint(name, folder, parent_path):
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", unreal.load_class(None, parent_path))
    return unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, unreal.Blueprint, factory)


def tag_container(names):
    container = unreal.GameplayTagContainer()
    container.import_text("(GameplayTags=(%s))" % ",".join('(TagName="%s")' % n for n in names))
    return container


def set_hit_trigger_source(cdo):
    """EGeoHitTriggerSource is a plain UENUM, so Python has no type for it and can't set it — the Remote Control
    API can, by name. Left for the caller to set that way when this reports it."""
    enum_type = getattr(unreal, "GeoHitTriggerSource", None)
    if enum_type is None:
        note("HitTriggerSource NOT set (unscriptable UENUM) — set it to Any over Remote Control or by hand")
        return
    cdo.set_editor_property("HitTriggerSource", enum_type.ANY)


def create_effect():
    """The damage buff itself: +1% per hit, each application its own 3s instance (StackingType None), so the boost
    accumulates while hits keep landing and bleeds off one instance at a time."""
    free_path(EFFECT_PATH)
    blueprint = make_blueprint(EFFECT_PATH.rsplit("/", 1)[1], FOLDER, "/Script/GameplayAbilities.GameplayEffect")
    cdo = unreal.get_default_object(blueprint.generated_class())
    template = cdo_of(TEMPLATE_EFFECT)

    # Keep the template's SetByCaller modifier as-is (tag Status.Buff.DamageBoost) — the actual per-stack value is
    # supplied at apply time by the ability's EffectDataInstances wrapper, not hardcoded here.
    modifier = type(template.get_editor_property("Modifiers")[0])()
    modifier.import_text(template.get_editor_property("Modifiers")[0].export_text())

    duration_magnitude = "(MagnitudeCalculationType=ScalableFloat,ScalableFloatMagnitude=(Value=%f))"
    cdo.set_editor_property("DurationPolicy", unreal.GameplayEffectDurationType.HAS_DURATION)
    cdo.set_editor_property("DurationMagnitude",
                            fresh(template.get_editor_property("DurationMagnitude"), duration_magnitude % DURATION))
    cdo.set_editor_property("Modifiers", [modifier])
    cdo.set_editor_property("StackingType", unreal.GameplayEffectStackingType.NONE)

    unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False)
    note("effect %s -> %s" % (EFFECT_PATH, cdo.get_editor_property("Modifiers")[0].export_text()))
    return blueprint


def create_ability():
    """The passive that watches the two hit abilities and the reload, and applies the buff to itself."""
    free_path(ABILITY_PATH)
    blueprint = make_blueprint(ABILITY_PATH.rsplit("/", 1)[1], FOLDER,
                               "/Script/GeoTrinity.GeoTriggeredEffectAbility")
    cdo = unreal.get_default_object(blueprint.generated_class())

    entry = unreal.InstancedStruct()
    entry.import_text(
        '/Script/GeoTrinity.GameplayEffectData(GameplayEffect="%s\'%s_C\'",'
        'DataTag=(TagName="%s"),Magnitude=(Value=%f),Duration=(Value=%f))'
        % ("/Script/Engine.BlueprintGeneratedClass", EFFECT_PATH + "." + EFFECT_PATH.rsplit("/", 1)[1],
           MAGNITUDE_DATA_TAG, DAMAGE_PER_STACK, DURATION))

    cdo.set_editor_property("AbilityTags", tag_container([ABILITY_TAG, "Ability.Type.Passive"]))
    cdo.set_editor_property("HitTriggerTags", tag_container(HIT_TRIGGERS))
    cdo.set_editor_property("bInvertHitTriggerTags", INVERT_HIT_TRIGGERS)
    set_hit_trigger_source(cdo)
    cdo.set_editor_property("ActivationTriggerTags", tag_container(ACTIVATION_TRIGGERS))
    cdo.set_editor_property("EffectDataInstances", [entry])
    cdo.set_editor_property("NetExecutionPolicy", unreal.GameplayAbilityNetExecutionPolicy.SERVER_INITIATED)
    cdo.set_editor_property("InstancingPolicy", unreal.GameplayAbilityInstancingPolicy.INSTANCED_PER_ACTOR)

    unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False)
    note("ability %s -> tags %s / hit %s / activation %s / effects %s"
         % (ABILITY_PATH,
            cdo.get_editor_property("AbilityTags").export_text(),
            cdo.get_editor_property("HitTriggerTags").export_text(),
            cdo.get_editor_property("ActivationTriggerTags").export_text(),
            [e.export_text() for e in cdo.get_editor_property("EffectDataInstances")]))
    return blueprint


def register_ability(blueprint):
    """Replaces every Momentum entry in DA_AbilityInfo's Triangle list with one built off the blueprint.

    Matches on the display name as well as the tag: an entry added by hand carries neither tag nor class yet, and
    filtering on the tag alone would leave it behind as a duplicate. Any icon already picked is carried over."""
    catalog = unreal.load_asset(ABILITY_INFO)

    def is_momentum(entry):
        return (ABILITY_TAG in entry.export_text()
                or entry.get_editor_property("AbilityDisplayName") == DISPLAY_NAME)

    # One read, partitioned by the predicate: each get_editor_property call returns fresh copies, so entries taken
    # from two separate reads compare by identity and never match.
    entries = list(catalog.get_editor_property("TriangleAbilities"))
    existing = [i for i in entries if is_momentum(i)]
    infos = [i for i in entries if not is_momentum(i)]
    icon = next((i.get_editor_property("AbilityIcon") for i in existing
                 if i.get_editor_property("AbilityIcon")), None)
    # AbilityIcon is EditDefaultsOnly, so Python refuses to set it on an instance — the literal goes through.
    icon_text = ('AbilityIcon="%s\'%s\'",' % (icon.get_class().get_path_name(), icon.get_path_name())) if icon else ""

    # The player lists hold FPlayersGameplayAbilityInfo; only EnemyAbilityInfos holds the base struct.
    # No Description: Content/Data/AbilityDescriptions.txt owns it, and PostLoad pulls the section in from there.
    info = unreal.PlayersGameplayAbilityInfo()
    info.import_text(
        '(TypeOfAbilityTag=(TagName="Ability.Type.Passive"),bGiveAtStartup=True,%s'
        'AbilityClass="/Script/Engine.BlueprintGeneratedClass\'%s\'",AbilityTag=(TagName="%s"),'
        'AbilityDisplayName="%s")'
        % (icon_text, blueprint.generated_class().get_path_name(), ABILITY_TAG, DISPLAY_NAME))
    note("replaced %d existing %s entr(y/ies), icon %s" % (len(existing), DISPLAY_NAME, icon))
    infos.append(info)

    catalog.set_editor_property("TriangleAbilities", infos)
    unreal.EditorAssetLibrary.save_loaded_asset(catalog, only_if_is_dirty=False)
    note("registered in %s (%d Triangle entries)" % (ABILITY_INFO, len(infos)))


def run(with_ability=True):
    create_effect()
    if with_ability:
        register_ability(create_ability())
    with open(REPORT, "w", encoding="utf-8") as report:
        report.write("\n".join(lines))
