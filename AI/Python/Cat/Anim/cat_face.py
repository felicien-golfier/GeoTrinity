"""The Cat's evil face (AI/Tutorial/CatAnimation.md, "Evil face"): arched squinting eyes, a wide grin, fangs, ears pinned
back. One beat = one weight curve (attack, hold, release); every feature reads it a frame or two late so the face never
snaps as one piece.

A clip calls `apply_evil(deltas, frame, start, hold)` on its finished pose: the face adds to what the eyes, mouth and ears
already do (blink, glance, swivel) and sets the fangs, which hide outside the beat.

Loaded by path like cat_clip.py; the idle, the walks and the run use it. Nothing is written by running it.
"""
import unreal

_CLIP_PATH = unreal.Paths.project_dir() + "AI/Python/Cat/Anim/cat_clip.py"
CLIP = {}
exec(compile(open(_CLIP_PATH, encoding="utf-8").read(), _CLIP_PATH, "exec"), CLIP)
attack_release, REST_DELTA = CLIP["attack_release"], CLIP["REST_DELTA"]

ATTACK = 4  # frames
RELEASE = 8
EYE_SQUINT = 0.35  # Z scale (the face's up axis)
EYE_TILT_DEGREES = 22.0  # roll on top of the rest tilt, inner corners down: the left eye's sign, the right mirrors it
EYE_PAIR_LAG = 1  # frames the right eye reads late
MOUTH_LAG = 1
MOUTH_SCALE = (1.0, 1.3, 1.6)  # Y is the grin's width, Z its depth
MOUTH_SINK = 1.0  # units down, mouth and fangs
FANG_LATE = 2  # frames after the grin opens
FANG_EARLY = 1  # frames before it closes
EAR_PIN = {"EarLeft": (-14.0, 10.0), "EarRight": (10.0, 14.0)}  # roll outward, pitch back; unequal on purpose


def evil_weight(frame, start, hold):
    """0 before `start`, up in ATTACK frames, held `hold`, back to 0 in RELEASE frames."""
    return attack_release(frame, start, ATTACK, hold, RELEASE)


def add(deltas, bone, translation, rotation, scale):
    """`deltas[bone]` plus a translation and a rotation, times a scale."""
    base = deltas.get(bone, REST_DELTA)
    deltas[bone] = (tuple(a + b for a, b in zip(base[0], translation)), tuple(a + b for a, b in zip(base[1], rotation)),
                    tuple(a * b for a, b in zip(base[2], scale)))


def apply_evil(deltas, frame, start, hold):
    """Add the evil beat (`start` frame, `hold` frames at full) to `deltas` and return it."""
    for side, (eye, mouth, fang, ear) in enumerate(zip(("EyeLeft", "EyeRight"), ("Mouth0", "Mouth1"),
                                                       ("FangLeft", "FangRight"), ("EarLeft", "EarRight"))):
        eye_weight = evil_weight(frame - side * EYE_PAIR_LAG, start, hold)
        add(deltas, eye, (0, 0, 0), (-EYE_TILT_DEGREES * eye_weight * (1 - 2 * side), 0, 0),
            (1, 1, 1.0 - (1.0 - EYE_SQUINT) * eye_weight))

        mouth_weight = evil_weight(frame - MOUTH_LAG, start, hold)
        add(deltas, mouth, (0, 0, -MOUTH_SINK * mouth_weight), (0, 0, 0),
            tuple(1.0 + (target - 1.0) * mouth_weight for target in MOUTH_SCALE))

        fang_weight = evil_weight(frame - MOUTH_LAG, start + FANG_LATE, max(hold - FANG_LATE - FANG_EARLY, 0))
        hidden = CLIP["FANG_HIDDEN_SCALE"]
        deltas[fang] = ((0, 0, -MOUTH_SINK * fang_weight), (0, 0, 0), (hidden + (1.0 - hidden) * fang_weight,) * 3)

        roll, pitch = EAR_PIN[ear]
        pin = evil_weight(frame, start, hold)
        add(deltas, ear, (0, 0, 0), (roll * pin, pitch * pin, 0), (1, 1, 1))
    return deltas
