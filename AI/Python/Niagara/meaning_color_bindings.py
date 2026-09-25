"""Every system drawing meaning colours hands them to its material: User.Color, User.Color2-4 and User.ColorCount,
bound to the Color, Color2-4 and ColorCount parameters of M_PulseBeam, M_ZoneIndicatorRay and M_ZoneIndicator
(AI/Python/Material/make_beam_and_telegraph_materials.py). The User names are GeoNiagaraParams::MeaningColors and
ColorCount, which UGeoBeamVFXComponent, UBeamPattern and GeoASLib::SetCueMeaningColors write. Re-runnable: an existing
parameter or binding is kept or replaced.

Run through MCP execute_script; the report goes to Saved/meaning_color_bindings.txt."""
import traceback

import unreal

# (system, the emitter whose renderer draws the material)
SYSTEMS = [
    ("/Game/Art/VFX/Generic/Niagara/NS_Beam", "Minimal"),
    ("/Game/Art/VFX/Generic/Niagara/NS_Ray_ZoneIndicator", "Minimal"),
    ("/Game/Art/VFX/Generic/Niagara/NS_Round_ZoneIndicator", "Explosion"),
]
# (User parameter, its type, its default, the material parameter it feeds)
BINDINGS = [
    ("User.Color", None, None, "Color"),
    ("User.Color2", "LinearColor", "1,1,1,1", "Color2"),
    ("User.Color3", "LinearColor", "1,1,1,1", "Color3"),
    ("User.Color4", "LinearColor", "1,1,1,1", "Color4"),
    # 1 by default: the colour pattern cycles through ColorCount colours, and none is a division by zero.
    ("User.ColorCount", "Float", "1", "ColorCount"),
]
REPORT = unreal.Paths.project_saved_dir() + "meaning_color_bindings.txt"

out = []
try:
    builder = unreal.GeoNiagaraBuilderUtil.get_default_object()
    for system, emitter in SYSTEMS:
        for user_name, type_name, default, material_name in BINDINGS:
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
