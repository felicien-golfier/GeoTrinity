// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Base/PatternAbility.h"
#include "AbilitySystem/Abilities/Boss/GeoPeriodicPatternAbility.h"
#include "AbilitySystem/Abilities/Boss/GeoSummonAddAbility.h"
#include "AbilitySystem/Abilities/Pattern/Pattern.h"
#include "AbilitySystem/Abilities/Pattern/Projectile/ProjectilePattern.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "Animation/AnimMontage.h"
#include "Characters/EnemyCharacter.h"
#include "CoreMinimal.h"

#include "GeoTestPatternAbility.generated.h"

/** Pattern of a test that plays a montage of its own. */
UCLASS(NotPlaceable, Transient)
class UGeoTestMontagePattern : public UPattern
{
	GENERATED_BODY()

public:
	UGeoTestMontagePattern() { AnimMontage = CreateDefaultSubobject<UAnimMontage>(TEXT("Montage")); }
};

/** Pattern a test creates by hand: no ability tag, so no ability CDO to read. */
UCLASS(NotPlaceable, Transient)
class UGeoTestBarePattern : public UPattern
{
	GENERATED_BODY()

public:
	virtual void OnCreate(FGameplayTag, AActor&) override {}
};

/** Pattern of a test with no montage. */
UCLASS(NotPlaceable, Transient)
class UGeoTestSilentPattern : public UPattern
{
	GENERATED_BODY()
};

/** Pattern ability of a test launching UGeoTestMontagePattern. */
UCLASS(NotPlaceable, Transient)
class UGeoTestMontagePatternAbility : public UPatternAbility
{
	GENERATED_BODY()

public:
	UGeoTestMontagePatternAbility() { PatternToLaunch = UGeoTestMontagePattern::StaticClass(); }
};

/** Ability of a test that stays active until the test cancels it. Its asset tag (a spell tag,
 * Ability.Spell.ShieldBurst: abilities are looked up by their Ability.Spell tag) is set when granted: native tags do
 * not exist yet in a constructor. */
UCLASS(NotPlaceable, Transient)
class UGeoTestCastAbility : public UGeoGameplayAbility
{
	GENERATED_BODY()

public:
	static FGameplayTag GetCastTag() { return FGeoGameplayTags::Get().Ability_Spell_ShieldBurst; }

	virtual void ActivateAbility(FGameplayAbilitySpecHandle /*Handle*/, FGameplayAbilityActorInfo const* /*ActorInfo*/,
								 FGameplayAbilityActivationInfo /*ActivationInfo*/,
								 FGameplayEventData const* /*TriggerEventData*/) override
	{
	}

protected:
#if WITH_EDITOR
	virtual void OnGiveAbility(FGameplayAbilityActorInfo const* ActorInfo, FGameplayAbilitySpec const& Spec) override
	{
		EditorGetAssetTags().AddTag(GetCastTag());
		Super::OnGiveAbility(ActorInfo, Spec);
	}
#endif
};

/** UGeoTestCastAbility that needs its caster's arena: the encounters' shared caster refuses it. */
UCLASS(NotPlaceable, Transient)
class UGeoTestArenaBoundAbility : public UGeoTestCastAbility
{
	GENERATED_BODY()

public:
	virtual bool RequiresArena() const override { return true; }
};

/** Pattern ability of a test launching UGeoTestSilentPattern. */
UCLASS(NotPlaceable, Transient)
class UGeoTestSilentPatternAbility : public UPatternAbility
{
	GENERATED_BODY()

public:
	UGeoTestSilentPatternAbility() { PatternToLaunch = UGeoTestSilentPattern::StaticClass(); }
};

/** Pattern ability of a test that records the pattern data it would launch its pattern with, and launches nothing.
 * Tagged like UGeoTestCastAbility so an encounter action finds it. */
UCLASS(NotPlaceable, Transient)
class UGeoTestDataPatternAbility : public UPatternAbility
{
	GENERATED_BODY()

public:
	UGeoTestDataPatternAbility() { PatternToLaunch = UGeoTestSilentPattern::StaticClass(); }

	TInstancedStruct<FPatternData> LaunchedPatternData;
	/** Where the launch would have left from (GetFireOrigin2D). */
	FVector2D LaunchedOrigin = FVector2D::ZeroVector;

protected:
	virtual void LaunchPattern() override
	{
		LaunchedPatternData = CreatePatternData();
		LaunchedOrigin = CreateAbilityPayload(LaunchSeed).Origin;
	}

	/** No pattern was launched: skips UPatternAbility's stop of it. */
	virtual void EndAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
							FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
							bool bWasCancelled) override
	{
		UGeoGameplayAbility::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	}

#if WITH_EDITOR
	virtual void OnGiveAbility(FGameplayAbilityActorInfo const* ActorInfo, FGameplayAbilitySpec const& Spec) override
	{
		EditorGetAssetTags().AddTag(UGeoTestCastAbility::GetCastTag());
		Super::OnGiveAbility(ActorInfo, Spec);
	}
#endif
};

/** UGeoTestDataPatternAbility launching from a point of its activation's room (FPatternAbilityParams::ArenaTag). The
 * tag is set on its one instance, once granted: native tags do not exist yet in a constructor. */
UCLASS(NotPlaceable, Transient)
class UGeoTestPointPatternAbility : public UGeoTestDataPatternAbility
{
	GENERATED_BODY()

public:
	UGeoTestPointPatternAbility() { InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor; }

	void SetPointTag(FGameplayTag const Tag) { TargetPointTag = Tag; }
};

/** Salve ability whose aim a test reads, for a salve leaving the world origin at the given Target. */
UCLASS(NotPlaceable, Transient)
class UGeoTestSalveAbility : public UGeoPeriodicPatternAbility
{
	GENERATED_BODY()

public:
	TArray<float> GetAimYaws(EProjectileTarget const AimTarget)
	{
		Target = AimTarget;
		return CreatePatternData().Get<FProjectilePatternData>().Yaws;
	}
};

/** Summon ability bringing a plain enemy, tagged "Summoned" with 300 max health by its AddSetup. Summon runs the end
 * of its pattern at once; the test creates the UGeoTestBarePattern instance that end looks for. */
UCLASS(NotPlaceable, Transient)
class UGeoTestSummonAddAbility : public UGeoSummonAddAbility
{
	GENERATED_BODY()

public:
	UGeoTestSummonAddAbility()
	{
		InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
		PatternToLaunch = UGeoTestBarePattern::StaticClass();
		AddClass = AEnemyCharacter::StaticClass();
		AddSetup.Tag = TEXT("Summoned");
		AddSetup.MaxHealth = FScalableFloat(300.f);
	}

	void Summon(FVector2D const Origin)
	{
		StoredPayload.SourceAvatar = GetAvatarActorFromActorInfo();
		StoredPayload.Origin = Origin;
		OnPatternEnd();
	}
};
