# Builds the sound classes the volume settings drive: SC_General parents SC_Effects, SC_Music and SC_Interface.
# Also creates Mix_Volume (the base sound mix UGeoGameUserSettings overrides) and puts every menu sound on SC_Interface.
# Every other sound keeps no class and falls back to SC_Effects, the project's Default Sound Class.
# Re-run safe: existing assets are reused in place. Reports to Saved/volume_sound_classes.txt.
import unreal

FOLDER = "/Game/Art/SFX/Mix"
PARENT = "SC_General"
CHILDREN = ["SC_Effects", "SC_Music", "SC_Interface"]
MIX = "Mix_Volume"
INTERFACE_SOUND_FOLDER = "/Game/HUD/Assets/SFX"

tools = unreal.AssetToolsHelpers.get_asset_tools()
report = []


def load_or_create(name, asset_class, factory):
    path = "%s/%s" % (FOLDER, name)
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.load_asset(path)
    return tools.create_asset(name, FOLDER, asset_class, factory)


general = load_or_create(PARENT, unreal.SoundClass, unreal.SoundClassFactory())
children = [load_or_create(name, unreal.SoundClass, unreal.SoundClassFactory()) for name in CHILDREN]
load_or_create(MIX, unreal.SoundMix, unreal.SoundMixFactory())

# One child per edit: the ChildClasses post-edit only re-parents the first class it did not hold before.
general.set_editor_property("child_classes", [])
for count in range(1, len(children) + 1):
    general.set_editor_property("child_classes", children[:count])

interface = children[CHILDREN.index("SC_Interface")]
saved = ["%s/%s" % (FOLDER, name) for name in [PARENT, MIX] + CHILDREN]
for data in unreal.AssetRegistryHelpers.get_asset_registry().get_assets_by_path(INTERFACE_SOUND_FOLDER, True):
    sound = data.get_asset()
    if isinstance(sound, unreal.SoundBase):
        sound.set_editor_property("sound_class_object", interface)
        saved.append(sound.get_path_name().split(".")[0])

for path in saved:
    unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)

report.append("General children: %s" % [c.get_name() for c in general.get_editor_property("child_classes")])
for data in unreal.AssetRegistryHelpers.get_asset_registry().get_assets_by_path("/Game", True):
    if data.asset_class_path.asset_name in ("SoundWave", "SoundCue", "MetaSoundSource"):
        sound_class = data.get_asset().get_editor_property("sound_class_object")
        if sound_class:
            report.append("%s -> %s" % (data.package_name, sound_class.get_name()))

open(unreal.Paths.project_saved_dir() + "volume_sound_classes.txt", "w").write("\n".join(report))
