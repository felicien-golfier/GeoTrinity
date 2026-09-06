"""A heartbeat running around NS_ChargedHalo's frame, breaking its line into a travelling wave.

Purely ADDITIVE. The Frame, Blades, Snap and Shed layers geo_aura_vfx.py built are never opened, and no
other system is touched: this only appends three emitters to the system named in TARGET.

The frame is one hollow triangle sprite, so no parameter on it can ripple its outline — a wave along a line
needs the line to be many particles. Each Beat layer is therefore one EDGE of that triangle: a beam between
two corners, which spaces its particles evenly along the chord with no lattice module and hands every one of
them `Particles.RibbonLinkOrder`, its position from 0 to 1 along that edge. Three edges laid end to end give
a perimeter coordinate, and one waveform read at that coordinate is the beat.

The corners have to turn with the frame or the beat slides off the line within seconds, and the frame turns
by a sprite rotation rate, which moves nothing through space. So the corners are trig on the emitter's own
age — the one placement whose trig actually advances, since a particle script resolves its angle once at
birth. Cosine and Sine share that age and hold no phase input of their own (their graph drives it), so a
corner 120 degrees along is written as the linear combination R*cos(t+f) = R*cos f*cos t - R*sin f*sin t.
That needs a real add, and Cosine's own Bias is not one: built on it the three edges land on an ellipse and
the triangle does not close. Add_Vector puts the two halves together instead. UpdateBeam then re-derives
every particle from those corners each frame, which is also what lets the wave displace them afterwards:
anything above that re-derivation is overwritten, anything below it survives.

The wave itself is one `Waveform` dynamic input read three times — size, alpha and a push off the line —
because a crest that only brightens reads as a light running around a rail, and one that only bulges reads
as a wobble. Together they read as a pulse travelling through the line and breaking it. Its input position
is the perimeter coordinate, so a single crest crosses all three edges without a seam.

PHASE and SPIN_PERIOD's sign are the two numbers that cannot be derived: which way a sprite rotation rate
turns on screen, and where the material's own triangle points inside its quad, are both conventions. One
frame of the system on screen shows the beat line against the frame line and settles them.

A run is `prepare()`, then `main()`; `retune()` rewrites the numbers afterwards without adding a node.
Re-running `main()` needs the previous Beat handles stripped over the `niagara_ops` route first —
`handles()` names them — and any actor playing the system destroyed before that, since a component ticking
a system mid-strip brings the editor down. `main()` is the only path that may link an input or nest a
dynamic input: doing either onto an override pin that already holds one is a checkf inside Niagara, which
kills the editor rather than returning false.

Reference: AI/ArtDirection.md, AI/MCP/MCP_Niagara.md, AI/VFX.md.
"""

import math
import traceback

import unreal

TARGET = "/Game/Art/VFX/Generic/Niagara/NS_ChargedHalo.NS_ChargedHalo"
LAYER_SOURCE_FOLDER = "/Game/Art/VFX/Generic/Niagara/_BeatSources"
TEMPLATE_SPRITE = "/Niagara/DefaultAssets/Templates/Emitters/SimpleSpriteBurst.SimpleSpriteBurst"
SHAPE = "/Game/Art/VFX/Generic/Materials/MatInstances/MI_GeoShape_TriSolid.MI_GeoShape_TriSolid"

# --- the frame this rides on, as geo_aura_vfx.py authored it -------------------------------------------
FRAME_SIZE = 175.0  # the Frame layer's Uniform Sprite Size
FRAME_SPIN = 18.0  # its Rotation Rate, degrees a second
FIT = 0.92  # M_GeoShape: circumradius as a fraction of the sprite's half width
THIN = 0.075  # MI_GeoShape_TriThin's outline thickness, in the same units

SIDES = 3
APOTHEM = math.cos(math.pi / SIDES)
# The centre of the drawn stroke, not the polygon's outer edge: the outline runs inward from the apothem.
CORNER = FRAME_SIZE * 0.5 * (FIT * APOTHEM - THIN * 0.5) / APOTHEM
SPIN_PERIOD = 360.0 / FRAME_SPIN  # seconds a turn; negate to turn the other way
# M_GeoShape puts a polygon's vertices at 60 degrees in sprite UV space, and this camera's UV-to-world
# adds a further 90, so the frame's own line sits a twelfth of a turn round from a beat corner at zero.
PHASE = 1.0 / 12.0  # turns, rotating the whole beat triangle onto the frame's own line

