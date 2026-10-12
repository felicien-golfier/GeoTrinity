"""Plain-Python checks of the sparse-key reduction in anim_sequence_authoring.py; no editor needed.

Run: python AI/Python/Anim/test_sparse_keys.py   (a stub `unreal` stands in for the editor module)
"""
import math
import os
import sys
import types

stub = types.ModuleType("unreal")
for name in ("AnimPoseExtensions", "Paths"):
    setattr(stub, name, types.SimpleNamespace(project_dir=lambda: "C:/GeoTrinity/"))
sys.modules["unreal"] = stub
toolkit = {}
path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "anim_sequence_authoring.py")
exec(compile(open(path, encoding="utf-8").read(), path, "exec"), toolkit)
reduce_channel, padded_frames, unwrapped = toolkit["reduce_channel"], toolkit["padded_frames"], toolkit["unwrapped"]


def worst_error(values, keys):
    return max(abs(values[f] - (values[a] + (values[b] - values[a]) * (f - a) / float(b - a)))
               for a, b in zip(keys, keys[1:]) for f in range(a, b + 1))


def check(name, condition):
    print(("ok   " if condition else "FAIL ") + name)
    if not condition:
        sys.exit(1)


hold = [2.0] * 40
check("hold has no keys", reduce_channel(hold, 0.01) == [])

ramp = [i * 0.5 for i in range(61)]
check("ramp keeps its two ends", reduce_channel(ramp, 0.01) == [0, 60])

step = [0.0] * 20 + [10.0] * 20
keys = reduce_channel(step, 0.01)
check("step keeps its corner", worst_error(step, keys) <= 0.01 and len(keys) <= 4)

ease = [10.0 * (a * a * (3 - 2 * a)) for a in (i / 59.0 for i in range(60))]
keys = reduce_channel(ease, 0.01)
check("ease is reduced within tolerance", worst_error(ease, keys) <= 0.01 and len(keys) < 40)

vibration = [(-1) ** i * 2.0 for i in range(30)]
check("per-frame vibration keeps every frame", reduce_channel(vibration, 0.01) == list(range(30)))

spin = [(i * 24.0 + 180.0) % 360.0 - 180.0 for i in range(46)]  # three turns, wrapped to +-180
turned = unwrapped(spin)
check("a spin unwraps to a continuous run", all(abs(b - a) < 180.0 for a, b in zip(turned, turned[1:])))
check("a spin is one run of two keys", reduce_channel(turned, 0.05) == [0, 45])

taken = {2: (0, 60), 3: (0, 20, 60)}
check("a channel on the claimed frames is left alone", padded_frames([0, 20, 60], 61, taken) == [0, 20, 60])
padded = padded_frames([0, 10, 60], 61, taken)
check("a channel of a claimed count on other frames gains keys until its count is free",
      len(padded) == 4 and {0, 10, 60} <= set(padded))
check("padding never collides with a claimed count", len(padded) not in taken)
check("a channel keyed on every frame stays dense", padded_frames(list(range(61)), 61, {61: (0, 5)}) == list(range(61)))
