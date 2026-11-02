// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Gem/GeoCorePassiveAbility.h"
#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/AttributeSet/GeoGemAttributeSet.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Data/AbilityInfo.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Actor/Arena/GeoArena.h"
#include "Actor/GeoGemDrop.h"
#include "Algo/Count.h"
#include "Characters/Component/GeoDeployableManagerComponent.h"
#include "Characters/PlayableCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Curves/CurveFloat.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/WorldSettings.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "Gem/GeoGemCatalog.h"
#include "Gem/GeoGemMeshes.h"
#include "Gem/GeoGemProfileSave.h"
#include "Gem/GeoGemStatsEffect.h"
#include "Misc/AutomationTest.h"
#include "Settings/GameDataSettings.h"
#include "Tests/Gameplay/Fake/GeoTestDeployable.h"
#include "Tests/Gameplay/Fake/GeoTestGemDrop.h"
#include "Tests/Gameplay/Fake/GeoTestWorld.h"

#if WITH_DEV_AUTOMATION_TESTS

inline FGeoGemInfo MakeGem(FName Id, FGameplayAttribute Attribute = FGameplayAttribute(),
						   EGeoGemOperation Operation = EGeoGemOperation::Add, float MagnitudePerGem = 0.f)
{
	FGeoGemInfo Gem;
	Gem.Id = Id;
	Gem.Attribute = Attribute;
	Gem.Operation = Operation;
	Gem.MagnitudePerGem = MagnitudePerGem;
	return Gem;
}

/** Three Chips, a Cut, a Prism and two Cores, with the doc's drop tables and 1000 XP per echo level, free of any
 *  asset. */
inline UGeoGemCatalog* MakeTestCatalog()
{
	UGeoGemCatalog* Catalog = NewObject<UGeoGemCatalog>();
	Catalog->GemsByTier[EGeoGemTier::Chip].Gems = {
		MakeGem("Power", UCharacterAttributeSet::GetDamageMultiplierAttribute(), EGeoGemOperation::Add, 0.002f),
		MakeGem("Guard", UCharacterAttributeSet::GetDamageReductionAttribute(), EGeoGemOperation::Add, 0.002f),
		MakeGem("Vigor", UCharacterAttributeSet::GetMaxHealthAttribute(), EGeoGemOperation::Percent, 0.0035f),
	};
	Catalog->GemsByTier[EGeoGemTier::Cut].Gems = {
		MakeGem("Magazine", UCharacterAttributeSet::GetMaxAmmoAttribute(), EGeoGemOperation::Percent, 0.012f)};
	Catalog->GemsByTier[EGeoGemTier::Prism].Gems = {
		MakeGem("Anchor", UGeoGemAttributeSet::GetDeployableHealthMultiplierAttribute(), EGeoGemOperation::Add, 0.03f)};
	FGeoGemInfo Critical =
		MakeGem("Critical", UGeoGemAttributeSet::GetCritDamageAttribute(), EGeoGemOperation::Add, 0.3f);
	Critical.GrantedTag = FGeoGameplayTags::Get().Gem_Core_Critical;
	Catalog->GemsByTier[EGeoGemTier::Core].Gems = {MakeGem("Surplus"), Critical};

	FGeoGemDropTable& BossDrops = Catalog->DropTables.Add(EGeoBossType::Boss);
	BossDrops.MinCount = 38;
	BossDrops.MaxCount = 50;
	BossDrops.TierChances = {{EGeoGemTier::Prism, 0.03f}, {EGeoGemTier::Cut, 0.16f}};
	FGeoGemDropTable& MiniBossDrops = Catalog->DropTables.Add(EGeoBossType::MiniBoss);
	MiniBossDrops.MinCount = 38;
	MiniBossDrops.MaxCount = 50;
	MiniBossDrops.TierChances = {{EGeoGemTier::Cut, 0.16f}};

	UCurveFloat* EchoXpCurve = NewObject<UCurveFloat>(Catalog);
	EchoXpCurve->FloatCurve.AddKey(2.f, 1000.f);
	Catalog->EchoXpCurve = EchoXpCurve;
	return Catalog;
}

/** Index of the first socket of Tier that opens at exactly UnlockLevel. */
inline int32 FindSocket(EGeoGemTier Tier, int32 UnlockLevel)
{
	return GeoGem::GetSockets().IndexOfByPredicate(
		[Tier, UnlockLevel](FGeoGemSocket const& Socket)
		{
			return Socket.Tier == Tier && Socket.UnlockLevel == UnlockLevel;
		});
}

/** Gems of a reward, all stacks summed. */
inline int32 CountGems(FGeoGemReward const& Reward)
{
	int32 Total = 0;
	for (FGeoGemStack const& Stack : Reward.Gems)
	{
		Total += Stack.Count;
	}
	return Total;
}

#endif
