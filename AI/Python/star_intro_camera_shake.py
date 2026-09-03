"""Camera shake for the star boss's intro montage: one curve that climbs with the clip it is played over.

The intro grows a star out of a seed, throws its points out one at a time faster and faster, winds the whole thing
in and blows it open. The camera is given the same shape. It holds dead still on the seed, picks up a tremor as the
seed stirs, then takes a kick on every point that erupts — each kick larger than the last and landing closer to it,
since the eruptions themselves tighten — and settles between them onto a floor that climbs with every point out.
With all of them out it nearly goes quiet, which is what gives the wind-up somewhere to climb from: through the
wind-up the shake escalates in amplitude and frequency together, and then stops dead for the frozen frames the
animation holds. The nova hits from that silence, once, harder than anything before it, and dies across the settle.

Nothing here is keyed by hand against the clip. The eruption schedule is re-derived from the same rhythm the intro
was built on, so every kick lands on the frame a point comes out, and the derived length is checked against the
sequence the montage plays — a schedule that has drifted fails the run rather than shaking off the beat.

The curve is authored in seconds from the notify, not in montage time, so the notify frame is subtracted from every
beat. The key table itself goes in through the shared curve writer.

Amplitudes are in world units against a 3000-unit orthographic view, and the offsets they drive stay in the screen
plane: see the curve shake pattern for why nothing is spent on the view axis.

Run via mcp-unreal execute_script. Re-runnable: rewrites the curve, the shake and the montage's notify in place.
Report written to REPORT.
"""
import unreal

SKELETON_PATH = "/Game/Characters/Meshes/Star/SK_Star"
SEQUENCE_PATH = "/Game/Characters/Anim/Star/SK_Star_Sequence_Intro"
MONTAGE_PATH = "/Game/Characters/Anim/Star/SK_Star_Montage_Intro"
CAMERA_PACKAGE = "/Game/Camera"
CURVE_NAME = "Curve_StarIntroShake"
SHAKE_NAME = "CameraShake_StarIntro"
REPORT = "star_intro_camera_shake.txt"

SPIKE_PREFIX = "apexe_outside_"
NOTIFY_TRACK = "1"
NOTIFY_FRAME = 1  # a notify on frame 0 can fall outside the montage's first tick window

FPS = 30

# --- The intro's own rhythm, mirrored from star_intro.py; the frame-count check is what holds them together ------
WAKE = 8
FIRST, GAP, GAP_RATIO, MIN_GAP = 34, 16.0, 0.78, 3
GROW, ALIVE, WIND, FREEZE, BURST_FRAMES, HOLD, SETTLE, MAX_LAG = 5, 12, 44, 4, 3, 14, 52, 2

# --- What the camera is worth at each beat: (offset amplitude in units, frequency in Hz, roll in degrees) --------
SILENT = (0.0, 12.0, 0.0)
STIR = (2.5, 9.0, 0.05)          # the seed shivering: felt rather than seen
LULL = (4.0, 16.0, 0.1)          # every point out, the star only turning — the quiet the wind-up climbs from
WIND_START = (12.0, 20.0, 0.4)
WIND_END = (46.0, 36.0, 1.5)     # wound tightest, the frame before the animation freezes
NOVA = (90.0, 24.0, 3.0)         # slower than the wind-up and three times its reach: a slam, not a buzz
SETTLED = (0.0, 12.0, 0.0)

# Per eruption, first to last: each kick harder and its floor higher, so the ring escalates on its own.
ERUPT_SPIKE = (8.0, 30.0)
ERUPT_FLOOR = (2.5, 10.0)
ERUPT_FREQUENCY = (13.0, 26.0)
ERUPT_ROLL = (0.15, 0.7)
FLOOR_ROLL_SHARE = 0.33
KICK_FRAMES = 3                  # frames a kick decays over, capped by the next eruption

# Frames after the nova -> the share of it still left, which the roll and the frequency fall off with.
NOVA_DECAY = ((8, 0.47), (17, 0.22), (40, 0.08))

LOG = []


def lerp(pair, alpha):
    return pair[0] + (pair[1] - pair[0]) * alpha


