// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Base/PatternAbility.h"
#include "AbilitySystem/Abilities/Boss/GeoPeriodicPatternAbility.h"
#include "AbilitySystem/Abilities/Pattern/Hazard/BeamPattern.h"
#include "AbilitySystem/Abilities/Pattern/Hazard/Zone/SpawnAddPattern.h"
#include "AbilitySystem/Abilities/Pattern/Hazard/Zone/SpawnPillarPattern.h"
#include "AbilitySystem/Abilities/Pattern/Hazard/Zone/ZonePattern.h"
#include "AbilitySystem/Abilities/Pattern/Pattern.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "Actor/Deployable/Pillar/GeoPillar.h"
#include "Animation/AnimMontage.h"
#include "Characters/EnemyCharacter.h"
#include "CoreMinimal.h"
#include "Tests/Gameplay/Fake/GeoTestDeployable.h"
#include "Tool/GeoDifficulty.h"
#include "Tool/UGeoGameplayLibrary.h"

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

/** Pattern of a test recording its launches, read by the target tests: InitPattern only counts and keeps the payload,
 * so the pattern never runs nor ends. */
UCLASS(NotPlaceable, Transient)
class UGeoTestTargetPattern : public UGeoTestBarePattern
{
	GENERATED_BODY()

public:
	virtual void InitPattern(FAbilityPayload const& Payload,
							 TInstancedStruct<FPatternData> const& /*PatternData*/) override
	{
		StoredPayload = Payload;
		++LaunchCount;
	}

	FAbilityPayload const& GetLaunchedPayload() const { return StoredPayload; }
	/** GetTargetYaw, for the test. */
	float GetYawTowards(int32 const Index) const { return GetTargetYaw(Index); }

	int32 LaunchCount = 0;
};

/** Pattern ability of a test launching UGeoTestTargetPattern at whatever Target the test sets. */
UCLASS(NotPlaceable, Transient)
class UGeoTestTargetPatternAbility : public UPatternAbility
{
	GENERATED_BODY()

public:
	UGeoTestTargetPatternAbility()
	{
		InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
		NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
		PatternToLaunch = UGeoTestTargetPattern::StaticClass();
	}

	void SetTarget(FGeoTarget const& NewTarget) { Target = NewTarget; }

	/** Launches PreLaunchDelay seconds after activation, marking with a cue carrying CueTag meanwhile. */
	void SetPreLaunch(float const Delay, FGameplayTag const CueTag)
	{
		PreLaunchDelay = Delay;
		PreLaunchCue.CueTag = CueTag;
	}

	/** AddPreLaunchCue, for the test: a second pick on an already marked actor. */
	void MarkForPreLaunch(UGeoAbilitySystemComponent* const TargetASC) { AddPreLaunchCue(TargetASC); }

	/** How many ASCs the pre-launch cue marks right now. */
	int32 GetPreLaunchMarkCount() const { return PreLaunchCueASCs.Num(); }
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
	/** The spell tag used by test abilities so an encounter action can find them by tag. */
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
	/** Where the launch would have landed: its Target's locations, resolved. */
	TArray<FVector2D> LaunchedLocations;

protected:
	virtual void LaunchPattern() override
	{
		LaunchedPatternData = CreatePatternData();
		StoredPayload = CreateAbilityPayload(LaunchSeed);
		LaunchedLocations = GeoASLib::GetTargetLocations(Target, MakeTargetContext());
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

	/** Targets the point tagged Tag in the activation's room. */
	void SetPointTag(FGameplayTag const Tag)
	{
		Target.Mode = EGeoTarget::TargetPoint;
		Target.TargetPointTag = Tag;
	}
};

/** Salve ability whose aim a test reads, for a salve leaving the world origin at the given Target mode. */
UCLASS(NotPlaceable, Transient)
class UGeoTestSalveAbility : public UGeoPeriodicPatternAbility
{
	GENERATED_BODY()

public:
	/** Returns the target locations the salve would aim at in Mode, launching from the world origin. */
	TArray<FVector2D> GetAimLocations(EGeoTarget const Mode)
	{
		Target.Mode = Mode;
		StoredPayload.Origin = FVector2D::ZeroVector;
		return GeoASLib::GetTargetLocations(Target, MakeTargetContext());
	}
};

/** Launches Pattern from Caster at Locations as PatternStartMulticast would, its wind-up just started. */
inline void LaunchTestPattern(UPattern& Pattern, AActor* const Caster, TArray<FVector2D> const& Locations,
							  int32 const Level = GeoDifficultyMask::Original)
{
	FAbilityPayload Payload;
	Payload.Origin = FVector2D(Caster->GetActorLocation());
	Payload.ServerSpawnTime = GeoLib::GetServerTime(Caster->GetWorld(), true);
	Payload.AbilityLevel = Level;
	Payload.SourceOwner = Caster;
	Payload.SourceAvatar = Caster;
	Pattern.SetTargetLocations(Locations, Payload.Origin);
	Pattern.InitPattern(Payload, TInstancedStruct<FPatternData>());
}

/** Beam of a test, never launched: reads its yaw along the sweep, from the world origin. */
UCLASS(NotPlaceable, Transient)
class UGeoTestBeamPattern : public UBeamPattern
{
	GENERATED_BODY()

public:
	void Aim(float const NewSweepAngle, int32 const Seed, FVector2D const& TargetLocation)
	{
		SweepAngle = NewSweepAngle;
		StoredPayload.Seed = Seed;
		SetTargetLocations({TargetLocation}, StoredPayload.Origin);
	}

