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

	void AddAbilityTag(FGameplayTag& outTag, FString const& tagName, FString const& comment)
	{
		CreateAndAssignGameplayTag(outTag, FName(RootTagNames::AbilityTag + "." + tagName), comment);
	}
} // namespace

void FGeoGameplayTags::InitializeNativeGameplayTags()
{
	CreateAndAssignGameplayTag(GameplayTags.Gameplay_Damage, "Gameplay.Damage", "Tag to identify damage");
	CreateAndAssignGameplayTag(GameplayTags.Data_Drain, "Data.Drain",
							   "SetByCaller magnitude tag for deployable health drain GE");

	// INPUT //
	AddInputTag(GameplayTags.InputTag_Basic, "Basic", "Input tag for left mouse button-linked spell");
	AddInputTag(GameplayTags.InputTag_Special, "Special", "Input tag for right mouse button-linked spell");
	AddInputTag(GameplayTags.InputTag_SpecialAlternative, "SpecialAlternative",
				"Input tag for alternative special ability");
	AddInputTag(GameplayTags.InputTag_Reload, "Reload", "Input tag for reload action");
	AddInputTag(GameplayTags.InputTag_Dash, "Dash", "Input tag for dash ability");

	// ABILITY TYPES //
	AddAbilityTag(GameplayTags.Ability_Type_Basic, "Type.Basic", "Tag associated with basic spells");
	AddAbilityTag(GameplayTags.Ability_Type_Special, "Type.Special", "Tag associated with special spells");
	AddAbilityTag(GameplayTags.Ability_Type_SpecialAlternative, "Type.SpecialAlternative",
				  "Tag associated with Special Alternate spells");
	AddAbilityTag(GameplayTags.Ability_Type_Reload, "Type.Reload", "Tag associated with Reloading spells");
	AddAbilityTag(GameplayTags.Ability_Type_Dash, "Type.Dash", "Tag associated with Dash spells");

	// PLAYER CLASSES //
	CreateAndAssignGameplayTag(GameplayTags.PlayerClass_Triangle, "PlayerClass.Triangle", "DPS class");
	CreateAndAssignGameplayTag(GameplayTags.PlayerClass_Circle, "PlayerClass.Circle", "Healer class");
	CreateAndAssignGameplayTag(GameplayTags.PlayerClass_Square, "PlayerClass.Square", "Tank class");

	// BUFF STATUS //
	CreateAndAssignGameplayTag(GameplayTags.Status_Buff_DamageBoost, "Status.Buff.DamageBoost", "Damage boost buff");
	CreateAndAssignGameplayTag(GameplayTags.Status_Buff_Tankiness, "Status.Buff.Tankiness", "Tankiness buff");
	CreateAndAssignGameplayTag(GameplayTags.Status_Buff_HealBoost, "Status.Buff.HealBoost", "Heal boost buff");
	CreateAndAssignGameplayTag(GameplayTags.Status_Buff_Speed, "Status.Buff.Speed", "Movement speed buff");
	CreateAndAssignGameplayTag(GameplayTags.Status_Buff_Heal, "Status.Buff.Heal", "Healing buff");
	CreateAndAssignGameplayTag(GameplayTags.Status_Buff_Shield, "Status.Buff.Shield", "Shield buff");
}
