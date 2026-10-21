"""Check that the badge and boss clip scripts still write exactly what the live clips hold, before any of them is
re-run.

Each script runs with its asset folder swapped for a fresh sandbox, so nothing live is touched; every sandbox asset is
then compared with its live namesake — sequences bone by bone on every frame of the built data, montages by slot,
sections, played length, blends and notifies. A live clip retouched by hand shows here as a script that would undo
it: bake it with bake_clips.py. Each run builds into a sandbox of its own, since a loaded asset cannot be deleted
and recreated in one editor session; delete Content/Developers/AIScratch with the editor closed.

Run via mcp-unreal execute_script or the headless pythonscript commandlet. TARGETS picks what runs, in order: a
script reading a sibling's clip runs after it. Report written to AI/Output/verify_badge_scripts.txt.
"""
import math
import time
import traceback

import unreal

STAMP = time.strftime("%Y%m%d_%H%M%S")
SCRIPTS = unreal.Paths.project_dir() + "AI/Python/"
REPORT = unreal.Paths.project_dir() + "AI/Output/verify_badge_scripts.txt"

# (live folder, scripts in the order they run: a script reading a sibling's clip runs after it)
TARGETS = [
    ("/Game/Characters/Anim/ClassBadge",
     ["Badge/Anim/class_badge_idle.py", "Badge/Anim/class_badge_body_turns.py", "Badge/Anim/class_badge_circle_charge_orbit.py",
      "Badge/Anim/class_badge_death.py", "Badge/Anim/class_badge_deploy.py", "Badge/Anim/class_badge_rez.py", "Badge/Anim/class_badge_square_fire_piston.py",
      "Badge/Anim/class_badge_square_sacrifice.py", "Badge/Anim/class_badge_triangle_fire.py", "Badge/Anim/class_badge_triangle_reload.py"]),
    ("/Game/Characters/Anim/HexBoss",
     ["Boss/Hex/Anim/hex_boss_intro.py", "Boss/Hex/Anim/hex_boss_idle.py", "Boss/Hex/Anim/hex_boss_death.py", "Boss/Hex/Anim/hex_boss_abilities.py", "Boss/Hex/Anim/hex_boss_launch.py"]),
    ("/Game/Characters/Anim/Star",
     ["Boss/Star/Anim/star_intro.py", "Boss/Star/Anim/star_idle_breath.py", "Boss/Star/Anim/star_death.py", "Boss/Star/Anim/star_devastating_wave.py", "Boss/Star/Anim/star_spike_nova.py"]),
]
# Below these a difference is float noise from the build.
TRANSLATION_TOLERANCE = 0.01
ROTATION_TOLERANCE = 0.05  # degrees
SCALE_TOLERANCE = 0.0005

LOG = []
LIBRARY = unreal.AnimationLibrary


def run_in_sandbox(script, live, sandbox):
    path = SCRIPTS + script
    source = open(path, encoding="utf-8").read().replace(live, sandbox)
    exec(compile(source, path, "exec"), {"__name__": "__main__"})


def bone_names(sequence):
    pose = unreal.AnimPoseExtensions.get_reference_pose(sequence.get_skeleton())
    return [str(bone) for bone in unreal.AnimPoseExtensions.get_bone_names(pose)]


def transform_gap(first, second):
    """(translation, rotation in degrees, scale) apart."""
    dot = abs(first.rotation.x * second.rotation.x + first.rotation.y * second.rotation.y
              + first.rotation.z * second.rotation.z + first.rotation.w * second.rotation.w)
    return ((first.translation - second.translation).length(),
            math.degrees(2.0 * math.acos(min(1.0, dot))),
            (first.scale3d - second.scale3d).length())


def notifies(anim):
    """(time, duration, class) of every notify and notify state."""
    events = []
    for event in LIBRARY.get_animation_notify_events(anim):
        notify = event.get_editor_property("notify") or event.get_editor_property("notify_state_class")
        events.append((round(LIBRARY.get_anim_notify_event_trigger_time(event), 3),
                       round(LIBRARY.get_anim_notify_event_duration(event), 3),
                       notify.get_class().get_name() if notify else str(event.get_editor_property("notify_name"))))
    return sorted(events)


def curves(anim):
    return sorted(str(name) for name in LIBRARY.get_animation_curve_names(anim, unreal.RawCurveTrackTypes.RCT_FLOAT))