# --- the beat ------------------------------------------------------------------------------------------
COUNT = 9  # shapes per edge, so 27 hold the whole perimeter
SIZE = 8.0
COLOR = "2.20,0.25,5.20,1"
SPIN = 150.0  # each shape turning on itself, so the chain is never a row of identical stamps
HELD = 100000.0  # a lifetime and a loop long enough that the burst never fires twice

WAVES = 3.0  # crests around the whole frame: one per edge, so the beat is a shape and not a lean
TRAVEL = 0.45  # Waveform phase scale — how fast that crest runs round
SHARP = 1.0  # waveform exponent: an even power folds the trough into a second crest
SWELL = (0.60, 0.62)  # sprite size multiplier: what it rests at, and how far the beat swings it
GLOW = (0.50, 0.52)  # alpha, the same way
BULGE = (0.0, 8.0)  # unreal units the crest throws the line off itself, to both sides
CLAMP = (-1000.0, 1000.0)  # wide enough that only Bias and the swing decide the wave

EMITTER_UPDATE = unreal.NiagaraScriptUsage.EMITTER_UPDATE_SCRIPT
PARTICLE_SPAWN = unreal.NiagaraScriptUsage.PARTICLE_SPAWN_SCRIPT
PARTICLE_UPDATE = unreal.NiagaraScriptUsage.PARTICLE_UPDATE_SCRIPT

EMITTER_STATE = "EmitterState"
BURST = "SpawnBurst_Instantaneous"
INITIALIZE_PARTICLE = "InitializeParticle"
TEMPLATE_FADE = "ScaleColor"  # the template's own, which holds alpha at ~0 at every age

MODULE_BEAM_SETUP = "/Niagara/Modules/Beams/BeamEmitterSetup.BeamEmitterSetup"
MODULE_SPAWN_BEAM = "/Niagara/Modules/Beams/SpawnBeam.SpawnBeam"
MODULE_UPDATE_BEAM = "/Niagara/Modules/Beams/UpdateBeam.UpdateBeam"
MODULE_OFFSET_POSITION = "/Niagara/Modules/Update/Position/OffsetPosition.OffsetPosition"
MODULE_SCALE_SPRITE_SIZE = "/Niagara/Modules/Update/Size/ScaleSpriteSize.ScaleSpriteSize"
MODULE_SCALE_COLOR = "/Niagara/Modules/Update/Color/ScaleColor.ScaleColor"
MODULE_SPRITE_ROTATION_RATE = "/Niagara/Modules/Update/Orientation/SpriteRotationRate.SpriteRotationRate"

DYN_ADD_VECTOR_TO_POSITION = "/Niagara/DynamicInputs/Vectors/Position/AddVectorToPosition.AddVectorToPosition"
DYN_CONVERT_VECTOR_TO_POSITION = "/Niagara/DynamicInputs/Transforms/ConvertVectorToPosition.ConvertVectorToPosition"
DYN_SIMULATION_POSITION = "/Niagara/DynamicInputs/Helpers/SimulationPosition.SimulationPosition"
DYN_MAKE_VECTOR = "/Niagara/DynamicInputs/TypeConversions/MakeVector.MakeVector"
DYN_COSINE = "/Niagara/DynamicInputs/Angles/Cosine.Cosine"
DYN_SINE = "/Niagara/DynamicInputs/Angles/Sine.Sine"
DYN_WAVEFORM = "/Niagara/DynamicInputs/Helpers/Waveform.Waveform"
DYN_SCALE_AND_BIAS = "/Niagara/DynamicInputs/Multiply/ScaleAndBiasFloat.ScaleAndBiasFloat"
DYN_ADD_VECTOR = "/Niagara/DynamicInputs/Add/Add_Vector.Add_Vector"
DYN_MULTIPLY_VECTOR_BY_FLOAT = "/Niagara/DynamicInputs/Multiply/Multiply_VectorByFloat.Multiply_VectorByFloat"

NODES = {}
LOG = []


