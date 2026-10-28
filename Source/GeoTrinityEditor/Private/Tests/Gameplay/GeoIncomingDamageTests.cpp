// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystemComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameClasses/Match/GeoGameState.h"
#include "GameplayEffect.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

constexpr EAutomationTestFlags GeoIncomingDamageTestFlags =
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

/** Applies an instant GE writing Amount to the victim's IncomingDamage meta attribute. */
static void ApplyIncomingDamage(UAbilitySystemComponent& VictimASC, float const Amount)
{
	UGameplayEffect* DamageEffect = NewObject<UGameplayEffect>(GetTransientPackage());
	DamageEffect->DurationPolicy = EGameplayEffectDurationType::Instant;
	FGameplayModifierInfo& Modifier = DamageEffect->Modifiers.AddDefaulted_GetRef();
	Modifier.Attribute = UGeoAttributeSetBase::GetIncomingDamageAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Amount));
	VictimASC.ApplyGameplayEffectToSelf(DamageEffect, 1.f, VictimASC.MakeEffectContext());
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoIncomingDamageStagesTest, "GeoTrinity.IncomingDamage.Stages",
								 GeoIncomingDamageTestFlags)

bool FGeoIncomingDamageStagesTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
	WorldContext.SetCurrentWorld(World);
	AGeoGameState* GameState = World->SpawnActor<AGeoGameState>();
	World->SetGameState(GameState);

	AActor* Victim = World->SpawnActor<AActor>();
	UAbilitySystemComponent* VictimASC = NewObject<UAbilitySystemComponent>(Victim);
	VictimASC->RegisterComponent();
	VictimASC->AddAttributeSetSubobject(NewObject<UGeoAttributeSetBase>(Victim));
	VictimASC->InitAbilityActorInfo(Victim, Victim);
	auto const ResetVictim = [VictimASC]
	{
		VictimASC->SetNumericAttributeBase(UGeoAttributeSetBase::GetMaxHealthAttribute(), 100.f);
		VictimASC->SetNumericAttributeBase(UGeoAttributeSetBase::GetHealthAttribute(), 100.f);
		VictimASC->SetNumericAttributeBase(UGeoAttributeSetBase::GetShieldAttribute(), 0.f);
	};

	// Stage 1 takes 40 and stage 2 takes 20 of the 60 left: the victim loses 40, and stage 2 saw what stage 1 left.
	float ShareSaw = -1.f;
	GameState->OnIncomingDamage.AddLambda(
		[](UAbilitySystemComponent&, FGameplayEffectContextHandle const&, float, float& Spared) { Spared += 40.f; });
	GameState->OnIncomingDamageShare.AddLambda(
		[&ShareSaw](UAbilitySystemComponent&, FGameplayEffectContextHandle const&, float const Damage, float& Spared)
		{
			ShareSaw = Damage;
			Spared += 20.f;
		});
	ResetVictim();
	ApplyIncomingDamage(*VictimASC, 100.f);
	TestEqual(TEXT("Stage 2 receives the damage stage 1 left"), ShareSaw, 60.f);
	TestEqual(TEXT("Victim loses what both stages left"),
			  VictimASC->GetNumericAttribute(UGeoAttributeSetBase::GetHealthAttribute()), 60.f);

	// Stage 1 takes the whole hit: stage 2 never runs, the victim is untouched.
	GameState->OnIncomingDamage.Clear();
	GameState->OnIncomingDamage.AddLambda(
		[](UAbilitySystemComponent&, FGameplayEffectContextHandle const&, float const Damage, float& Spared)
		{ Spared = Damage; });
	ShareSaw = -1.f;
	ResetVictim();
	ApplyIncomingDamage(*VictimASC, 100.f);
	TestEqual(TEXT("A hit taken over whole never reaches stage 2"), ShareSaw, -1.f);
	TestEqual(TEXT("A hit taken over whole leaves the victim whole"),
			  VictimASC->GetNumericAttribute(UGeoAttributeSetBase::GetHealthAttribute()), 100.f);

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
