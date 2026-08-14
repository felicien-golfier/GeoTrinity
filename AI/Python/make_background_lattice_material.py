"""Create MPC_BackgroundPulse + M_BackgroundLattice — the floor's triangle line-art, pulsed from C++.

Replaces FloorMat, which was Opaque/DefaultLit with a literally empty graph. Masked and Default Lit:
only the lattice lines survive the clip, everything between them shows whatever is behind the floor.

Per pixel:
  p       = AbsoluteWorldPosition.xy      -> NOT TexCoord, see below
  h       = ShapeSize * sqrt(3)/2         -> row spacing of the lattice
  f_i     = abs(frac(dot(p, n_i)/h + 0.5) - 0.5)      for n_i in the three edge normals
  fmin    = min(f_0, f_1, f_2)            -> distance to the nearest edge, in units of h
  Line    = Step(fmin, LineThickness / (ShapeSize * sqrt(3)))
  Pulse   = max_i saturate(1 - abs(length(p - Src_i.xy) - Src_i.z) / RingWidth) * Src_i.w

  BaseColor   = LineColor
  Emissive    = PulseColor * Pulse * PulseBrightness
  OpacityMask = Line

THE LATTICE IS MADE OF THE SHAPE IT DRAWS. Three families of evenly spaced parallel lines at 60
degrees to each other cut the plane into equilateral triangles and nothing else, so one Step over
the nearest-edge distance draws every triangle at once — there is no per-cell shape to instance and
no cell index to compute. The three normals are (0,1), (-sqrt(3)/2, 1/2) and (sqrt(3)/2, 1/2), and
all three families are phase-0: for lattice points V = S*(a,0) + S*(b/2, b*sqrt(3)/2) the three dot
products come out to -a*h, (a+b)*h and b*h, all exact multiples of h. That alignment is the whole
trick. Give any one family a phase offset and the lines stop meeting at shared vertices — the
tiling turns into the kagome mix of small triangles and hexagons, which is a different pattern that
happens to be one constant away.

WORLD POSITION, NEVER TexCoord. The nine Floor_C actors in DraftMap carry non-uniform scales
(1.4x2.8 up to 4.33x3.99) and rotations (-45, -90), so a UV-space lattice would draw a different
triangle size on every floor piece and break at each seam. Reading absolute world position instead
makes the lattice one continuous sheet across all of them for free, and puts it in the same space
as the pulse origins the MPC carries, so no transform is needed on either side. WPT_Default is
already absolute world position; it is left at the default rather than set, unlike M_PulseCircle's
ObjectPositionWS which must be pinned away from Camera Relative.

LineThickness and ShapeSize are both in world centimetres and independent of each other: ShapeSize
is the triangle's side length, LineThickness the full drawn width of a line (hence the /2 folded
into the threshold's sqrt(3) — the raw comparison is against distance to the line's centre, so
without it the parameter would draw twice as wide as it reads).

PULSES COME FROM THE MPC, NOT FROM ACTORS. The material knows nothing about who is pulsing: each of
the eight PulseSource_XX vectors is (OriginX, OriginY, Radius, Intensity), and whatever fills them
owns the animation entirely — following a character, sweeping a boss telegraph, or running a
scripted pattern with no actor behind it at all. Radius is the ring's current distance from its
origin, so a travelling wave is C++ raising Radius over time, not the shader deriving it from Time.
That is the reason the shader has no Time node: a pattern the material times itself cannot be
started, stopped or synchronised by gameplay.

Intensity in .w is a SELF-CANCELLING SENTINEL: an unused slot is all zeroes, which contributes
exactly nothing to the max, so there is no magic coordinate to keep in sync between the shader and
the writer. MPC_MaskedArea needs (-10000,-10000,-10000,0) for its pillar slots precisely because it
has no such multiplier, and UDevastatingWavePattern::ClearData exists to write that constant back.
Nothing here needs a matching Clear: zeroing is the neutral value.

The MPC is global state shared by every material that reads it, so this collection is its own asset
rather than more slots on MPC_MaskedArea — the devastating wave clears all eight of those between
activations and would blank the background with them.

Also creates MI_BackgroundLattice: use THAT one on the floor. A base material's parameter defaults
cannot be tuned from the Details panel, an instance's can.

Run outside PIE — creating expressions fails while a session is running.
"""
import unreal