def builder():
    return unreal.GeoNiagaraBuilderUtil.get_default_object()


def record(label, value):
    LOG.append("%-64s %s" % (label, value))
    return value


def names():
    return ["Beat%d" % edge for edge in range(SIDES)]


def handles():
    """The handles a re-run has to strip over the ops route before prepare(), newest build last."""
    return names()


def clear_asset(package_path):
    """Frees a package path so an asset can be written to it; one still held open is renamed aside."""
    if not unreal.EditorAssetLibrary.does_asset_exist(package_path):
        return True
    return (unreal.EditorAssetLibrary.delete_asset(package_path)
            or unreal.EditorAssetLibrary.rename_asset(package_path, package_path + "_Superseded"))


def duplicate_into(source_path, folder, name):
    """Duplicates over folder/name; duplication returns nothing when the name is already taken."""
    clear_asset("%s/%s" % (folder, name))
    duplicate = unreal.AssetToolsHelpers.get_asset_tools().duplicate_asset(
        name, folder, unreal.load_object(None, source_path))
    record(name, duplicate.get_path_name() if duplicate else "FAILED")
    return duplicate


def prepare():
    """One emitter source per edge, carrying the shape and the local space no stack edit can reach."""
    for name in names():
        source = duplicate_into(TEMPLATE_SPRITE, LAYER_SOURCE_FOLDER, name)
        if not source:
            continue
        # Set here, not in the system: inside a system an emitter's object name is not its handle name.
        sprite = unreal.find_object(source, "NiagaraSpriteRendererProperties_0")
        if sprite:
            sprite.set_editor_property("Material", unreal.load_object(None, SHAPE))
        record("%s renderer" % name, "MI_GeoShape_TriSolid" if sprite else "NOT FOUND")
        record("%s local space" % name,
               builder().set_emitter_property(source.get_path_name(), "bLocalSpace", "true"))
        unreal.EditorAssetLibrary.save_loaded_asset(source)
    return "\n".join(LOG)


# --- addressing ----------------------------------------------------------------------------------------
# Every shim call that misses fires an ensure, which halts the game thread under a debugger, so a node
# that failed to nest is never addressed and no input is written outside the branch its switches expose.

def ok(node):
    return node and node != "None"


def write(system, name, usage, node, input_name, value):
    if not ok(node):
        return record("%s %s %s" % (name, node, input_name), "SKIPPED")
    return record("%s %s %s" % (name, node, input_name),
                  builder().set_input_value(system, name, usage, node, input_name, str(value)))


def link(system, name, usage, node, input_name, parameter):
    if not ok(node):
        return record("%s %s %s" % (name, node, input_name), "SKIPPED")
    return record("%s %s <- %s" % (name, node, parameter),
                  builder().set_input_linked_parameter(system, name, usage, node, input_name, parameter))


def switch(system, name, usage, node, switch_name, entry):
    if not ok(node):
        return record("%s %s %s" % (name, node, switch_name), "SKIPPED")
    return record("%s %s %s" % (name, node, switch_name),
                  builder().set_static_switch(system, name, usage, node, switch_name, entry))


def nest(system, name, usage, node, input_name, script):
    if not ok(node):
        return record("%s %s %s" % (name, node, input_name), "SKIPPED")
    return record("%s %s %s" % (name, node, input_name), str(
        builder().set_input_dynamic_input(system, name, usage, node, input_name, script)))


def add(system, name, usage, script):
    return record("%s add %s" % (name, script.rsplit("/", 1)[-1]),
                  str(builder().add_module(system, name, usage, script)))


# --- the stack -----------------------------------------------------------------------------------------

