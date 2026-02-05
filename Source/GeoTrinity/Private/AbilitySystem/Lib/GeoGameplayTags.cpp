#include "AbilitySystem/Lib/GeoGameplayTags.h"

#include "GameplayTagsManager.h"


FGeoGameplayTags FGeoGameplayTags::GameplayTags;

namespace
{
	void CreateAndAssignGameplayTag(FGameplayTag& outTag, FName const& tagName, FString const& comment)
	{
		outTag = UGameplayTagsManager::Get().AddNativeGameplayTag(tagName, comment);
	}

	// INPUT //
	void AddInputTag(FGameplayTag& outTag, FString const& tagName, FString const& comment)
	{
		CreateAndAssignGameplayTag(outTag, FName(RootTagNames::InputTag + "." + tagName), comment);
	}

	// ABILITY //
	void AddAbilityTag(FGameplayTag& outTag, FString const& tagName, FString const& comment)
	{
		CreateAndAssignGameplayTag(outTag, FName(RootTagNames::AbilityTag + "." + tagName), comment);
	}
	void AddAbilityIDTag(FGameplayTag& outTag, FString const& tagName, FString const& comment)
	{
		CreateAndAssignGameplayTag(outTag, FName(RootTagNames::AbilityIDTag + "." + tagName), comment);
	}
} // namespace

void FGeoGameplayTags::InitializeNativeGameplayTags()
{
	CreateAndAssignGameplayTag(GameplayTags.Gameplay_Damage, "Gameplay.Damage", "Tag to identify damage");

	// INPUT //
	AddInputTag(GameplayTags.InputTag_BasicSpell, "BasicSpell", "Input tag for left mouse button-linked spell");
	AddInputTag(GameplayTags.InputTag_SpecialSpell, "SpecialSpell", "Input tag for right mouse button-linked spell");
	AddInputTag(GameplayTags.InputTag_Reload, "Reload", "Input tag for reload action");
	AddInputTag(GameplayTags.InputTag_Deploy, "Deploy", "Input tag for deploying objects");
	AddInputTag(GameplayTags.InputTag_AltSpecial, "AltSpecial", "Input tag for alternative special ability");
	AddInputTag(GameplayTags.InputTag_Dash, "Dash", "Input tag for dash ability");

	// TYPE OF ABILITY //
	AddAbilityTag(GameplayTags.Ability_Type_Basic, "Type.Basic", "Tag associated with basic spells");
	AddAbilityTag(GameplayTags.Ability_Type_Special, "Type.Special", "Tag associated with special spells");

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
