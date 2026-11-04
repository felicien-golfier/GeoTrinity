// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Gem/GeoCorePassiveAbility.h"
#include "AbilitySystem/Abilities/Pattern/Hazard/LateralWavePattern.h"
#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/AttributeSet/GeoGemAttributeSet.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Data/AbilityInfo.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Actor/Arena/GeoArena.h"
#include "Actor/Buttons/GeoTeleporter.h"
#include "Actor/Deployable/Zones/GeoEffectZone.h"
#include "Actor/GeoCat.h"
#include "Actor/GeoGemDrop.h"
#include "Actor/GeoTargetPoint.h"
#include "Algo/Count.h"
#include "Characters/Component/GeoDeployableManagerComponent.h"
#include "Characters/EnemyCharacter.h"
#include "Characters/GeoEnemySetup.h"
#include "Characters/PlayableCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Curves/CurveFloat.h"
#include "Encounter/Action/Cat/GeoCatMontageAction.h"
#include "Encounter/Action/Cat/GeoCatRoamAction.h"
#include "Encounter/Action/Cat/GeoCatWalkAction.h"
#include "Encounter/Action/GeoActivateAbilityAction.h"
#include "Encounter/Action/GeoAddToCounterAction.h"
#include "Encounter/Action/GeoCatLineAction.h"
#include "Encounter/Action/GeoCatLineFromFileAction.h"
#include "Encounter/Action/GeoDelayAction.h"
#include "Encounter/Action/GeoQuestCardAction.h"
#include "Encounter/Action/GeoSetActorHiddenAction.h"
#include "Encounter/Action/GeoSetMovementLockedAction.h"
#include "Encounter/Action/GeoSetTeleporterOpenAction.h"
#include "Encounter/Action/GeoShowAbilityPageAction.h"
#include "Encounter/Action/GeoSpawnEnemiesAction.h"
#include "Encounter/GeoEncounterSubsystem.h"
#include "Encounter/Trigger/GeoAbilityActivatedTrigger.h"
#include "Encounter/Trigger/GeoActorDiedTrigger.h"
#include "Encounter/Trigger/GeoCounterTrigger.h"
#include "Encounter/Trigger/GeoEventTrigger.h"
#include "Encounter/Trigger/GeoHealthThresholdTrigger.h"
#include "Encounter/Trigger/GeoPlayerIdleTrigger.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameClasses/Match/GeoGameState.h"
#include "GameFramework/DefaultPawn.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/WorldSettings.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "GameplayTagsManager.h"
#include "Gem/GeoGemCatalog.h"
#include "Gem/GeoGemMeshes.h"
#include "Gem/GeoGemProfileSave.h"
#include "Gem/GeoGemStatsEffect.h"
#include "HAL/FileManager.h"
#include "Materials/Material.h"
#include "Misc/AutomationTest.h"
#include "Tests/Gameplay/GeoTestFlags.h"
#include "Misc/DataValidation.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Settings/GameDataSettings.h"
#include "StateTree.h"
#include "Tests/Gameplay/Fake/GeoTestDeployable.h"
#include "Tests/Gameplay/Fake/GeoTestEncounter.h"
#include "Tests/Gameplay/Fake/GeoTestEnemy.h"
#include "Tests/Gameplay/Fake/GeoTestGemDrop.h"
#include "Tests/Gameplay/Fake/GeoTestPatternAbility.h"
#include "Tests/Gameplay/Fake/GeoTestWorld.h"
#include "Tool/CatLine/GeoCatLineBank.h"
#include "Tool/GeoDifficulty.h"
#include "Tool/UGeoGameplayLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoDeployableSelfOwnedSpawnTest, "GeoTrinity.Deployable.SelfOwnedSpawn",
								 GeoDeployableTestFlags)

