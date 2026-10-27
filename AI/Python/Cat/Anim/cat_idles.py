"""Idle variants of the Cat, to play in place of Cat_Idle now and then: Cat_Idle_Toilet, _Groom, _Loaf, _LookAround,
_Knead. Each starts and ends on Cat_Idle frame 0 (cat_posing.py), so any of them chains with the idle.

Rigid motion, the paws and the head placed in the world, nothing squashed. Seen from above, height is invisible: the
beats read as turning, paws travelling in X / Y, the head's yaw and pitch, the tail's length.

Run via mcp-unreal execute_script. Re-runnable. Writes the clips and a montage each, AM_Cat_Idle_<Name>. Reports:
AI/Output/cat_Cat_Idle_<name>.json.
"""
import math

import unreal

_POSING_PATH = unreal.Paths.project_dir() + "AI/Python/Cat/Anim/cat_posing.py"
POSING = {}
exec(compile(open(_POSING_PATH, encoding="utf-8").read(), _POSING_PATH, "exec"), POSING)
sit_spec, run_clips, SIT_HEAD, SIT_PAWS = POSING["sit_spec"], POSING["run_clips"], POSING["SIT_HEAD"], POSING["SIT_PAWS"]
curve, attack_release, EAR_DETACH = POSING["curve"], POSING["CLIP"]["attack_release"], POSING["CLIP"]["EAR_DETACH"]
CLIP, TOOLKIT = POSING["CLIP"], POSING["CLIP"]["TOOLKIT"]

FLAT = (0.0, 0.0, 0.0)
MONTAGE_BLEND = 0.15  # seconds in and out: each variant starts and ends on Cat_Idle's first pose


def wave(frame, start, period, amplitude, phase=0.0):
    """A sine that starts at `start` (0 before)."""
    return amplitude * math.sin(2.0 * math.pi * ((frame - start) / float(period) + phase)) if frame >= start else 0.0


def head_at(offset, pitch, yaw):
    """The sitting head moved by `offset` and turned to `pitch`, `yaw` in the world."""
    return (tuple(a + b for a, b in zip(SIT_HEAD[0], offset)), (0.0, pitch, yaw))


def hands(left, right):
    paws = dict(SIT_PAWS)
    paws["HandLeft"], paws["HandRight"] = (left, FLAT), (right, FLAT)
    return paws


def toilet(frame, weight):
    """Circles, sniffs, squats with the tail straight up and strains, digs with both hands, looks back."""
    circle = curve(frame, [(0, 0.0), (40, -24.0), (80, 24.0), (110, 0.0)])
    squat = curve(frame, [(110, 0.0), (140, 1.0)])
    strain = wave(frame, 140, 6, 0.8) * curve(frame, [(140, 0.0), (150, 1.0), (185, 1.0), (192, 0.0)])
    dig = curve(frame, [(190, 0.0), (196, 1.0), (240, 1.0), (250, 0.0)])
    glance = curve(frame, [(248, 0.0), (262, 1.0), (285, 1.0), (298, 0.0)])
    left_stroke = -14.0 + 14.0 * math.sin(2.0 * math.pi * (frame - 190) / 20.0)
    right_stroke = -14.0 + 14.0 * math.sin(2.0 * math.pi * (frame - 190) / 20.0 + math.pi)
    return sit_spec(
        body=((0, strain, -10 * squat), (0, 0, circle + 5.0 * dig * math.sin(frame / 4.0)), 1.0),
        head=head_at((0, 0, -4 * squat), 80.0 - 12.0 * squat, 40.0 * glance - 0.3 * circle),
        paws=hands((8 + dig * left_stroke, 4 * dig, 0), (8 + dig * right_stroke, -4 * dig, 0)),
        tail0=(0, -70 * squat, 0), tail=lambda i: (16.0 * squat if i >= 6 else 0.0, 8.0 * squat - 10.0 * (1 - squat)),
        eyes=(1.0 - 0.5 * squat + 0.4 * glance,) * 2)


def groom(frame, weight):
    """Turns a little, lifts the left hand to its mouth and licks it, then wipes it over the left ear, the ear folding
    each time the hand passes."""
    lift = curve(frame, [(20, 0.0), (50, 1.0), (240, 1.0), (265, 0.0)])
    lick = wave(frame, 60, 24, 5.0) * curve(frame, [(55, 0.0), (65, 1.0), (125, 1.0), (138, 0.0)])
    ear = curve(frame, [(130, 0.0), (155, 1.0), (225, 1.0), (245, 0.0)])
    stroke = 2.0 * math.pi * (frame - 150) / 26.0
    wipe = ear * math.sin(stroke)  # the hand's X, back at -1
    fold = ear * 0.5 * (1.0 - math.sin(stroke))  # the ear's, most when the hand is at the back
    mouth = (30.0, 44.0 + 0.6 * lick, 20.0)
    over_ear = (-23.0 + 17.0 * wipe, 30.0, 50.0)
    hand = tuple(a + (b - a) * ear for a, b in zip(mouth, over_ear))
    return sit_spec(
        body=(FLAT, (0, 0, 14.0 * lift), 1.0),
        head=head_at((4 * lift - 4 * ear, -6 * lift - 3 * ear - 1.5 * fold, 0),
                     80.0 - 28.0 * lift + lick + 36.0 * ear + 4.0 * fold, -14.0 * lift - 8.0 * ear),
        paws=hands(tuple(lift * value for value in hand), FLAT),
        eyes=(1.0 - 0.85 * lift, 1.0 - 0.85 * lift),
        extra={"EarLeft": (EAR_DETACH["EarLeft"], (-30.0 * fold, 0, 0)), "EarRight": (EAR_DETACH["EarRight"], (0, 0, 0))})