PKG_PATH = "/Game/VFX/Generic/Materials"
NAME = "M_BackgroundLattice"
INSTANCE_NAME = "MI_BackgroundLattice"
MPC_NAME = "MPC_BackgroundPulse"
SLOT_COUNT = 8
SLOT_FORMAT = "PulseSource_{:02d}"

SQRT3_OVER_2 = 0.8660254037844386
SQRT3 = 1.7320508075688772

mel = unreal.MaterialEditingLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
ALWAYS = unreal.PropertyAccessChangeNotifyMode.ALWAYS


def save(asset):
    """Write asset to disk, or fail loudly.

    EditorAssetLibrary.save_asset defaults to only_if_is_dirty and reports the outcome only through a return
    value, so a freshly created asset whose package never got flagged dirty writes NOTHING and says nothing.
    That is not hypothetical: the first run of this script left MPC_BackgroundPulse and MI_BackgroundLattice
    unsaved while the material went through (recompile_material had dirtied that one), and the material then
    reloaded with all eight of its collection nodes pointing at an asset that did not exist. Forcing the write
    and asserting on it is what makes a missing asset a failed run instead of a silently broken material.
    """
    assert unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False), \
        f"{asset.get_path_name()}: would not save"

# --- MPC_BackgroundPulse -------------------------------------------------------------------------
# Rebuilt every run: the slots carry no tuning worth preserving (all zero is the neutral value), and
# the writer side is generated from the same SLOT_COUNT/SLOT_FORMAT constants above.
if unreal.EditorAssetLibrary.does_asset_exist(f"{PKG_PATH}/{MPC_NAME}"):
    collection = unreal.EditorAssetLibrary.load_asset(f"{PKG_PATH}/{MPC_NAME}")
else:
    collection = at.create_asset(MPC_NAME, PKG_PATH, unreal.MaterialParameterCollection,
                                 unreal.MaterialParameterCollectionFactoryNew())

slots = []
for index in range(SLOT_COUNT):
    slot = unreal.CollectionVectorParameter()
    slot.set_editor_property("parameter_name", SLOT_FORMAT.format(index))
    slot.set_editor_property("default_value", unreal.LinearColor(0.0, 0.0, 0.0, 0.0))
    slots.append(slot)

# Notify forced on: the collection assigns each parameter its Id in the post-change handler, and an
# entry that never gets one is invisible to every CollectionParameter node that names it.
collection.set_editor_property("vector_parameters", slots, ALWAYS)

registered = [str(n) for n in collection.get_vector_parameter_names()]
assert registered == [SLOT_FORMAT.format(i) for i in range(SLOT_COUNT)], \
    f"{MPC_NAME}: parameters would not register, got {registered}"
save(collection)
# The material is about to take hard references to this collection, so it has to be on disk before that — a
# reference to an unsaved package resolves to null on the next load and takes every pulse slot with it.
assert unreal.EditorAssetLibrary.does_asset_exist(f"{PKG_PATH}/{MPC_NAME}"), \
    f"{MPC_NAME}: saved but is not on disk"

# --- M_BackgroundLattice -------------------------------------------------------------------------
# Rebuilt in place, never deleted and recreated: the floor actors reference it, and a delete on a
# referenced asset opens a "still referenced / force delete" modal that blocks the game thread with
# nobody there to answer it.
if unreal.EditorAssetLibrary.does_asset_exist(f"{PKG_PATH}/{NAME}"):
    mat = unreal.EditorAssetLibrary.load_asset(f"{PKG_PATH}/{NAME}")
    # UMaterialEditingLibrary::DeleteAllMaterialExpressions range-iterates the very array each
    # delete removes from, so one call drops only every other node. Repeat until it is really empty.
    while mel.get_num_material_expressions(mat):
        before = mel.get_num_material_expressions(mat)
        mel.delete_all_material_expressions(mat)
        assert mel.get_num_material_expressions(mat) < before, "material graph would not clear"