bool FGeoDeployableSelfOwnedSpawnTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	FDeployableData Data;
	Data.Params.LifeDrainMaxDuration = 10.f;
	AGeoDeployableBase* const Deployable =
		GeoASLib::FullySpawnDeployable(AGeoTestDeployable::StaticClass(), Data, FTransform::Identity, World);
	if (TestNotNull(TEXT("A deployable with no owner spawns"), Deployable))
	{
		TestTrue(TEXT("It is its own owner"), Data.Owner == Deployable);
		TestTrue(TEXT("And its own instigator"), Data.Instigator == Deployable);
		TestTrue(TEXT("It is active, with its own ability system"),
				 Deployable->IsActive() && Deployable->GetAbilitySystemComponent() != nullptr);
	}

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoDeployableStartHealthTest, "GeoTrinity.Deployable.StartHealth",
								 GeoDeployableTestFlags)

bool FGeoDeployableStartHealthTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();

	APawn* Deployer = World->SpawnActor<AGeoTestDeployer>();
	auto const Spawn = [Deployer](float const StartHealthRatio)
	{
		FDeployableData Data;
		Data.Owner = Deployer;
		Data.Instigator = Deployer;
		Data.Params.StartHealthRatio = StartHealthRatio;
		Data.Params.LifeDrainMaxDuration = 10.f;
		return GeoASLib::FullySpawnDeployable(AGeoTestDeployable::StaticClass(), Data, FTransform::Identity);
	};
	auto const Attribute = [](AGeoDeployableBase const* Deployable, FGameplayAttribute const& Which)
	{
		return Deployable->GetAbilitySystemComponent()->GetNumericAttribute(Which);
	};

	AGeoDeployableBase const* Full = Spawn(1.f);
	AGeoDeployableBase const* Half = Spawn(0.5f);
	if (TestNotNull(TEXT("Both deployables spawn"), Full) && TestNotNull(TEXT("Both deployables spawn"), Half))
	{
		float const MaxHealth = Attribute(Full, UGeoAttributeSetBase::GetMaxHealthAttribute());
		TestTrue(TEXT("Default attributes gave a max health"), MaxHealth > 0.f);
		TestEqual(TEXT("Ratio 1 starts full"), Attribute(Full, UGeoAttributeSetBase::GetHealthAttribute()), MaxHealth);
		TestEqual(TEXT("Ratio 0.5 keeps the max health"),
				  Attribute(Half, UGeoAttributeSetBase::GetMaxHealthAttribute()), MaxHealth);
		TestEqual(TEXT("Ratio 0.5 starts at half"), Attribute(Half, UGeoAttributeSetBase::GetHealthAttribute()),
				  MaxHealth * 0.5f, KINDA_SMALL_NUMBER);
	}

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoDeployableStartHealthFromAbilityTest,
								 "GeoTrinity.Deployable.StartHealthFromAbility", GeoDeployableTestFlags)

bool FGeoDeployableStartHealthFromAbilityTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();

	AGeoTestDeployer* Deployer = World->SpawnActor<AGeoTestDeployer>();
	UGeoAbilitySystemComponent* ASC = Cast<UGeoAbilitySystemComponent>(Deployer->GetAbilitySystemComponent());
	ASC->GiveAbility(FGameplayAbilitySpec(UGeoTestDeployAbility::StaticClass(), 1));
	ASC->SetNumericAttributeBase(UGeoGemAttributeSet::GetDeployableHealthMultiplierAttribute(), 2.f);

	AGeoDeployableBase const* Copy = GeoASLib::SpawnDeployableFromAbility(*ASC, FVector::ZeroVector, 0.5f);
	if (TestNotNull(TEXT("The ability places the deployable"), Copy))
	{
		UAbilitySystemComponent const* CopyASC = Copy->GetAbilitySystemComponent();
		float const BaseMaxHealth = UGeoTestDeployableAttributes::MaxHealth;
		TestEqual(TEXT("The deployer's gems scale the max health (x2)"),
				  CopyASC->GetNumericAttribute(UGeoAttributeSetBase::GetMaxHealthAttribute()), BaseMaxHealth * 2.f);
		TestEqual(TEXT("Ratio 0.5 starts at half of that"),
				  CopyASC->GetNumericAttribute(UGeoAttributeSetBase::GetHealthAttribute()), BaseMaxHealth,
				  KINDA_SMALL_NUMBER);
	}

	DestroyTestWorld(World);
	return true;
}

#endif
