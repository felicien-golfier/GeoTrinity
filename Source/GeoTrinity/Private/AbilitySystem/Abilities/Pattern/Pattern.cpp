#include "AbilitySystem/Abilities/Pattern/Pattern.h"

#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Tool/UGeoGameplayLibrary.h"

void UPattern::OnCreate(FGameplayTag AbilityTag, AActor&)
{
	checkf(
		EffectDataArray.IsEmpty(),
		TEXT(
			"EffectDataArray is not empty when creating a Pattern ! Ensure you call this function only at the spawn of the pattern."));
	UGeoGameplayAbility const* AbilityCDO = GeoASLib::GetAbilityCDO(AbilityTag);
	if (!AbilityCDO)
	{
		ensureMsgf(false, TEXT("Pattern %s has no AbilityCDO !"), *AbilityTag.ToString());
		return;
	}

	StartDelay = AbilityCDO->GetFireDelay();
	EffectDataArray = AbilityCDO->GetEffectDataArray();

	if (AnimMontage)
	{
		int StartSection = GeoASLib::GetAndCheckSection(AnimMontage, GeoASLib::SectionStartName);
		if (StartDelay == 0.f && StartSection != INDEX_NONE)
		{
			StartDelay = AnimMontage->GetSectionLength(StartSection);
		}

		(void)GeoASLib::GetAndCheckSection(AnimMontage, GeoASLib::SectionFireName);
		(void)GeoASLib::GetAndCheckSection(AnimMontage, GeoASLib::SectionEndName);
	}
}

void UPattern::InitPattern(FAbilityPayload const& Payload)
{
	if (bPatternIsActive)
	{
		UE_LOG(LogPattern, Error,
			   TEXT("Starting pattern when the Previous pattern of the same instance is still active !"));
		EndPattern();
	}

	bPatternIsActive = true;
	StoredPayload = Payload;
	StartTime = GeoLib::GetServerTime(GetWorld(), true) - Payload.ServerSpawnTime;
	bool const bIsServer = GeoLib::IsServer(GetWorld());

	UAnimInstance* AnimInstance = GeoASLib::GetAnimInstance(Payload);

	ExecuteDelayGameplayCue();

	if (StartTime > StartDelay)
	{
		UE_LOG(LogPattern, Warning, TEXT("We start the montage too late, starting from loop directly"));
		StartPattern();
	}
	else
	{
		if (!bIsServer && IsValid(AnimMontage) && IsValid(AnimInstance))
		{
			int const StartSection = GeoASLib::GetAndCheckSection(AnimMontage, GeoASLib::SectionStartName);
			float const PlayRate = AnimMontage->GetSectionLength(StartSection) / StartDelay;
			AnimInstance->Montage_Play(AnimMontage, PlayRate, EMontagePlayReturnType::MontageLength,
									   FMath::Max(0.f, StartTime));
		}

		float const RemainingStartTime = StartDelay - StartTime;
		GetWorld()->GetTimerManager().SetTimer(StartSectionTimerHandle, this, &UPattern::StartPattern,
											   RemainingStartTime);
	}
}

void UPattern::ExecuteDelayGameplayCue()
{
	bool const bIsServer = GeoLib::IsServer(GetWorld());
	if (DelayGameplayCueTag.IsValid() && !bIsServer)
	{
		UGeoAbilitySystemComponent* InstigatorASC = GeoASLib::GetGeoAscFromActor(StoredPayload.Instigator);
		if (!IsValid(InstigatorASC))
		{
			ensureMsgf(false, TEXT("Pattern Instigator %s has no ASC !"), *StoredPayload.Instigator->GetName());
		}
		else
		{
			FScopedPredictionWindow ScopedPredictionWindow(InstigatorASC);
			FGameplayCueParameters CueParams = FillCueParam(StoredPayload);
			InstigatorASC->ExecuteGameplayCue(DelayGameplayCueTag, CueParams);
		}
	}
}

FGameplayCueParameters UPattern::FillCueParam(FAbilityPayload const& Payload)
{
	FGameplayCueParameters CueParams;
	CueParams.Location = FVector(Payload.Origin, 0.f);
	CueParams.Instigator = Payload.Instigator;
	CueParams.AbilityLevel = Payload.AbilityLevel;
	// Hack Normale to pack timing info into (start delay, elapsed time before stating, ratio)
	CueParams.Normal = FVector(StartDelay, StartTime, 1 - ((StartDelay - StartTime) / StartDelay));
	return CueParams;
}