else:
    mat = at.create_asset(NAME, PKG_PATH, unreal.Material, unreal.MaterialFactoryNew())


def expr(cls, x, y):
    return mel.create_material_expression(mat, cls, x, y)


def connect(source, target, target_input):
    # A wrong pin name is not an error, it is a no-op that leaves the input on its default — and a
    # defaulted Step input still compiles, into a lattice that is simply the wrong size. Never call
    # the library's connect directly.
    assert mel.connect_material_expressions(source, "", target, target_input), (
        f"{type(source).__name__} -> {type(target).__name__}.{target_input}")


def binary(cls, a, b, x, y, first="A", second="B"):
    node = expr(cls, x, y)
    connect(a, node, first)
    connect(b, node, second)
    return node


def unary(cls, source, x, y):
    node = expr(cls, x, y)
    connect(source, node, "")
    return node


def scalar(name, value, x, y):
    node = expr(unreal.MaterialExpressionScalarParameter, x, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", value)
    return node


def color(name, value, x, y):
    node = expr(unreal.MaterialExpressionVectorParameter, x, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", value)
    return node


def mask(source, x, y, r=False, g=False, b=False, a=False):
    node = expr(unreal.MaterialExpressionComponentMask, x, y)
    connect(source, node, "")
    node.set_editor_property("r", r)
    node.set_editor_property("g", g)
    node.set_editor_property("b", b)
    node.set_editor_property("a", a)
    return node


def const2(x_value, y_value, x, y):
    node = expr(unreal.MaterialExpressionConstant2Vector, x, y)
    node.set_editor_property("r", x_value)
    node.set_editor_property("g", y_value)
    return node


def times(a, b, x, y):
    return binary(unreal.MaterialExpressionMultiply, a, b, x, y)


def scaled(source, factor, x, y):
    node = expr(unreal.MaterialExpressionMultiply, x, y)
    connect(source, node, "A")
    node.set_editor_property("const_b", factor)
    return node


# --- p = AbsoluteWorldPosition.xy ---
world_position = expr(unreal.MaterialExpressionWorldPosition, -2600, -200)
p = mask(world_position, -2400, -200, r=True, g=True)

shape_size = scalar("ShapeSize", 200.0, -2600, 100)
line_thickness = scalar("LineThickness", 8.0, -2600, 260)

# --- the three edge families ---
row_height = scaled(shape_size, SQRT3_OVER_2, -2340, 100)

nearest = None
for slot, (normal_x, normal_y) in enumerate(((0.0, 1.0), (-SQRT3_OVER_2, 0.5), (SQRT3_OVER_2, 0.5))):
    row = -600 + slot * 240
    normal = const2(normal_x, normal_y, -2200, row + 80)
    projected = binary(unreal.MaterialExpressionDotProduct, p, normal, -2000, row)
    steps = binary(unreal.MaterialExpressionDivide, projected, row_height, -1840, row)

    # +0.5 before frac and -0.5 after folds the sawtooth into a symmetric triangle wave, so the
    # result is distance to the NEAREST line of the family rather than distance since the last one.
    shifted = expr(unreal.MaterialExpressionAdd, -1680, row)
    shifted.set_editor_property("const_b", 0.5)
    connect(steps, shifted, "A")

    wrapped = unary(unreal.MaterialExpressionFrac, shifted, -1520, row)

    centred = expr(unreal.MaterialExpressionSubtract, -1360, row)
    centred.set_editor_property("const_b", 0.5)
    connect(wrapped, centred, "A")

    distance = unary(unreal.MaterialExpressionAbs, centred, -1200, row)
    nearest = distance if nearest is None \
        else binary(unreal.MaterialExpressionMin, nearest, distance, -1040, row - 120)

# fmin is measured in units of row height, so the thickness has to be too. The extra factor of 2
# (sqrt(3) rather than sqrt(3)/2) is the half-width conversion: fmin is distance to the line's
# centre, so LineThickness reads as the full drawn width.
threshold_scale = scaled(shape_size, SQRT3, -2340, 260)
threshold = binary(unreal.MaterialExpressionDivide, line_thickness, threshold_scale, -2140, 260)

# Step(Y, X) is 1 where X >= Y — here, wherever the nearest edge is within half a line width.
line = binary(unreal.MaterialExpressionStep, nearest, threshold, -820, -300, first="Y", second="X")

# --- Pulse = max over the eight MPC slots ---
ring_width = scalar("RingWidth", 200.0, -2600, 420)

pulse = None
for index in range(SLOT_COUNT):
    row = 700 + index * 260
    source = expr(unreal.MaterialExpressionCollectionParameter, -2400, row)
    source.set_editor_property("collection", collection, ALWAYS)
    # The node resolves its parameter id in the post-change notification; assigned quietly it stays
    # unresolved and compiles to zero, which reads as "this slot never pulses" and nothing warns.
    # ParameterId itself is not reflected to Python, so the two inputs that notification reads are
    # what gets checked here — an id derived from both of them holding cannot be the stale one.
    source.set_editor_property("parameter_name", SLOT_FORMAT.format(index), ALWAYS)
    assert source.get_editor_property("collection") == collection \
        and str(source.get_editor_property("parameter_name")) == SLOT_FORMAT.format(index), \
        f"{SLOT_FORMAT.format(index)}: collection parameter node would not take its binding"

    origin = mask(source, -2180, row, r=True, g=True)
    radius = mask(source, -2180, row + 80, b=True)
    intensity = mask(source, -2180, row + 160, a=True)

    offset = binary(unreal.MaterialExpressionSubtract, p, origin, -1980, row)
    distance = unary(unreal.MaterialExpressionLength, offset, -1820, row)

    from_ring = binary(unreal.MaterialExpressionSubtract, distance, radius, -1660, row)
    depth = unary(unreal.MaterialExpressionAbs, from_ring, -1500, row)
    normalised = binary(unreal.MaterialExpressionDivide, depth, ring_width, -1340, row)
    falloff = unary(unreal.MaterialExpressionOneMinus, normalised, -1180, row)
    clamped = unary(unreal.MaterialExpressionSaturate, falloff, -1020, row)

    ring = times(clamped, intensity, -860, row)
    pulse = ring if pulse is None \
        else binary(unreal.MaterialExpressionMax, pulse, ring, -700, row - 130)

# --- outputs ---
line_color = color("LineColor", unreal.LinearColor(0.04, 0.05, 0.08, 1.0), -400, -520)
pulse_color = color("PulseColor", unreal.LinearColor(0.15, 0.6, 1.0, 1.0), -400, 400)
pulse_brightness = scalar("PulseBrightness", 4.0, -400, 560)

emissive = times(pulse_color, times(pulse, pulse_brightness, -200, 460), 0, 420)

mel.connect_material_property(line_color, "", unreal.MaterialProperty.MP_BASE_COLOR)
mel.connect_material_property(emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.connect_material_property(line, "", unreal.MaterialProperty.MP_OPACITY_MASK)

# --- settings (after wiring) ---
mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)

mel.recompile_material(mat)
save(mat)

# Left alone on a rerun so tuned values live.
if not unreal.EditorAssetLibrary.does_asset_exist(f"{PKG_PATH}/{INSTANCE_NAME}"):
    instance = at.create_asset(INSTANCE_NAME, PKG_PATH, unreal.MaterialInstanceConstant,
                               unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(instance, mat)
    save(instance)

unreal.log(f"BGLATTICE::built {PKG_PATH}/{NAME} + {INSTANCE_NAME} + {MPC_NAME}")
