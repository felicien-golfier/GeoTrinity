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
	if (!ensureMsgf(AbilityCDO, TEXT("Pattern %s has no AbilityCDO !"), *AbilityTag.ToString()))
	{
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

void UPattern::InitPattern(FAbilityPayload const& Payload, TInstancedStruct<FPatternData> const& PatternData)
{
	if (bPatternIsActive)
	{
		UE_LOG(LogPattern, Error,
			   TEXT("Starting pattern when the Previous pattern of the same instance is still active !"));
		EndPattern(true);
	}

	bPatternIsActive = true;
	StoredPayload = Payload;
	StoredPatternData = PatternData;
	TravelTime = GeoLib::GetServerTime(GetWorld(), true) - Payload.ServerSpawnTime;
	// Cosmetic montage gate: any machine that renders this boss must play it, including the listen-server host.
	// Only a dedicated server (no viewport) skips it. !IsServer() would wrongly skip the host.
	bool const bRendersLocally = !GeoLib::IsDedicatedServer(GetWorld());

	UAnimInstance* AnimInstance = GeoASLib::GetAnimInstance(Payload);

	ExecuteGameplayCue(InitCue);

	if (TravelTime >= StartDelay)
	{
		// A pattern with no wind-up at all is not late, it is meant to fire on arrival.
		if (StartDelay > 0.f)
		{
			UE_LOG(LogPattern, Warning, TEXT("We start the montage too late, starting from loop directly"));
		}
		StartPattern();
	}
	else
	{
		if (bRendersLocally && IsValid(AnimMontage) && IsValid(AnimInstance))
		{
			int const StartSection = GeoASLib::GetAndCheckSection(AnimMontage, GeoASLib::SectionStartName);
			float const PlayRate = AnimMontage->GetSectionLength(StartSection) / StartDelay;
			AnimInstance->Montage_Play(AnimMontage, PlayRate, EMontagePlayReturnType::MontageLength,
									   FMath::Max(0.f, TravelTime));
		}

		float const RemainingStartTime = StartDelay - TravelTime;
		GetWorld()->GetTimerManager().SetTimer(StartSectionTimerHandle, this, &UPattern::StartPattern,
											   RemainingStartTime);
	}
}

void UPattern::ExecuteGameplayCue(FGeoCueParam const& Cue)
{
	// Local (non-replicated) cue: execute on every rendering machine including the listen-server host; skip only on a
	// dedicated server, which has no viewport.
	if (!Cue.CueTag.IsValid() || GeoLib::IsDedicatedServer(GetWorld()))
	{
		return;
	}

	UGeoAbilitySystemComponent* InstigatorASC = GeoASLib::GetGeoAscFromActor(StoredPayload.Instigator);
	if (!ensureMsgf(IsValid(InstigatorASC), TEXT("Pattern %s: instigator has no ASC"), *GetName()))
	{
		return;
	}

	FScopedPredictionWindow ScopedPredictionWindow(InstigatorASC);
	FGameplayCueParameters CueParams = FillCueParam(StoredPayload);
	Cue.FillCueParams(CueParams);
	InstigatorASC->ExecuteGameplayCue(Cue.CueTag, CueParams);
}

FGameplayCueParameters UPattern::FillCueParam(FAbilityPayload const& Payload)
{
	FGameplayCueParameters CueParams;
	CueParams.Location = FVector(Payload.Origin, 0.f);
	CueParams.Instigator = Payload.Instigator;
	CueParams.AbilityLevel = Payload.AbilityLevel;
	float WindUpRatio = 1.f;
	if (StartDelay > 0.f)
	{
		WindUpRatio = TravelTime / StartDelay;
	}
	else
	{
		UE_LOG(LogPattern, Verbose, TEXT("%s has no wind-up, packing a cue timing ratio of 1."), *GetName());
	}
	// Hack Normale to pack timing info into (start delay, elapsed time before stating, ratio)
	CueParams.Normal = FVector(StartDelay, TravelTime, WindUpRatio);
	return CueParams;
}

void UPattern::StartPattern()
{
	UAnimInstance* AnimInstance = GeoASLib::GetAnimInstance(StoredPayload);
	if (IsValid(AnimMontage) && !GeoLib::IsDedicatedServer(GetWorld()) && IsValid(AnimInstance))
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

	ExecuteGameplayCue(StartCue);

	OnPatternStart.Broadcast();
}

void UPattern::JumpMontageToEndSection() const
{
	UAnimInstance* AnimInstance = GeoASLib::GetAnimInstance(StoredPayload);
	if (IsValid(AnimMontage) && !GeoLib::IsDedicatedServer(GetWorld()) && IsValid(AnimInstance)
		&& AnimInstance->Montage_IsPlaying(AnimMontage)
		&& AnimInstance->Montage_GetCurrentSection(AnimMontage) != GeoASLib::SectionEndName)
	{
		AnimInstance->Montage_JumpToSection(GeoASLib::SectionEndName);
	}
}

void UPattern::EndPattern(bool const bForceStop)
{
	if (!bPatternIsActive)
	{
		return;
	}
	bPatternIsActive = false;

	if (bForceStop)
	{
		UAnimInstance* AnimInstance = GeoASLib::GetAnimInstance(StoredPayload);
		if (IsValid(AnimInstance))
		{
			AnimInstance->StopAllMontages(.2f);
		}
	}
	else
	{
		JumpMontageToEndSection();
	}

	if (StartSectionTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(StartSectionTimerHandle);
	}

	if (!bForceStop)
	{
		OnPatternEnd.Broadcast();
	}
}

void UTickablePattern::InitPattern(FAbilityPayload const& Payload, TInstancedStruct<FPatternData> const& PatternData)
{
	if (TimeSyncTimerHandle.IsValid())
	{
		UE_LOG(LogPattern, Error,
			   TEXT("Starting pattern when the Previous pattern of the same instance is still active !"));
		EndPattern();
	}

	Super::InitPattern(Payload, PatternData);

	if (IsPatternActive())
	{
		CalculateTimeAndTickPattern();
	}
}

/**
 * Drives the tick loop using SetTimerForNextTick rather than a regular Tick override so that
 * the pattern can stop itself cleanly (by simply not re-scheduling) without a separate bIsActive guard.
 * SpentTime excludes the Start-section length because projectiles are not spawned during that phase; the loop
 * already runs during that section, where the still-pending Start-section timer routes it to TickDuringInit.
 */
void UTickablePattern::CalculateTimeAndTickPattern()
{
	float const ServerTime = GeoLib::GetServerTime(GetWorld(), true);
	float const SpentTime = ServerTime - StoredPayload.ServerSpawnTime - StartDelay;
	if (GetWorld()->GetTimerManager().IsTimerActive(StartSectionTimerHandle))
	{
		TickDuringInit(SpentTime);
	}
	else
	{
		TickPattern(ServerTime, FMath::Max(0.f, SpentTime));
	}

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

void UTickablePattern::TickDuringInit(float /*SpentTime is NEGATIVE value until 0 when StartPattern*/)
{
	// To be overriden when your pattern must keep moving during the wind-up !
}

void UTickablePattern::EndPattern(bool bForceStop)
{
	Super::EndPattern(bForceStop);
	GetWorld()->GetTimerManager().ClearTimer(TimeSyncTimerHandle);
}
