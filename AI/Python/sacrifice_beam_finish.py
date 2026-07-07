"""
Finish the SacrificeBeam setup — run via MCP execute_script AFTER an editor restart
(the three tags added to Config/Tags/GeoGameplayTags.ini must be resolvable).
Writes a report to Saved/sacrifice_beam_finish.json.
"""
import unreal, json

out = {}
FOLDER = '/Game/AbilitySystem/Abilities/Square/SacrificeBeam'

# --- 1. GA: full AbilityTags + FireGameplayCueTag ---
ga_bp = unreal.EditorAssetLibrary.load_asset(FOLDER + '/GA_Square_SpecAlt_SacrificeBeam')
ga_cdo = unreal.get_default_object(ga_bp.generated_class())
tc = unreal.GameplayTagContainer()
tc.import_text('(GameplayTags=((TagName="Ability.Type.SpecialAlternative"),(TagName="Ability.Spell.SacrificeBeam")))')
ga_cdo.set_editor_property('AbilityTags', tc)
out['ga_tags'] = ga_cdo.get_editor_property('AbilityTags').export_text()

fire_tag = unreal.GameplayTag()
fire_tag.import_text('(TagName="GameplayCue.Ability.Player.Square.SacrificeBeam")')
ga_cdo.set_editor_property('FireGameplayCueTag', fire_tag)
out['fire_tag'] = ga_cdo.get_editor_property('FireGameplayCueTag').export_text()
unreal.EditorAssetLibrary.save_loaded_asset(ga_bp)

# --- 2. GE_SacrificeMark: WhileActive GameplayCue ---
ge_bp = unreal.EditorAssetLibrary.load_asset(FOLDER + '/GE_SacrificeMark')
ge_cdo = unreal.get_default_object(ge_bp.generated_class())
cue = unreal.GameplayEffectCue()
cue.import_text('(GameplayCueTags=(GameplayTags=((TagName="GameplayCue.Status.Sacrificed"))))')
ge_cdo.set_editor_property('GameplayCues', [cue])
out['ge_cues'] = str(ge_cdo.get_editor_property('GameplayCues'))
unreal.EditorAssetLibrary.save_loaded_asset(ge_bp)

# --- 3. DA_AbilityInfo: fill AbilityTag on the swapped entry ---
da = unreal.load_asset('/Game/AbilitySystem/Data/DA_AbilityInfo')
arr = da.get_editor_property('SquareAbilities')
for i, e in enumerate(arr):
    if 'SacrificeBeam' in str(e.get_editor_property('AbilityClass')):
        new_e = unreal.PlayersGameplayAbilityInfo()
        new_e.import_text(e.export_text())
        new_e.import_text('(AbilityTag=(TagName="Ability.Spell.SacrificeBeam"))')
        arr[i] = new_e
        out['ability_info_index'] = i
da.set_editor_property('SquareAbilities', arr)
unreal.EditorLoadingAndSavingUtils.save_packages([da.get_outermost()], False)

open(r'C:/GeoTrinity/Saved/sacrifice_beam_finish.json', 'w').write(json.dumps(out, indent=1))