def loaf(frame, weight):
    """Hands and feet tucked in, tail wrapped tight, nodding off: the head sinks, snaps up once, sinks again."""
    tuck = curve(frame, [(0, 0.0), (45, 1.0)])
    nod = curve(frame, [(60, 0.0), (150, 22.0), (165, 22.0), (172, 0.0), (215, 4.0), (270, 26.0)])
    eyes = curve(frame, [(60, 1.0), (120, 0.08), (165, 0.08), (172, 0.85), (215, 0.6), (260, 0.06)])
    return sit_spec(
        body=((0, 0, -12 * tuck), FLAT, 1.0), head=head_at((0, 0, -10 * tuck), 80.0 - nod, 0.0),
        paws={"HandLeft": ((-6 * tuck, 26 * tuck, -10 * tuck), FLAT), "HandRight": ((-6 * tuck, -26 * tuck, -10 * tuck), FLAT),
              "FootLeft": ((8 * tuck, 14 * tuck, -6 * tuck), FLAT), "FootRight": ((8 * tuck, -14 * tuck, -6 * tuck), FLAT)},
        tail=lambda i: (0.0, -16.0), eyes=(eyes, eyes))


def look_around(frame, weight):
    """Eyes then head turn to one side, hold, to the other, back; the body follows a little, ears swivel to the gaze."""
    gaze = curve(frame, [(0, 0.0), (30, 0.0), (52, 55.0), (100, 55.0), (122, -50.0), (170, -50.0), (190, 0.0)])
    late = curve(frame - 8, [(0, 0.0), (30, 0.0), (52, 55.0), (100, 55.0), (122, -50.0), (170, -50.0), (190, 0.0)])
    return sit_spec(
        body=(FLAT, (0, 0, 0.25 * late), 1.0), head=head_at(FLAT, 80.0, gaze),
        extra={"EarLeft": (EAR_DETACH["EarLeft"], (0.3 * late, 0, 0)), "EarRight": (EAR_DETACH["EarRight"], (0.25 * late, 0, 0))})


def knead(frame, weight):
    """Making biscuits: the hands push forward one after the other, eyes half shut, tail swaying slowly."""
    push = curve(frame, [(0, 0.0), (30, 1.0), (170, 1.0), (195, 0.0)])
    left = 9.0 * push * math.sin(2.0 * math.pi * (frame - 30) / 28.0)
    right = 9.0 * push * math.sin(2.0 * math.pi * (frame - 30) / 28.0 + math.pi)
    return sit_spec(
        body=(FLAT, (0, 0, 2.5 * push * math.sin(2.0 * math.pi * (frame - 30) / 56.0)), 1.0),
        head=head_at((2 * push, 0, -4 * push), 80.0 - 14.0 * push, 0.0),
        paws=hands((12 * push + left, 3 * push, 0), (12 * push + right, -3 * push, 0)),
        tail=lambda i: (0.0, -10.0 - push * 9.0 * math.sin(2.0 * math.pi * (frame - 30 - 5 * i) / 56.0)),
        eyes=(1.0 - 0.7 * push, 1.0 - 0.7 * push))


def add_montage(sequence):
    """AM_Cat_Idle_<Name> for Cat_Idle_<Name>: DefaultSlot, short blends."""
    name = sequence.get_name().replace("Cat_Idle_", "AM_Cat_Idle_")
    montage = TOOLKIT["build_montage"](sequence, CLIP["ANIM_PACKAGE"], name, ["Default"], [0.0], ["None"])
    for blend in ("blend_in", "blend_out"):
        settings = montage.get_editor_property(blend).copy()
        settings.set_editor_property("blend_time", MONTAGE_BLEND)
        montage.set_editor_property(blend, settings)

    unreal.EditorAssetLibrary.save_asset("{}/{}".format(CLIP["ANIM_PACKAGE"], name))
    return {"montage": name, "montage_sections": TOOLKIT["montage_sections"](montage)}


if __name__ == "__main__":
    run_clips([("Cat_Idle_Toilet", 300, 24, 270, toilet), ("Cat_Idle_Groom", 300, 30, 265, groom),
               ("Cat_Idle_Loaf", 330, 45, 300, loaf), ("Cat_Idle_LookAround", 210, 24, 180, look_around),
               ("Cat_Idle_Knead", 240, 30, 205, knead)], after=add_montage)
