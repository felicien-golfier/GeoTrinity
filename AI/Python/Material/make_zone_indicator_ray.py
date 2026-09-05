"""Create M_ZoneIndicatorRay — the ray/bar variant of M_ZoneIndicator.

Fill grows horizontally from the center to BOTH side edges, driven by the
FillOpacity scalar (0 = nothing, 1 = full width). A thin boundary line frames
all four edges (left/right AND top/bottom) so the full extent is always
visible while the fill telegraphs outward.

Mirrors M_ZoneIndicator settings: Unlit, Additive, Two-Sided.

Geometry (per pixel):
  u    = TexCoord.x ; v = TexCoord.y
  dX   = abs((u - 0.5) * 2)                -> 0 at center, 1 at left/right edges
  dY   = abs((v - 0.5) * 2)                -> 0 at center, 1 at top/bottom edges
  Fill = Step(dX, FillOpacity)             -> 1 where dX <= FillOpacity
  Edge = max( Step(1 - LineThickness, dX),  -> left/right border
              Step(1 - LineThickness, dY) ) -> top/bottom border
  Emissive = FillColor*Fill + LineColor*Edge
  Opacity  = max(Fill, Edge)
"""
import unreal

PKG_PATH = "/Game/VFX/Generic/Materials"
# Optional override: define RAY_NAME_OVERRIDE before exec() to build a test
# asset under a different name without touching the shipped M_ZoneIndicatorRay.
NAME = globals().get("RAY_NAME_OVERRIDE", "M_ZoneIndicatorRay")
FULL = f"{PKG_PATH}/{NAME}.{NAME}"

mel = unreal.MaterialEditingLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()

# Clean recreate for idempotency
if unreal.EditorAssetLibrary.does_asset_exist(f"{PKG_PATH}/{NAME}"):
    unreal.EditorAssetLibrary.delete_asset(f"{PKG_PATH}/{NAME}")

mat = at.create_asset(NAME, PKG_PATH, unreal.Material, unreal.MaterialFactoryNew())

def expr(cls, x, y):
    return mel.create_material_expression(mat, cls, x, y)

# --- centered horizontal distance from center ---
tex = expr(unreal.MaterialExpressionTextureCoordinate, -1400, 0)
maskR = expr(unreal.MaterialExpressionComponentMask, -1200, 0)
maskR.set_editor_property("r", True)
maskR.set_editor_property("g", False)
maskR.set_editor_property("b", False)
maskR.set_editor_property("a", False)
mel.connect_material_expressions(tex, "", maskR, "")

sub = expr(unreal.MaterialExpressionSubtract, -1000, 0)
sub.set_editor_property("const_b", 0.5)
mel.connect_material_expressions(maskR, "", sub, "A")

mul2 = expr(unreal.MaterialExpressionMultiply, -820, 0)
mul2.set_editor_property("const_b", 2.0)
mel.connect_material_expressions(sub, "", mul2, "A")

absN = expr(unreal.MaterialExpressionAbs, -640, 0)
mel.connect_material_expressions(mul2, "", absN, "")   # dX in [0,1]

# --- centered vertical distance from center (for top/bottom border) ---
maskG = expr(unreal.MaterialExpressionComponentMask, -1200, -220)
maskG.set_editor_property("r", False)
maskG.set_editor_property("g", True)
maskG.set_editor_property("b", False)
maskG.set_editor_property("a", False)
mel.connect_material_expressions(tex, "", maskG, "")

subY = expr(unreal.MaterialExpressionSubtract, -1000, -220)
subY.set_editor_property("const_b", 0.5)
mel.connect_material_expressions(maskG, "", subY, "A")

mul2Y = expr(unreal.MaterialExpressionMultiply, -820, -220)
mul2Y.set_editor_property("const_b", 2.0)
mel.connect_material_expressions(subY, "", mul2Y, "A")