def add_modules(system, name):
    """Everything below UpdateBeam survives its re-derivation; everything above it is overwritten."""
    node = NODES[name] = {}
    record("%s disable template fade" % name,
           builder().set_module_enabled(system, name, PARTICLE_UPDATE, TEMPLATE_FADE, False))
    node["beam"] = add(system, name, EMITTER_UPDATE, MODULE_BEAM_SETUP)
    node["place"] = add(system, name, PARTICLE_SPAWN, MODULE_SPAWN_BEAM)
    node["follow"] = add(system, name, PARTICLE_UPDATE, MODULE_UPDATE_BEAM)
    node["bulge"] = add(system, name, PARTICLE_UPDATE, MODULE_OFFSET_POSITION)
    node["swell"] = add(system, name, PARTICLE_UPDATE, MODULE_SCALE_SPRITE_SIZE)
    node["glow"] = add(system, name, PARTICLE_UPDATE, MODULE_SCALE_COLOR)
    node["spin"] = add(system, name, PARTICLE_UPDATE, MODULE_SPRITE_ROTATION_RATE)

    switch(system, name, EMITTER_UPDATE, EMITTER_STATE, "Loop Behavior", "Infinite")
    for switch_name in ("Lifetime Mode", "Color Mode"):
        switch(system, name, PARTICLE_SPAWN, INITIALIZE_PARTICLE, switch_name, "Direct Set")
    # The curve branch is the default and exposes no plain factor to hang a waveform on.
    switch(system, name, PARTICLE_UPDATE, NODES[name]["swell"], "Scale Sprite Size Mode", "Uniform")


def axis_of(edge):
    """Where a corner sits at rest, as the cosine and sine coefficients its two halves are scaled by."""
    turn = 2.0 * math.pi * (edge / float(SIDES) + PHASE)
    return CORNER * math.cos(turn), CORNER * math.sin(turn)


def corner_axes(edge):
    """The start corner, then the step from it to the next one.

    BeamEmitterSetup adds Beam Start into Beam End, so the end is a displacement and not a place: handed
    the next corner outright, every edge ran from its corner to the sum of the two, which lands at the same
    radius but half the angle on and draws three sides of a hexagon. The step rotates with the corner it is
    added to, so one set of trig still carries both.
    """
    start, end = axis_of(edge), axis_of(edge + 1)
    return [start, (end[0] - start[0], end[1] - start[1])]


def build_corners(system, name, edge):
    """Beam Start is an absolute position and needs the emitter's own under it; Beam End is an offset."""
    node = NODES[name]
    node["start"] = nest(system, name, EMITTER_UPDATE, node["beam"], "Beam Start",
                         DYN_ADD_VECTOR_TO_POSITION)
    node["end"] = nest(system, name, EMITTER_UPDATE, node["beam"], "Beam End",
                       DYN_CONVERT_VECTOR_TO_POSITION)
    node["axes"] = corner_axes(edge)


def build_corner_sums(system, name):
    """A corner is R*cos(f)*cos(t) - R*sin(f)*sin(t) on X and its partner on Y, so it needs a real add.

    Cosine's own Bias is not one — a triangle built on it does not close — so the two halves are separate
    vectors and Add_Vector puts them together.
    """
    node = NODES[name]
    nest(system, name, EMITTER_UPDATE, node["start"], "Position", DYN_SIMULATION_POSITION)
    node["sums"] = [nest(system, name, EMITTER_UPDATE, node["start"], "Vector", DYN_ADD_VECTOR),
                    nest(system, name, EMITTER_UPDATE, node["end"], "Input Position", DYN_ADD_VECTOR)]


def build_corner_vectors(system, name):
    node = NODES[name]
    node["vectors"] = []
    for sum_node in node["sums"]:
        for half in ("A", "B"):
            node["vectors"].append(nest(system, name, EMITTER_UPDATE, sum_node, half, DYN_MAKE_VECTOR))