def beat_table(toolkit, tips):
    """The frames the clip turns on -> {name: frame}, re-derived from the intro's rhythm."""
    schedule = toolkit["accelerating_schedule"](FIRST, tips, GAP, GAP_RATIO, MIN_GAP)
    alive = schedule[-1] + GROW + ALIVE
    wind_still = alive + WIND
    blast = wind_still + FREEZE
    nova = blast + 1
    plateau = blast + BURST_FRAMES - 1 + MAX_LAG + HOLD
    return {"schedule": schedule, "alive": alive, "wind_still": wind_still, "blast": blast, "nova": nova,
            "plateau": plateau, "frames": plateau + SETTLE + MAX_LAG}


def eruption_keys(schedule):
    """A kick per point erupting -> {frame: values}: the floor it comes off, the spike, the higher floor it lands on."""
    keys = {}
    last = len(schedule) - 1
    for place, frame in enumerate(schedule):
        alpha, next_alpha = place / float(last), min(1.0, (place + 1) / float(last))
        following = schedule[place + 1] if place < last else frame + KICK_FRAMES + 1
        keys[frame - 1] = floor_values(alpha)
        keys[frame] = (lerp(ERUPT_SPIKE, alpha), lerp(ERUPT_FREQUENCY, alpha), lerp(ERUPT_ROLL, alpha))
        keys[frame + min(KICK_FRAMES, following - frame - 1)] = floor_values(next_alpha)
    return keys


def floor_values(alpha):
    """What the camera holds between two eruptions, `alpha` of the way through the ring."""
    return (lerp(ERUPT_FLOOR, alpha), lerp(ERUPT_FREQUENCY, alpha), lerp(ERUPT_ROLL, alpha) * FLOOR_ROLL_SHARE)


def shake_keys(beats):
    """Every key of the shake curve -> {frame: (amplitude, frequency, roll)}.

    The frozen frames are keyed silent so the nova lands out of nothing, which is the same stillness the animation
    holds there.
    """
    schedule = beats["schedule"]
    quiet = (schedule[-1] + KICK_FRAMES + beats["alive"]) // 2
    keys = {0: SILENT, WAKE: SILENT, schedule[0] - 1: STIR}
    keys.update(eruption_keys(schedule))
    keys[quiet] = LULL
    keys[beats["alive"]] = WIND_START
    keys[beats["wind_still"] - 1] = WIND_END
    keys[beats["wind_still"]] = (0.0, WIND_END[1], 0.0)
    keys[beats["blast"]] = (0.0, WIND_END[1], 0.0)
    keys[beats["nova"]] = NOVA
    for frames_after, share in NOVA_DECAY:
        keys[beats["nova"] + frames_after] = (NOVA[0] * share, lerp((SETTLED[1], NOVA[1]), share), NOVA[2] * share)
    keys[beats["frames"]] = SETTLED
    return keys


def build_curve(keys, curves):
    """Import the key table as the shake curve. Times are seconds from the notify, so frame 0 clamps onto it."""
    return curves["import_curve"](CAMERA_PACKAGE, CURVE_NAME,
                                  {max(0.0, (frame - NOTIFY_FRAME) / float(FPS)): keys[frame] for frame in keys})


def module_class(name):
    """A GeoTrinity UCLASS by script path.

    Not every reflected class is mirrored as a `unreal.` attribute — the shake pattern is one that is not — so the
    path is what says whether a class is in the build, never the module.
    """
    try:
        return unreal.load_class(None, "/Script/GeoTrinity." + name)
    except Exception:
        return None


def build_shake(curve, shake_class, toolkit):
    """The camera shake asset: single-instance so a replay never stacks, and the curve is all it authors.

    Reparented on the way through rather than only on creation, since the pattern reaches spawned shakes only as the
    default subobject the native shake class declares, and an asset built on any other base carries none.
    """
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("ParentClass", shake_class)
    blueprint = toolkit["get_or_create_asset"](CAMERA_PACKAGE, SHAKE_NAME, unreal.Blueprint, factory)
    unreal.BlueprintEditorLibrary.reparent_blueprint(blueprint, shake_class)

    shake = unreal.get_default_object(blueprint.generated_class())
    shake.get_editor_property("RootShakePattern").set_editor_property("ShakeCurve", curve)
    shake.set_editor_property("bSingleInstance", True)
    unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False)
    return blueprint