void UPattern::StartPattern()
{
	UAnimInstance* AnimInstance = GeoASLib::GetAnimInstance(StoredPayload);
	bool bIsServer = GeoLib::IsServer(GetWorld());
	if (IsValid(AnimMontage) && !bIsServer && IsValid(AnimInstance))
	{
		if (!AnimInstance->Montage_IsPlaying(AnimMontage))
		{
			UE_LOG(LogPattern, Warning, TEXT("Montage is NOT playing %s"), *AnimMontage->GetName());
			AnimInstance->Montage_Play(AnimMontage);
		}
		else
		{
			AnimInstance->Montage_SetPlayRate(AnimMontage);
		}

		FName const SectionName = AnimInstance->Montage_GetCurrentSection(AnimMontage);
		if (SectionName == GeoASLib::SectionStartName)
		{
			AnimInstance->Montage_JumpToSection(GeoASLib::SectionFireName);
		}
		else if (!SectionName.ToString().Contains(GeoASLib::SectionFireName.ToString()))
		{
			UE_LOG(LogPattern, Warning,
				   TEXT("AnimMontage in the section %s where it should be only %s or containing section name %s !"),
				   *SectionName.ToString(), *GeoASLib::SectionStartName.ToString(),
				   *GeoASLib::SectionFireName.ToString());
		}
	}

	if (StartGameplayCueTag.IsValid() && !bIsServer)
	{
		UGeoAbilitySystemComponent* InstigatorASC = GeoASLib::GetGeoAscFromActor(StoredPayload.Instigator);
		if (!IsValid(InstigatorASC))
		{
			ensureMsgf(false, TEXT("Pattern Instigator %s has no ASC !"), *StoredPayload.Instigator->GetName());
		}
		else
		{
			FScopedPredictionWindow ScopedPredictionWindow(InstigatorASC);
			FGameplayCueParameters CueParams = FillCueParam(StoredPayload);
			InstigatorASC->ExecuteGameplayCue(StartGameplayCueTag, CueParams);
		}
	}

	OnPatternStart.Broadcast();
}

void UPattern::JumpMontageToEndSection() const
{
	UAnimInstance* AnimInstance = GeoASLib::GetAnimInstance(StoredPayload);
	if (IsValid(AnimMontage) && !GeoLib::IsServer(GetWorld()) && IsValid(AnimInstance)
		&& AnimInstance->Montage_IsPlaying(AnimMontage)
		&& AnimInstance->Montage_GetCurrentSection(AnimMontage) != GeoASLib::SectionEndName)
	{
		AnimInstance->Montage_JumpToSection(GeoASLib::SectionEndName);
	}
}

float UPattern::CalculateElapsedTime() const
{
	return FMath::Max(0.f, GeoLib::GetServerTime(GetWorld(), true) - StoredPayload.ServerSpawnTime - StartDelay);
}

void UPattern::EndPattern()
{
	JumpMontageToEndSection();

	if (StartSectionTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(StartSectionTimerHandle);
	}

	bPatternIsActive = false;
	OnPatternEnd.Broadcast();
}

void UTickablePattern::InitPattern(FAbilityPayload const& Payload)
{
	if (TimeSyncTimerHandle.IsValid())
	{
		UE_LOG(LogPattern, Error,
			   TEXT("Starting pattern when the Previous pattern of the same instance is still active !"));
		EndPattern();
	}

	Super::InitPattern(Payload);
}

void UTickablePattern::StartPattern()
{
	Super::StartPattern();
	CalculateTimeAndTickPattern();
}

/**
 * Drives the tick loop using SetTimerForNextTick rather than a regular Tick override so that
 * the pattern can stop itself cleanly (by simply not re-scheduling) without a separate bIsActive guard.
 * SpentTime excludes the Start-section length because projectiles are not spawned during that phase.
 */
void UTickablePattern::CalculateTimeAndTickPattern()
{
	float const ServerTime = GeoLib::GetServerTime(GetWorld(), true);
	TickPattern(ServerTime, CalculateElapsedTime());
	if (IsPatternActive())
	{
		TimeSyncTimerHandle =
			GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UTickablePattern::CalculateTimeAndTickPattern);
	}
}

void UTickablePattern::TickPattern(float const ServerTime, float const SpentTime)
{
	// To be overriden by your own Tickable pattern !
}

void UTickablePattern::EndPattern()
{
	Super::EndPattern();
	GetWorld()->GetTimerManager().ClearTimer(TimeSyncTimerHandle);
}
