"""User parameters the beam and telegraph systems hand to their material, bound to the material parameters of the same
name (AI/Python/Material/make_beam_and_telegraph_materials.py):
- every system drawing meaning colours: User.Color, User.Color2-4 and User.ColorCount -> Color, Color2-4, ColorCount of
  M_PulseBeam, M_ZoneIndicatorRay and M_ZoneIndicator;
- the two beam systems: User.SafeSpotCut -> SafeSpotCut, 1 only on a hazard that owns safe spots (ULateralWavePattern).
The User names are GeoNiagaraParams::MeaningColors, ColorCount and SafeSpotCut. Re-runnable: an existing parameter or
binding is kept or replaced.

Run through MCP execute_script; the report goes to AI/Output/material_user_bindings.txt."""
import traceback

import unreal

# (User parameter, its type, its default, the material parameter it feeds)
MEANING_COLORS = [
    ("User.Color", None, None, "Color"),
    ("User.Color2", "LinearColor", "1,1,1,1", "Color2"),
    ("User.Color3", "LinearColor", "1,1,1,1", "Color3"),
    ("User.Color4", "LinearColor", "1,1,1,1", "Color4"),
    # 1 by default: the colour pattern cycles through ColorCount colours, and none is a division by zero.
    ("User.ColorCount", "Float", "1", "ColorCount"),
]
# 0 by default: an effect ignores the safe spot mask unless its hazard opts in.
SAFE_SPOT_CUT = [("User.SafeSpotCut", "Float", "0", "SafeSpotCut")]
# (system, the emitter whose renderer draws the material, its bindings)
SYSTEMS = [
    ("/Game/Art/VFX/Generic/Niagara/NS_Beam", "Minimal", MEANING_COLORS + SAFE_SPOT_CUT),
    ("/Game/Art/VFX/Generic/Niagara/NS_Ray_ZoneIndicator", "Minimal", MEANING_COLORS + SAFE_SPOT_CUT),
    ("/Game/Art/VFX/Generic/Niagara/NS_Round_ZoneIndicator", "Explosion", MEANING_COLORS),
]
REPORT = (unreal.Paths.project_dir() + "AI/Output/") + "material_user_bindings.txt"

out = []
try:
    builder = unreal.GeoNiagaraBuilderUtil.get_default_object()
    for system, emitter, bindings in SYSTEMS:
        for user_name, type_name, default, material_name in bindings:
            if type_name:
                assert builder.add_user_parameter(system, user_name, type_name), f"{system}: add {user_name}"
                assert builder.set_user_parameter(system, user_name, default), f"{system}: default {user_name}"

            assert builder.bind_material_parameter_to_user_parameter(system, emitter, material_name, user_name), \
                f"{system}: bind {material_name}"
            out.append(f"{system}: {material_name} <- {user_name}")

        assert builder.compile_and_save(system), f"{system}: compile and save"
        out.append(f"{system}: saved")
except Exception:
    out.append(traceback.format_exc())
open(REPORT, "w").write("\n".join(out))