def add_notify(shake, notify_class, toolkit):
    """Put the one notify that starts the shake on the montage, on the frame the curve is authored from."""
    montage = unreal.load_asset(MONTAGE_PATH)
    toolkit["set_notify"](montage, NOTIFY_TRACK, NOTIFY_FRAME / float(FPS), notify_class,
                          {"ShakeClass": shake.generated_class()})
    unreal.EditorAssetLibrary.save_loaded_asset(montage, only_if_is_dirty=False)
    return montage


def report_curve(beats, keys, curve):
    named = {WAKE: "the seed stirs", beats["alive"]: "every point out",
             beats["wind_still"]: "frozen: dead still", beats["nova"]: "NOVA", beats["frames"]: "at rest"}
    named.update({frame: "point %d of %d erupts" % (place + 1, len(beats["schedule"]))
                  for place, frame in enumerate(beats["schedule"])})

    LOG.append("{} — {} keys over {:.2f}s, sampled in seconds from the notify on f{}".format(
        CURVE_NAME, len(keys), (beats["frames"] - NOTIFY_FRAME) / float(FPS), NOTIFY_FRAME))
    LOG.append("")
    LOG.append("frame   time   amplitude   freq    roll   read back            beat")
    for frame in sorted(keys):
        time = max(0.0, (frame - NOTIFY_FRAME) / float(FPS))
        sampled = curve.get_vector_value(time)
        LOG.append("%5d %6.3f %11.2f %6.1f %7.3f   %6.2f %5.1f %6.3f   %s" % (
            frame, time, keys[frame][0], keys[frame][1], keys[frame][2],
            sampled.x, sampled.y, sampled.z, named.get(frame, "")))

    peak = max(values[0] for values in keys.values())
    LOG.append("")
    LOG.append("peak %.0f units of offset and %.1f deg of roll, %.1f%% of a 3000-unit view across" % (
        peak, max(values[2] for values in keys.values()), 200.0 * peak / 3000.0))


def report_assets(beats, curve, shake, montage, tips, toolkit):
    minimum, maximum = curve.get_time_range()
    LOG.append("curve time range %.3f..%.3f s, which is the shake's whole duration" % (minimum, maximum))
    pattern = unreal.get_default_object(shake.generated_class()).get_editor_property("RootShakePattern")
    LOG.append("%s root pattern %s -> %s" % (
        SHAKE_NAME, pattern.get_class().get_name(), pattern.get_editor_property("ShakeCurve").get_name()))
    for time, notify in toolkit["notify_events"](montage):
        LOG.append("montage notify at %.3fs: %s -> %s" % (
            time, notify.get_class().get_name(), notify.get_editor_property("ShakeClass").get_name()))
    LOG.append("%d points, kicks on f%s" % (tips, beats["schedule"]))


try:
    toolkit, curves = {}, {}
    for path, module in ((unreal.Paths.project_dir() + "AI/Python/anim_sequence_authoring.py", toolkit),
                         (unreal.Paths.project_dir() + "AI/Python/curve_asset_authoring.py", curves)):
        exec(compile(open(path).read(), path, "exec"), module)

    tips = len([bone for bone in toolkit["reference_pose_table"](SKELETON_PATH) if bone.startswith(SPIKE_PREFIX)])
    beats = beat_table(toolkit, tips)
    played = unreal.load_asset(SEQUENCE_PATH).get_editor_property("number_of_sampled_frames")
    if beats["frames"] != played:
        raise RuntimeError("the intro's rhythm derives {} frames against the {} the sequence plays — the beats here"
                           " no longer match star_intro.py".format(beats["frames"], played))

    keys = shake_keys(beats)
    curve = build_curve(keys, curves)
    report_curve(beats, keys, curve)
    LOG.append("")

    classes = {name: module_class(name) for name in ("GeoCurveCameraShake", "GeoCameraShakeNotify")}
    missing = [name for name, loaded in classes.items() if not loaded]
    if missing:
        raise RuntimeError("{} not in this editor build — the curve is written; build the project and reopen the"
                           " editor, then run this again for the shake and the notify".format(missing))

    shake = build_shake(curve, classes["GeoCurveCameraShake"], toolkit)
    montage = add_notify(shake, classes["GeoCameraShakeNotify"], toolkit)
    report_assets(beats, curve, shake, montage, tips, toolkit)
except Exception:
    import traceback
    LOG.append(traceback.format_exc())

with open(unreal.Paths.project_saved_dir() + REPORT, "w") as handle:
    handle.write("\n".join(LOG))