	/** GetBeamYaw, Fraction of the way through the sweep. */
	float GetYawAtFraction(float const Fraction) const { return GetBeamYaw(Fraction * BeamDuration, 0.f); }
};

/** Burst zone of a test, radius 100, with a 10 s wind-up the test never waits for. */
UCLASS(NotPlaceable, Transient)
class UGeoTestZonePattern : public UZonePattern
{
	GENERATED_BODY()

public:
	virtual void OnCreate(FGameplayTag, AActor&) override
	{
		StartDelay = 10.f;
		bHasHazard = true;
		ZoneParams.Size = 100.f;
		ZoneParams.Color.Color = EGeoColor::Damage;
	}

	int32 GetTelegraphCount() const { return IndicatorComponents.Num(); }
	bool IsLocationInHazard(AActor const* Target, FVector2D const Location) const
	{
		return IsInHazard(Target, Location, 0.f);
	}
};

/** Pillar with working attributes, the way BP_Pillar_Deployable's would be. */
UCLASS(NotPlaceable, Transient)
class AGeoTestPillar : public AGeoPillar
{
	GENERATED_BODY()

public:
	AGeoTestPillar(FObjectInitializer const& ObjectInitializer) : Super(ObjectInitializer)
	{
		Cast<UGeoAbilitySystemComponent>(GetAbilitySystemComponent())->DefaultAttributes =
			UGeoTestDeployableAttributes::StaticClass();
	}
};

/** Pillar pattern of a test: AGeoTestPillar, with a 10 s wind-up the test never waits for. */
UCLASS(NotPlaceable, Transient)
class UGeoTestPillarPattern : public USpawnPillarPattern
{
	GENERATED_BODY()

public:
	UGeoTestPillarPattern() { PillarClass = AGeoTestPillar::StaticClass(); }

	virtual void OnCreate(FGameplayTag, AActor&) override
	{
		StartDelay = 10.f;
		bHasHazard = true;
		ZoneParams.Color.Color = EGeoColor::Damage;
	}
};

/** Add pattern of a test: brings a plain enemy, tagged "Summoned" with 300 max health by AddSetup, "SafeSummoned" by
 * SafeAddSetup, with a 10 s wind-up the test never waits for. */
UCLASS(NotPlaceable, Transient)
class UGeoTestSpawnAddPattern : public USpawnAddPattern
{
	GENERATED_BODY()

public:
	UGeoTestSpawnAddPattern()
	{
		AddClass = AEnemyCharacter::StaticClass();
		AddSetup.Tag = TEXT("Summoned");
		AddSetup.MaxHealth = FScalableFloat(300.f);
		bUseSafeAddSetup = true;
		SafeAddSetup.Tag = TEXT("SafeSummoned");
	}

	virtual void OnCreate(FGameplayTag, AActor&) override
	{
		StartDelay = 10.f;
		bHasHazard = true;
		ZoneParams.Color.Color = EGeoColor::Damage;
	}
};