absNY = expr(unreal.MaterialExpressionAbs, -640, -220)
mel.connect_material_expressions(mul2Y, "", absNY, "")   # dY in [0,1]

# --- parameters ---
fillOp = expr(unreal.MaterialExpressionScalarParameter, -640, 220)
fillOp.set_editor_property("parameter_name", "FillOpacity")
fillOp.set_editor_property("default_value", 0.5)

lineTh = expr(unreal.MaterialExpressionScalarParameter, -1000, 460)
lineTh.set_editor_property("parameter_name", "LineThickness")
lineTh.set_editor_property("default_value", 0.03)

fillCol = expr(unreal.MaterialExpressionVectorParameter, -400, 520)
fillCol.set_editor_property("parameter_name", "FillColor")
fillCol.set_editor_property("default_value", unreal.LinearColor(1.0, 0.15, 0.1, 1.0))

lineCol = expr(unreal.MaterialExpressionVectorParameter, -400, 720)
lineCol.set_editor_property("parameter_name", "LineColor")
lineCol.set_editor_property("default_value", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))

# --- Fill = Step(dX, FillOpacity) : 1 where FillOpacity >= dX ---
fillStep = expr(unreal.MaterialExpressionStep, -400, 60)
mel.connect_material_expressions(absN, "", fillStep, "Y")     # Y = dX
mel.connect_material_expressions(fillOp, "", fillStep, "X")   # X = FillOpacity

# --- Edge = max( Step(1-thick, dX), Step(1-thick, dY) ) : 1 near any of the 4 edges ---
oneMinus = expr(unreal.MaterialExpressionOneMinus, -820, 460)
mel.connect_material_expressions(lineTh, "", oneMinus, "")

edgeStepX = expr(unreal.MaterialExpressionStep, -560, 300)
mel.connect_material_expressions(oneMinus, "", edgeStepX, "Y")  # Y = 1-thickness
mel.connect_material_expressions(absN, "", edgeStepX, "X")      # X = dX (left/right)

edgeStepY = expr(unreal.MaterialExpressionStep, -560, 460)
mel.connect_material_expressions(oneMinus, "", edgeStepY, "Y")  # Y = 1-thickness
mel.connect_material_expressions(absNY, "", edgeStepY, "X")     # X = dY (top/bottom)

edgeStep = expr(unreal.MaterialExpressionMax, -400, 380)
mel.connect_material_expressions(edgeStepX, "", edgeStep, "A")
mel.connect_material_expressions(edgeStepY, "", edgeStep, "B")

# --- Emissive = FillColor*Fill + LineColor*Edge ---
fillTint = expr(unreal.MaterialExpressionMultiply, -120, 240)
mel.connect_material_expressions(fillCol, "", fillTint, "A")
mel.connect_material_expressions(fillStep, "", fillTint, "B")

lineTint = expr(unreal.MaterialExpressionMultiply, -120, 460)
mel.connect_material_expressions(lineCol, "", lineTint, "A")
mel.connect_material_expressions(edgeStep, "", lineTint, "B")

emissive = expr(unreal.MaterialExpressionAdd, 120, 320)
mel.connect_material_expressions(fillTint, "", emissive, "A")
mel.connect_material_expressions(lineTint, "", emissive, "B")

# --- Opacity = max(Fill, Edge) ---
opac = expr(unreal.MaterialExpressionMax, 120, 60)
mel.connect_material_expressions(fillStep, "", opac, "A")
mel.connect_material_expressions(edgeStep, "", opac, "B")

# --- outputs ---
mel.connect_material_property(emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.connect_material_property(opac, "", unreal.MaterialProperty.MP_OPACITY)

# --- settings (after wiring) ---
mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
mat.set_editor_property("two_sided", True)

mel.recompile_material(mat)
unreal.EditorAssetLibrary.save_asset(f"{PKG_PATH}/{NAME}")
unreal.log(f"RAYMAT::created {FULL}")