def compare_sequence(live, sandbox):
    """Every difference between the two sequences, as lines; none when the script reproduces the live clip."""
    lines = []
    live_frames, sandbox_frames = LIBRARY.get_num_frames(live), LIBRARY.get_num_frames(sandbox)
    if live_frames != sandbox_frames:
        lines.append("frames: live {} script {}".format(live_frames, sandbox_frames))
    for name in ("additive_anim_type", "rate_scale", "enable_root_motion"):
        if str(live.get_editor_property(name)) != str(sandbox.get_editor_property(name)):
            lines.append("{}: live {} script {}".format(name, live.get_editor_property(name),
                                                        sandbox.get_editor_property(name)))
    for bone in bone_names(live):
        worst = [0.0, 0.0, 0.0]
        worst_frame = [0, 0, 0]
        for frame in range(min(live_frames, sandbox_frames) + 1):
            gap = transform_gap(LIBRARY.get_bone_pose_for_frame(live, bone, frame, False),
                                LIBRARY.get_bone_pose_for_frame(sandbox, bone, frame, False))
            for index in range(3):
                if gap[index] > worst[index]:
                    worst[index], worst_frame[index] = gap[index], frame
        if (worst[0] > TRANSLATION_TOLERANCE or worst[1] > ROTATION_TOLERANCE or worst[2] > SCALE_TOLERANCE):
            lines.append("{}: translation {:.3f} (frame {}), rotation {:.2f} deg (frame {}), scale {:.4f} (frame {})"
                         .format(bone, worst[0], worst_frame[0], worst[1], worst_frame[1], worst[2], worst_frame[2]))
    for label, read in (("notifies", notifies), ("curves", curves)):
        if read(live) != read(sandbox):
            lines.append("{}: live {} script {}".format(label, read(live), read(sandbox)))
    return lines


def montage_layout(montage):
    util = unreal.get_default_object(unreal.GeoAnimBuilderUtil)
    names, starts, following = util.get_montage_sections(montage)
    layout = {"slots": [str(slot) for slot in LIBRARY.get_montage_slot_names(montage)],
              "sections": [(str(n), round(s, 4), str(f)) for n, s, f in zip(names, starts, following)],
              "length": round(montage.get_editor_property("sequence_length"), 4),
              "rate_scale": montage.get_editor_property("rate_scale"),
              "auto_blend_out": montage.get_editor_property("enable_auto_blend_out"),
              "notifies": notifies(montage), "curves": curves(montage)}
    for name in ("blend_in", "blend_out"):
        blend = montage.get_editor_property(name)
        layout[name] = (round(blend.get_editor_property("blend_time"), 4), str(blend.get_editor_property("blend_option")))
    layout["blend_out_trigger_time"] = montage.get_editor_property("blend_out_trigger_time")
    return layout


def compare_montage(live, sandbox):
    live_layout, sandbox_layout = montage_layout(live), montage_layout(sandbox)
    return ["{}: live {} script {}".format(key, live_layout[key], sandbox_layout[key])
            for key in live_layout if live_layout[key] != sandbox_layout[key]]


def compare_all(live_root, sandbox_root):
    registry = unreal.EditorAssetLibrary
    sandbox_paths = sorted(path.split(".")[0] for path in registry.list_assets(sandbox_root, True, False))
    compared = set()
    for sandbox_path in sandbox_paths:
        live_path = sandbox_path.replace(sandbox_root, live_root)
        sandbox = unreal.load_asset(sandbox_path)
        live = unreal.load_asset(live_path)
        name = live_path.replace(live_root + "/", "")
        if isinstance(sandbox, unreal.AnimSequence) or isinstance(sandbox, unreal.AnimMontage):
            compared.add(live_path)
            if live is None:
                LOG.append("{}: written by a script, no live clip of that name".format(name))
                continue
            lines = (compare_sequence if isinstance(sandbox, unreal.AnimSequence) else compare_montage)(live, sandbox)
            LOG.append("{}: {}".format(name, "MATCHES" if not lines else "DIFFERS"))
            LOG.extend("    " + line for line in lines)
    for live_path in sorted(path.split(".")[0] for path in registry.list_assets(live_root, True, False)):
        asset = unreal.load_asset(live_path)
        if (isinstance(asset, unreal.AnimSequence) or isinstance(asset, unreal.AnimMontage)) \
                and live_path not in compared:
            LOG.append("{}: live clip no script writes".format(live_path.replace(live_root + "/", "")))


for live_folder, scripts in TARGETS:
    sandbox_folder = "/Game/Developers/AIScratch/{}_{}".format(live_folder.split("/")[-1], STAMP)
    LOG.append("sandbox " + sandbox_folder)
    for script_name in scripts:
        try:
            run_in_sandbox(script_name, live_folder, sandbox_folder)
            LOG.append("ran {}".format(script_name))
        except Exception:
            LOG.append("FAILED {}: {}".format(script_name, traceback.format_exc()))

    try:
        compare_all(live_folder, sandbox_folder)
    except Exception:
        LOG.append(traceback.format_exc())

with open(REPORT, "w") as handle:
    handle.write("\n".join(LOG))
