"""Create MF_DurationWipe — the clock-wipe mask every material showing a remaining-life readout shares.

  uvn  = (UV - 0.5) * 2                              quad space, [-1,1] on both axes
  t    = frac(atan2(uvn.g, uvn.r) / 2pi + 0.25)      0 at 12 o'clock, rising clockwise
  Wipe = Step(t, 1 - Spent)                          1 on the wedge that is still alive

The +0.25 turn puts t = 0 at 12 o'clock and the mask then shrinks clockwise, so half a life left
reads as the right half of the disc lit. UV.y grows DOWNWARD in Unreal, which is what makes
atan2(y, x) run clockwise on screen rather than counter-clockwise; a graph built on maths-convention
UVs would sweep the wrong way.

Spent is the fraction CONSUMED, not the fraction left, and the inversion is the point: this is fed
by custom primitive data, and a primitive nobody ever writes to reads 0. Phrased as "remaining" that
zero would mean empty, so every mesh sharing the material without a writer — MI_PulseCircle's sibling
instance on BP_DamageZone, sprite and preview uses — would silently render as a fully drained zone.
Phrased as "spent", the same zero means untouched, and an unwritten primitive keeps the full disc.

Step gives a hard edge on purpose, matching the rest of the zone-indicator family: the wedge boundary
is one pixel wide at any radius, where a smoothstep would blur wide near the rim and stay sharp at
the center.
"""
import unreal

PKG_PATH = "/Game/VFX/Generic/Materials/Functions"
NAME = "MF_DurationWipe"
FULL = f"{PKG_PATH}/{NAME}.{NAME}"

mel = unreal.MaterialEditingLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()

# Rebuilt in place, never deleted and recreated: any material already calling the function would turn
# a delete into a "still referenced / force delete" modal, which blocks the game thread with nobody to
# answer it.
if unreal.EditorAssetLibrary.does_asset_exist(f"{PKG_PATH}/{NAME}"):
    func = unreal.EditorAssetLibrary.load_asset(f"{PKG_PATH}/{NAME}")
    # UMaterialEditingLibrary::DeleteAllMaterialExpressionsInFunction range-iterates the very array
    # each delete removes from, so one call drops only every other node. Repeat until it is empty.
    while mel.get_num_material_expressions_in_function(func):
        before = mel.get_num_material_expressions_in_function(func)
        mel.delete_all_material_expressions_in_function(func)
        assert mel.get_num_material_expressions_in_function(func) < before, "function graph would not clear"
else:
    func = at.create_asset(NAME, PKG_PATH, unreal.MaterialFunction, unreal.MaterialFunctionFactoryNew())

func.set_editor_property("expose_to_library", True)


def expr(cls, x, y):
    return mel.create_material_expression_in_function(func, cls, x, y)


def connect(source, target, target_input):
    # A wrong pin name is not an error, it is a no-op that leaves the input on its default — and a
    # defaulted Step input still compiles, into a wedge that is simply always full. Never call the
    # library's connect directly.
    assert mel.connect_material_expressions(source, "", target, target_input), (
        f"{type(source).__name__} -> {type(target).__name__}.{target_input}")


def multiply(a, b_const, x, y):
    node = expr(unreal.MaterialExpressionMultiply, x, y)
    connect(a, node, "A")
    node.set_editor_property("const_b", b_const)
    return node


# --- inputs ---
uv_input = expr(unreal.MaterialExpressionFunctionInput, -1400, -100)
uv_input.set_editor_property("input_name", "UV")
uv_input.set_editor_property("input_type", unreal.FunctionInputType.FUNCTION_INPUT_VECTOR2)
uv_input.set_editor_property("sort_priority", 0)

spent_input = expr(unreal.MaterialExpressionFunctionInput, -1400, 220)
spent_input.set_editor_property("input_name", "Spent")
spent_input.set_editor_property("input_type", unreal.FunctionInputType.FUNCTION_INPUT_SCALAR)
spent_input.set_editor_property("sort_priority", 1)

# --- uvn: quad space, [-1,1] on both axes ---
centered = expr(unreal.MaterialExpressionSubtract, -1180, -100)
centered.set_editor_property("const_b", 0.5)
connect(uv_input, centered, "A")

uvn = multiply(centered, 2.0, -1000, -100)

axis_x = expr(unreal.MaterialExpressionComponentMask, -820, -180)
axis_x.set_editor_property("r", True)
axis_x.set_editor_property("g", False)
axis_x.set_editor_property("b", False)
axis_x.set_editor_property("a", False)
connect(uvn, axis_x, "")

axis_y = expr(unreal.MaterialExpressionComponentMask, -820, -20)
axis_y.set_editor_property("r", False)
axis_y.set_editor_property("g", True)
axis_y.set_editor_property("b", False)
axis_y.set_editor_property("a", False)
connect(uvn, axis_y, "")

# --- t: turn around the disc, 0 at 12 o'clock, rising clockwise ---
angle = expr(unreal.MaterialExpressionArctangent2, -600, -100)
connect(axis_y, angle, "Y")
connect(axis_x, angle, "X")

quarter_turn = expr(unreal.MaterialExpressionAdd, -240, -100)
quarter_turn.set_editor_property("const_b", 0.25)
connect(multiply(angle, 0.15915494, -420, -100), quarter_turn, "A")

# atan2 spans [-pi, pi], so the quarter turn leaves [-0.25, 0.75] — Frac wraps the negative arc
# (9 o'clock round to 12) back onto the top of the range instead of clipping it away.
turn = expr(unreal.MaterialExpressionFrac, -60, -100)
connect(quarter_turn, turn, "")

# --- Wipe = Step(t, 1 - Spent) ---
alive = expr(unreal.MaterialExpressionOneMinus, -240, 220)
connect(spent_input, alive, "")

# Step(Y, X) is 1 where X >= Y, so this lights every turn up to the surviving fraction.
wipe = expr(unreal.MaterialExpressionStep, 140, 0)
connect(turn, wipe, "Y")
connect(alive, wipe, "X")

output = expr(unreal.MaterialExpressionFunctionOutput, 380, 0)
output.set_editor_property("output_name", "Wipe")
connect(wipe, output, "")

mel.update_material_function(func)
unreal.EditorAssetLibrary.save_asset(f"{PKG_PATH}/{NAME}")

unreal.log(f"DURATIONWIPE::built {FULL}")
