"""Creates DA_PlayerClassData (UPlayerClassDataAsset) and repopulates its ClassData map by
import_text-ing the exact struct text captured in class_data_snapshot.txt before FPlayerClassData
moved off APlayableCharacter. EditDefaultsOnly struct fields reject the property setter even on a
by-value copy (see struct_container_edit.py), so each entry is round-tripped through import_text
instead of built field-by-field."""
import unreal

ASSET_PATH = "/Game/Characters/Playable"
ASSET_NAME = "DA_PlayerClassData"

data_asset_class = unreal.load_class(None, "/Script/GeoTrinity.PlayerClassDataAsset")

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
factory = unreal.DataAssetFactory()
factory.set_editor_property("data_asset_class", data_asset_class)

full_path = f"{ASSET_PATH}/{ASSET_NAME}"
if unreal.EditorAssetLibrary.does_asset_exist(full_path):
    asset = unreal.EditorAssetLibrary.load_asset(full_path)
else:
    asset = asset_tools.create_asset(ASSET_NAME, ASSET_PATH, data_asset_class, factory)

# Exact struct text captured from BP_GeoPlayableCharacter's CDO ClassData before the property moved.
entry_text = {
    "SQUARE": '(Mesh="/Script/Engine.SkeletalMesh\'/Game/Characters/Meshes/Cube/SKM_Cube.SKM_Cube\'",AliveMaterial="/Script/Engine.Material\'/Game/Characters/Meshes/Cube/MAT_CUBE_ALIVE.MAT_Cube_Alive\'",DeathMaterial="/Script/Engine.Material\'/Game/Characters/Meshes/Cube/MAT_Cube_Dead.MAT_Cube_Dead\'",AnimClass="/Script/Engine.AnimBlueprintGeneratedClass\'/Game/Characters/Anim/Cube/SK_Cube_AnimBlueprint.SK_Cube_AnimBlueprint_C\'",DeathMontage="/Script/Engine.AnimMontage\'/Game/Characters/Anim/Cube/SK_CubeDeath_Montage.SK_CubeDeath_Montage\'",FallMontage=None,ReviveMontage=None,DefaultAttributes="/Script/Engine.BlueprintGeneratedClass\'/Game/AbilitySystem/DefaultAttributes/GE_Square_AttributesPlayer.GE_Square_AttributesPlayer_C\'",SatelliteParams=(Mesh=None,System="/Script/Niagara.NiagaraSystem\'/Game/Art/VFX/Assets/NS_GeoTrinity_Projectile01.NS_GeoTrinity_Projectile01\'",OrbitRadius=100.000000,OrbitSpeed=60.000000,Scale=0.500000,TravelTime=1.000000))',
    "TRIANGLE": '(Mesh="/Script/Engine.SkeletalMesh\'/Game/Characters/Meshes/Cone/SKM_Cone.SKM_Cone\'",AliveMaterial="/Script/Engine.Material\'/Game/Characters/Meshes/Cone/MAT_Cone_Alive.MAT_Cone_Alive\'",DeathMaterial="/Script/Engine.Material\'/Game/Characters/Meshes/Cone/MAT_Cone_Dead.MAT_Cone_Dead\'",AnimClass="/Script/Engine.AnimBlueprintGeneratedClass\'/Game/Characters/Anim/Cone/SK_Cone_AnimBlueprint.SK_Cone_AnimBlueprint_C\'",DeathMontage="/Script/Engine.AnimMontage\'/Game/Characters/Anim/Cone/SK_ConeDeath_Montage.SK_ConeDeath_Montage\'",FallMontage=None,ReviveMontage=None,DefaultAttributes="/Script/Engine.BlueprintGeneratedClass\'/Game/AbilitySystem/DefaultAttributes/GE_Triangle_AttributesPlayer.GE_Triangle_AttributesPlayer_C\'",SatelliteParams=(Mesh=None,System="/Script/Niagara.NiagaraSystem\'/Game/Art/VFX/Assets/NS_GeoTrinity_Projectile01.NS_GeoTrinity_Projectile01\'",OrbitRadius=100.000000,OrbitSpeed=60.000000,Scale=0.500000,TravelTime=1.000000))',
    "CIRCLE": '(Mesh="/Script/Engine.SkeletalMesh\'/Game/Characters/Meshes/Cylinder/SKM_Cylinder.SKM_Cylinder\'",AliveMaterial="/Script/Engine.Material\'/Game/Characters/Meshes/Cylinder/MAT_Cylinder_Alive.MAT_Cylinder_Alive\'",DeathMaterial="/Script/Engine.Material\'/Game/Characters/Meshes/Cylinder/MAT_Cylinder_Dead.MAT_Cylinder_Dead\'",AnimClass="/Script/Engine.AnimBlueprintGeneratedClass\'/Game/Characters/Anim/Cylinder/SK_Cylinder_AnimBlueprint.SK_Cylinder_AnimBlueprint_C\'",DeathMontage="/Script/Engine.AnimMontage\'/Game/Characters/Anim/Cylinder/SK_CylinderDeath_Montage.SK_CylinderDeath_Montage\'",FallMontage=None,ReviveMontage=None,DefaultAttributes="/Script/Engine.BlueprintGeneratedClass\'/Game/AbilitySystem/DefaultAttributes/GE_Circle_AttributesPlayer.GE_Circle_AttributesPlayer_C\'",SatelliteParams=(Mesh=None,System="/Script/Niagara.NiagaraSystem\'/Game/Art/VFX/Assets/NS_GeoTrinity_Projectile01.NS_GeoTrinity_Projectile01\'",OrbitRadius=100.000000,OrbitSpeed=60.000000,Scale=0.200000,TravelTime=1.000000))',
}

class_data = {}
for class_name, text in entry_text.items():
    player_class = getattr(unreal.PlayerClass, class_name)
    entry = unreal.PlayerClassData()
    entry.import_text(text)
    class_data[player_class] = entry

asset.set_editor_property("ClassData", class_data)

unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)
unreal.log(f"Saved {full_path}")
