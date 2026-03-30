#include "AbilitySystem/Lib/GeoGameplayTags.h"

#include "GameplayTagsManager.h"


FGeoGameplayTags FGeoGameplayTags::GameplayTags;

namespace
{
	void CreateAndAssignGameplayTag(FGameplayTag& outTag, FName const& tagName, FString const& comment)
	{
		outTag = UGameplayTagsManager::Get().AddNativeGameplayTag(tagName, comment);
	}

	void AddInputTag(FGameplayTag& outTag, FString const& tagName, FString const& comment)
	{
		CreateAndAssignGameplayTag(outTag, FName(RootTagNames::InputTag + "." + tagName), comment);
	}

	void AddAbilityTypeTag(FGameplayTag& outTag, FString const& tagName, FString const& comment)
	{
		CreateAndAssignGameplayTag(outTag, FName(RootTagNames::AbilityTypeTag + "." + tagName), comment);
	}
} // namespace

void FGeoGameplayTags::InitializeNativeGameplayTags()
{
	CreateAndAssignGameplayTag(GameplayTags.Gameplay_Damage, "Gameplay.Damage", "Tag to identify damage");
	CreateAndAssignGameplayTag(GameplayTags.Gameplay_Heal, "Gameplay.Heal", "Tag to identify healing");
	CreateAndAssignGameplayTag(GameplayTags.Gameplay_Shield, "Gameplay.Shield", "Tag to identify shield");
	CreateAndAssignGameplayTag(GameplayTags.Gameplay_Drain, "Gameplay.Drain",
							   "SetByCaller magnitude tag for deployable health drain GE");
	CreateAndAssignGameplayTag(GameplayTags.Gameplay_DurationMagnitude, "Gameplay.DurationMagnitude",
							   "SetByCaller magnitude tag for Duration Magnitude");
	CreateAndAssignGameplayTag(GameplayTags.Gameplay_GenericMagnitude, "Gameplay.GenericMagnitude",
							   "SetByCaller magnitude tag for Generic Gameplay Effects.");

	// INPUT //
	AddInputTag(GameplayTags.InputTag_Basic, "Basic", "Input tag for left mouse button-linked spell");
	AddInputTag(GameplayTags.InputTag_Special, "Special", "Input tag for right mouse button-linked spell");
	AddInputTag(GameplayTags.InputTag_SpecialAlternative, "SpecialAlternative",
				"Input tag for alternative special ability");
	AddInputTag(GameplayTags.InputTag_Reload, "Reload", "Input tag for reload action");
	AddInputTag(GameplayTags.InputTag_Dash, "Dash", "Input tag for dash ability");

	// ABILITY TYPES //
	AddAbilityTypeTag(GameplayTags.Ability_Type_Basic, "Basic", "Tag associated with basic spells");
	AddAbilityTypeTag(GameplayTags.Ability_Type_Special, "Special", "Tag associated with special spells");
	AddAbilityTypeTag(GameplayTags.Ability_Type_SpecialAlternative, "SpecialAlternative",
					  "Tag associated with Special Alternate spells");
	AddAbilityTypeTag(GameplayTags.Ability_Type_Reload, "Reload", "Tag associated with Reloading spells");
	AddAbilityTypeTag(GameplayTags.Ability_Type_Dash, "Dash", "Tag associated with Dash spells");

	// PLAYER CLASSES //
	CreateAndAssignGameplayTag(GameplayTags.PlayerClass_Triangle, "PlayerClass.Triangle", "DPS class");
	CreateAndAssignGameplayTag(GameplayTags.PlayerClass_Circle, "PlayerClass.Circle", "Healer class");
	CreateAndAssignGameplayTag(GameplayTags.PlayerClass_Square, "PlayerClass.Square", "Tank class");

	// ABILITY STATES //
	CreateAndAssignGameplayTag(GameplayTags.Status_Reloading, "Status.Reloading",
							   "Active while reload ability is in progress");

	// BUFF STATUS //
	CreateAndAssignGameplayTag(GameplayTags.Status_Buff_DamageBoost, "Status.Buff.DamageBoost", "Damage boost buff");
	CreateAndAssignGameplayTag(GameplayTags.Status_Buff_DamageReduction, "Status.Buff.DamageReduction",
							   "Damage reduction buff");
	CreateAndAssignGameplayTag(GameplayTags.Status_Buff_HealBoost, "Status.Buff.HealBoost", "Heal boost buff");
	CreateAndAssignGameplayTag(GameplayTags.Status_Buff_Speed, "Status.Buff.Speed", "Movement speed buff");
	CreateAndAssignGameplayTag(GameplayTags.Status_Buff_Shield, "Status.Buff.Shield", "Shield buff");
}
