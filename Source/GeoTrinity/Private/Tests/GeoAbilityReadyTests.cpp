// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Base/GeoGameplayAbility.h"
#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Characters/Component/GeoDeployableManagerComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

constexpr EAutomationTestFlags GeoAbilityReadyTestFlags =
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

/** CDO of the ability Blueprint at /Game/AbilitySystem/Abilities/<Path>, as the HUD sees it on a client. */
static UGeoGameplayAbility const* LoadAbilityCDO(FString const& Path)
{
	FString const AssetName = FPaths::GetBaseFilename(Path);
	UClass* AbilityClass = LoadClass<UGeoGameplayAbility>(
		nullptr, *FString::Printf(TEXT("/Game/AbilitySystem/Abilities/%s.%s_C"), *Path, *AssetName));
	return AbilityClass ? AbilityClass->GetDefaultObject<UGeoGameplayAbility>() : nullptr;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoAbilityReadyTest, "GeoTrinity.AbilityReady.Rules", GeoAbilityReadyTestFlags)

bool FGeoAbilityReadyTest::RunTest(FString const& /*Parameters*/)
{
	UGeoGameplayAbility const* Bond = LoadAbilityCDO(TEXT("Gem/GA_Core_Bond"));
	UGeoGameplayAbility const* Wake = LoadAbilityCDO(TEXT("Gem/GA_Core_Wake"));
	UGeoGameplayAbility const* SweetSpot = LoadAbilityCDO(TEXT("Circle/Passive/GA_SweetSpotCharge"));
	UGeoGameplayAbility const* HealReturn = LoadAbilityCDO(TEXT("Circle/Passive/GA_HealsYouWhatYouHeal"));
	UGeoGameplayAbility const* Detonate =
		LoadAbilityCDO(TEXT("Square/SacrificeBeam/GA_Square_Special_SacrificeDetonate"));
	UGeoGameplayAbility const* Recall = LoadAbilityCDO(TEXT("Triangle/TurretRecall/GA_TurretRecall"));
	if (!TestTrue(TEXT("Every ability Blueprint loads"),
				  Bond && Wake && SweetSpot && HealReturn && Detonate && Recall))
	{
		return false;
	}

	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
	WorldContext.SetCurrentWorld(World);

	AActor* Character = World->SpawnActor<AActor>();
	UAbilitySystemComponent* ASC = NewObject<UAbilitySystemComponent>(Character);
	ASC->RegisterComponent();
	ASC->AddAttributeSetSubobject(NewObject<UCharacterAttributeSet>(Character));
	ASC->InitAbilityActorInfo(Character, Character);
	NewObject<UGeoDeployableManagerComponent>(Character)->RegisterComponent();

	float Fill = 0.f;
	bool bGaugeReady = false;
	FText Label;

	TestFalse(TEXT("Bond: passive without cooldown, never lit"), Bond->IsReady(*ASC));
	TestFalse(TEXT("HealReturn: passive without cooldown, never lit"), HealReturn->IsReady(*ASC));

	TestTrue(TEXT("Wake: lit off cooldown"), Wake->IsReady(*ASC));
	FGameplayTagContainer const* WakeCooldownTags = Wake->GetCooldownTags();
	if (TestTrue(TEXT("Wake has a cooldown tag"), WakeCooldownTags && !WakeCooldownTags->IsEmpty()))
	{
		ASC->AddLooseGameplayTags(*WakeCooldownTags);
		TestFalse(TEXT("Wake: unlit while its cooldown runs"), Wake->IsReady(*ASC));
		ASC->RemoveLooseGameplayTags(*WakeCooldownTags);
	}

	ASC->SetNumericAttributeBase(UCharacterAttributeSet::GetHealChargeAttribute(), 0.f);
	TestFalse(TEXT("Sweet-spot: unlit at 0 heals"), SweetSpot->IsReady(*ASC));
	ASC->SetNumericAttributeBase(UCharacterAttributeSet::GetHealChargeAttribute(), 150.f);
	TestTrue(TEXT("Sweet-spot feeds the class gauge"), SweetSpot->GetClassGauge(*ASC, Fill, bGaugeReady));
	TestEqual(TEXT("Sweet-spot: half full at 150 heals"), Fill, 0.5f, KINDA_SMALL_NUMBER);
	TestFalse(TEXT("Sweet-spot: gauge not ready at 150 heals"), bGaugeReady);
	TestTrue(TEXT("Sweet-spot writes a label"), SweetSpot->GetGaugeLabel(*ASC, Label));
	TestEqual(TEXT("Sweet-spot: label 50% at 150 heals"), Label.ToString(), FString(TEXT("50%")));
	ASC->SetNumericAttributeBase(UCharacterAttributeSet::GetHealChargeAttribute(), 300.f);
	TestTrue(TEXT("Sweet-spot: lit at 300 heals"), SweetSpot->IsReady(*ASC));

	FGameplayTag const DetonateReady = FGeoGameplayTags::Get().Status_Square_DetonateReady;
	ASC->SetNumericAttributeBase(UCharacterAttributeSet::GetSacrificeValueAttribute(), 100.f);
	TestFalse(TEXT("Detonate: unlit while not armed"), Detonate->IsReady(*ASC));
	ASC->AddLooseGameplayTag(DetonateReady);
	TestTrue(TEXT("Detonate: lit while armed, whatever the value"), Detonate->IsReady(*ASC));
	TestTrue(TEXT("Detonate feeds the class gauge"), Detonate->GetClassGauge(*ASC, Fill, bGaugeReady));
	TestEqual(TEXT("Detonate: 100 sacrificed = half the gauge"), Fill, 0.5f, KINDA_SMALL_NUMBER);
	TestTrue(TEXT("Detonate: gauge ready while armed at half fill"), bGaugeReady);
	ASC->RemoveLooseGameplayTag(DetonateReady);
	ASC->SetNumericAttributeBase(UCharacterAttributeSet::GetSacrificeValueAttribute(), 0.f);
	TestFalse(TEXT("Detonate: no label at 0 sacrificed"), Detonate->GetGaugeLabel(*ASC, Label));
	ASC->SetNumericAttributeBase(UCharacterAttributeSet::GetSacrificeValueAttribute(), 120.f);
	TestTrue(TEXT("Detonate: label while sacrificed"), Detonate->GetGaugeLabel(*ASC, Label));
	TestEqual(TEXT("Detonate: label = sacrificed value"), Label.ToString(), FString(TEXT("120")));

	TestFalse(TEXT("Recall: no label without a turret"), Recall->GetGaugeLabel(*ASC, Label));

	TestFalse(TEXT("Bond feeds no class gauge"), Bond->GetClassGauge(*ASC, Fill, bGaugeReady));
	TestFalse(TEXT("Bond writes no label"), Bond->GetGaugeLabel(*ASC, Label));

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}

#endif
