// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Common/GeoDeployAbility.h"
#include "AbilitySystem/AttributeSet/GeoGemAttributeSet.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "Actor/Deployable/GeoDeployableBase.h"
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

#include "GeoTestDeployable.generated.h"

/** Instant effect giving a test deployable 100 MaxHealth and Health, the way a deployable Blueprint's DefaultAttributes
 * would. */
UCLASS(NotPlaceable, Transient)
class UGeoTestDeployableAttributes : public UGameplayEffect
{
	GENERATED_BODY()

public:
	static constexpr float MaxHealth = 100.f;

	UGeoTestDeployableAttributes()
	{
		DurationPolicy = EGameplayEffectDurationType::Instant;
		for (FGameplayAttribute const& Attribute :
			 {UGeoAttributeSetBase::GetMaxHealthAttribute(), UGeoAttributeSetBase::GetHealthAttribute()})
		{
			FGameplayModifierInfo& Modifier = Modifiers.AddDefaulted_GetRef();
			Modifier.Attribute = Attribute;
			Modifier.ModifierOp = EGameplayModOp::Override;
			Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(MaxHealth));
		}
	}
};

/** Concrete stand-in for the abstract AGeoDeployableBase, spawned only by automation tests through
 * GeoASLib::FullySpawnDeployable (recipe: AI/Testing.md, "Spawning a deployable in a test"). It stores its own
 * FDeployableData and carries working DefaultAttributes, so a test sets only the params it is about. */
UCLASS(NotPlaceable, Transient)
class AGeoTestDeployable : public AGeoDeployableBase
{
	GENERATED_BODY()

public:
	AGeoTestDeployable(FObjectInitializer const& ObjectInitializer) : Super(ObjectInitializer)
	{
		Cast<UGeoAbilitySystemComponent>(GetAbilitySystemComponent())->DefaultAttributes =
			UGeoTestDeployableAttributes::StaticClass();
	}

	/** Copies Data into TestData so the deployable owns its lifetime, then initialises through Super. */
	virtual void InitInteractable(FInteractableActorData* Data) override
	{
		TestData = *static_cast<FDeployableData*>(Data);
		Super::InitInteractable(&TestData);
	}

protected:
	virtual FDeployableData const* GetData() const override { return &TestData; }

private:
	FDeployableData TestData;
};

/** Charge-pool effect of the test deploy ability: one stack per spent charge, each held 10 s, granting the cooldown
 * tag. Its stack limit stays unlimited, as the deploy ability requires. */
UCLASS(NotPlaceable, Transient)
class UGeoTestDeployCooldown : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGeoTestDeployCooldown()
	{
		DurationPolicy = EGameplayEffectDurationType::HasDuration;
		DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(10.f));
		PRAGMA_DISABLE_DEPRECATION_WARNINGS // SetStackingType is not exported
		StackingType = EGameplayEffectStackingType::AggregateByTarget;
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
		StackLimitCount = 0;
		GEComponents.Add(CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("CooldownTagsComponent")));
	}

	/** Gives the effect its cooldown tag. Native tags exist only after this class's constructor ran, so a test ability
	 * sets it when granted. */
	static void GrantCooldownTag()
	{
		FInheritedTagContainer CooldownTags;
		CooldownTags.AddTag(FGeoGameplayTags::Get().Ability_Cooldown);
		CastChecked<UTargetTagsGameplayEffectComponent>(GetMutableDefault<UGeoTestDeployCooldown>()->GEComponents[0])
			->SetAndApplyTargetTagChanges(CooldownTags);
	}
};

/** Deploy ability of a test: deploys AGeoTestDeployable, so a test goes through the ability path without a Blueprint.
 * Grant it with ASC->GiveAbility(FGameplayAbilitySpec(UGeoTestDeployAbility::StaticClass(), 1)). */
UCLASS(NotPlaceable, Transient)
class UGeoTestDeployAbility : public UGeoDeployAbility
{
	GENERATED_BODY()

public:
	UGeoTestDeployAbility()
	{
		CooldownGameplayEffectClass = UGeoTestDeployCooldown::StaticClass();
		DeployableActorClass = AGeoTestDeployable::StaticClass();
		Params.LifeDrainMaxDuration = 10.f;
	}

protected:
	virtual void OnGiveAbility(FGameplayAbilityActorInfo const* ActorInfo, FGameplayAbilitySpec const& Spec) override
	{
		UGeoTestDeployCooldown::GrantCooldownTag();
		Super::OnGiveAbility(ActorInfo, Spec);
	}
};

/** Pawn that deploys in a test: owns an ASC (gem attributes included) with itself as owner and avatar, which spawn
 * cues resolve their instigator through. Grant it abilities (a deploy ability, a Core passive) for a test that goes through the ability path. */
UCLASS(NotPlaceable, Transient)
class AGeoTestDeployer
	: public APawn
	, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AGeoTestDeployer(FObjectInitializer const& ObjectInitializer) : Super(ObjectInitializer)
	{
		AbilitySystemComponent = CreateDefaultSubobject<UGeoAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	}

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystemComponent; }

	/** Adds gem attributes and initialises ASC actor info with this pawn as both owner and avatar. */
	virtual void PostInitializeComponents() override
	{
		Super::PostInitializeComponents();
		AbilitySystemComponent->AddAttributeSetSubobject(NewObject<UGeoGemAttributeSet>(this));
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}

private:
	UPROPERTY()
	TObjectPtr<UGeoAbilitySystemComponent> AbilitySystemComponent;
};