def build_corner_trig(system, name):
    """A is the cosine half of a corner and B the sine half; the pair rotates the rest point by the age."""
    node = NODES[name]
    node["trig"] = []
    for index, vector in enumerate(node["vectors"]):
        script = DYN_COSINE if index % 2 == 0 else DYN_SINE
        for component in ("X", "Y"):
            node["trig"].append((nest(system, name, EMITTER_UPDATE, vector, component, script),
                                 script, component, node["axes"][index // 2]))


def set_corner_values(system, name):
    node = NODES[name]
    for trig, script, component, axis in node["trig"]:
        write(system, name, EMITTER_UPDATE, trig, "Period", SPIN_PERIOD)
        write(system, name, EMITTER_UPDATE, trig, "Bias", 0.0)
        if script == DYN_COSINE:
            scale = axis[0] if component == "X" else axis[1]
        else:
            scale = -axis[1] if component == "X" else axis[0]
        write(system, name, EMITTER_UPDATE, trig, "Scale", scale)
    for vector in node["vectors"]:
        write(system, name, EMITTER_UPDATE, vector, "Z", 0.0)


def build_waves(system, name):
    """One waveform per thing the beat moves: how big a shape is, how bright, and how far off the line."""
    node = NODES[name]
    node["waves"] = [
        ("swell", nest(system, name, PARTICLE_UPDATE, node["swell"], "Uniform Scale Factor", DYN_WAVEFORM),
         SWELL),
        ("glow", nest(system, name, PARTICLE_UPDATE, node["glow"], "Scale Alpha", DYN_WAVEFORM), GLOW),
    ]
    node["push"] = nest(system, name, PARTICLE_UPDATE, node["bulge"], "Position Offset",
                        DYN_MULTIPLY_VECTOR_BY_FLOAT)


def build_wave_inputs(system, name, edge):
    """Every waveform reads the same perimeter coordinate, so one crest crosses the corners unbroken."""
    node = NODES[name]
    node["waves"].append(("push", nest(system, name, PARTICLE_UPDATE, node["push"], "Float", DYN_WAVEFORM),
                          BULGE))
    node["edge"] = edge


def build_wave_positions(system, name):
    """The switch goes in with the nesting: it decides which inputs the branch below it even has."""
    node = NODES[name]
    node["positions"] = []
    for _, wave, _ in node["waves"]:
        switch(system, name, PARTICLE_UPDATE, wave, "Waveform [1]", "Sine")
        node["positions"].append((wave, nest(system, name, PARTICLE_UPDATE, wave, "[1] Input Position",
                                             DYN_SCALE_AND_BIAS)))


def set_wave_values(system, name):
    node = NODES[name]
    for label, wave, (rest, swing) in node["waves"]:
        write(system, name, PARTICLE_UPDATE, wave, "[1] Frequency", WAVES)
        write(system, name, PARTICLE_UPDATE, wave, "[1] Phase Scale", TRAVEL)
        write(system, name, PARTICLE_UPDATE, wave, "[1] Exponent", SHARP)
        # Amplitude Min/Max is the range the amplitude is DRAWN from, not the range the wave outputs:
        # left wide it rolls a different height per read and the beat sits at nothing. The value the
        # wave swings around is Bias, and it is the only thing holding the trough off zero.
        write(system, name, PARTICLE_UPDATE, wave, "[1] Amplitude Min/Max", "%s,%s" % (swing, swing))
        write(system, name, PARTICLE_UPDATE, wave, "[1] Bias", rest)
        # The globals sit over every waveform and default to a unit range, which flattens a swing that
        # leaves it while Bias goes on moving the level — the shapes then read as one size with a tilt.
        write(system, name, PARTICLE_UPDATE, wave, "Global Amplitude Scale", 1.0)
        write(system, name, PARTICLE_UPDATE, wave, "Global Clamp Min", CLAMP[0])
        write(system, name, PARTICLE_UPDATE, wave, "Global Clamp Max", CLAMP[1])
        record("%s wave %s" % (name, label), (rest, swing))
    for wave, position in node["positions"]:
        # The edge's own third of the perimeter: 0..1 along this edge maps to edge/3 .. (edge+1)/3.
        write(system, name, PARTICLE_UPDATE, position, "Scale", 1.0 / SIDES)
        write(system, name, PARTICLE_UPDATE, position, "Bias", node["edge"] / float(SIDES))


def build_links(system, name):
    """Build only, and never twice.

    Linking an input whose override pin already holds one is a checkf inside Niagara, not a failed call —
    it takes the editor down with it — so every link lives here, off the tuning path, and a re-run reaches
    it only through a fresh emitter.
    """
    node = NODES[name]
    for _, position in node["positions"]:
        link(system, name, PARTICLE_UPDATE, position, "Float", "Particles.RibbonLinkOrder")
    # The beam's own normal, so the push follows the corners round instead of pointing at a fixed compass.
    link(system, name, PARTICLE_UPDATE, node["push"], "Vector", "Particles.BeamSplineNormal")


def set_values(system, name):
    write(system, name, EMITTER_UPDATE, EMITTER_STATE, "Loop Duration", HELD)
    for input_name, value in (("Spawn Count", COUNT), ("Spawn Probability", 1.0), ("Spawn Time", 0),
                              ("Loop Count Limit", 0)):
        write(system, name, EMITTER_UPDATE, BURST, input_name, value)
    write(system, name, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Lifetime", HELD)
    write(system, name, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Color", COLOR)
    write(system, name, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Uniform Sprite Size", SIZE)
    write(system, name, PARTICLE_UPDATE, NODES[name]["spin"], "Rotation Rate", SPIN)
    set_corner_values(system, name)
    set_wave_values(system, name)


def wire_names(name, edge):
    """The names a fresh build hands out, so tuning needs no rebuild.

    Each node gets a unique function name in creation order, and that order is fixed by the phases below:
    the start corner before the end corner, its cosine half before its sine half, X before Y, and swell,
    glow, push in that order.
    """
    axes = corner_axes(edge)
    node = NODES[name] = {"beam": "BeamEmitterSetup", "place": "SpawnBeam", "follow": "UpdateBeam",
                          "bulge": "OffsetPosition", "swell": "ScaleSpriteSize", "glow": "ScaleColor001",
                          "spin": "SpriteRotationRate", "push": "Multiply_VectorByFloat", "edge": edge,
                          "axes": axes}
    node["vectors"] = ["MakeVector%s" % suffix for suffix in ("", "001", "002", "003")]
    node["trig"] = [("%s%s" % ("Cosine" if script == DYN_COSINE else "Sine", suffix),
                     script, component, axes[corner])
                    for script, suffix, component, corner in
                    ((DYN_COSINE, "", "X", 0), (DYN_COSINE, "001", "Y", 0),
                     (DYN_SINE, "", "X", 0), (DYN_SINE, "001", "Y", 0),
                     (DYN_COSINE, "002", "X", 1), (DYN_COSINE, "003", "Y", 1),
                     (DYN_SINE, "002", "X", 1), (DYN_SINE, "003", "Y", 1))]
    node["waves"] = [("swell", "Waveform", SWELL), ("glow", "Waveform001", GLOW),
                     ("push", "Waveform002", BULGE)]
    node["positions"] = [(wave, "ScaleAndBiasFloat%s" % suffix)
                         for (_, wave, _), suffix in zip(node["waves"], ("", "001", "002"))]


def retune(system=TARGET):
    """Rewrites every value on an already built beat — the tuning loop, with no module added.

    Constants only. A rapid-iteration constant is overwritten as often as you like; a link or a nested
    dynamic input is not, and re-issuing one crashes the editor outright, so neither appears below.
    """
    for edge, name in enumerate(names()):
        wire_names(name, edge)
        set_values(system, name)
    record("compile retune", builder().compile_and_save(system))
    return "\n".join(LOG)


def main(system=TARGET):
    """Each phase runs before the compile that makes the nodes it added addressable by value."""
    cdo = builder()
    for name in names():
        record("emitter %s" % name,
               cdo.add_emitter(system, "%s/%s.%s" % (LAYER_SOURCE_FOLDER, name, name)))
    record("compile emitters", cdo.compile_and_save(system))

    for name in names():
        add_modules(system, name)
    record("compile modules", cdo.compile_and_save(system))

    for phase, per_edge in ((build_corners, True),
                            (build_corner_sums, False), (build_corner_vectors, False),
                            (build_corner_trig, False),
                            (build_waves, False), (build_wave_inputs, True),
                            (build_wave_positions, False), (build_links, False)):
        for edge, name in enumerate(names()):
            if per_edge:
                phase(system, name, edge)
            else:
                phase(system, name)
        record("compile %s" % phase.__name__, cdo.compile_and_save(system))

    for name in names():
        set_values(system, name)
    record("compile values", cdo.compile_and_save(system))
    unreal.EditorAssetLibrary.delete_directory(LAYER_SOURCE_FOLDER)
    return "\n".join(LOG)


if __name__ == "__main__":
    report = "%sGeoTrinity_ChargedHaloBeat.txt" % unreal.Paths.project_saved_dir()
    try:
        open(report, "w").write(main())
    except Exception:
        open(report, "w").write("\n".join(LOG) + "\nFAILED\n" + traceback.format_exc())
