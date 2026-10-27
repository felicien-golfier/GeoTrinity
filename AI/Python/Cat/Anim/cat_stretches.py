"""Stretch clips of the Cat, lying on the floor so the top camera reads them: Cat_Stretch_Reach, _Sprawl, _Flat, _Twist.

Rigid motion: only the Body stretches along its own X, the parts round it keep their size. Each clip starts and ends on
Cat_Idle frame 0 (cat_posing.py): eased in, held with a fine shake through the full stretch, eased out. Height is
invisible from above, so every stretch is a change of length, direction or spread on the floor, never a rise.

Run via mcp-unreal execute_script. Re-runnable. Reports: AI/Output/cat_Cat_Stretch_<name>.json.
"""
import unreal

_POSING_PATH = unreal.Paths.project_dir() + "AI/Python/Cat/Anim/cat_posing.py"
POSING = {}
exec(compile(open(_POSING_PATH, encoding="utf-8").read(), _POSING_PATH, "exec"), POSING)
sit_spec, run_clips, paws, shake_between, twist_spec, FACE_PEEK_DEGREES = (POSING["sit_spec"], POSING["run_clips"], POSING["paws"],
                                                POSING["shake_between"], POSING["twist_spec"], POSING["FACE_PEEK_DEGREES"])

FRAMES = 120  # 4 s
EASE_IN, EASE_OUT = 36, 84


def shake(frame):
    return shake_between(frame, EASE_IN, EASE_OUT)


def reach(frame, weight):
    """Lying on its side, long, forelegs reaching past the head."""
    tremor = shake(frame)
    return sit_spec(
        body=((10, 0, -26), (80, 0, 8 + 0.7 * tremor), 1.45), head=((40, 6, -34), (70, FACE_PEEK_DEGREES, 8 + 0.5 * tremor)),
        paws=paws((62, 22, -26), (62, -22, -26), (-20, -6, -22), (-20, 6, -22), tremor, yaw=8.0),
        tail0=(0, -3, 8), tail=lambda i: (0.0, 12.0 + 0.4 * i * tremor), eyes=(0.1, 0.1))


def sprawl(frame, weight):
    """On its back, limp, paws flopped up and out, head turned back."""
    tremor = shake(frame)
    return sit_spec(
        body=((4, 0, -24), (140, 0, 0.7 * tremor), 1.3), head=((14, 0, -22), (-25, 65, -35 + 0.5 * tremor)),
        paws=paws((30, -18, 8), (30, 18, 8), (-10, -14, 12), (-10, 14, 12), tremor),
        tail0=(0, -8, -25), tail=lambda i: (0.0, 11.0 + 0.4 * i * tremor), eyes=(0.3, 0.3))


def flat(frame, weight):
    """Belly down, hands far ahead, feet pushed back, the body long and the tail laid out straight behind."""
    tremor = shake(frame)
    return sit_spec(
        body=((8, 0, -20), (0, 0, 0.7 * tremor), 1.45), head=((34, 0, -30), (0, FACE_PEEK_DEGREES, 0.5 * tremor)),
        paws=paws((66, 12, -18), (66, -12, -18), (-26, 0, -16), (-26, 0, -16), tremor),
        tail0=(0, 0, 0), tail=lambda i: (0.0, 8.0 + 0.4 * i * tremor), eyes=(0.25, 0.25))


def twist(frame, weight):
    return twist_spec(shake(frame))


if __name__ == "__main__":
    run_clips([("Cat_Stretch_" + name, FRAMES, EASE_IN, EASE_OUT, target)
               for name, target in (("Reach", reach), ("Sprawl", sprawl), ("Flat", flat), ("Twist", twist))])
