#include "AbilitySystem/Abilities/Pattern/Pattern.h"

#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Tool/UGeoGameplayLibrary.h"

void UPattern::OnCreate(FGameplayTag AbilityTag)
{
	checkf(
		EffectDataArray.IsEmpty(),
		TEXT(
			"EffectDataArray is not empty when creating a Pattern ! Ensure you call this function only at the spawn of the pattern."));
	EffectDataArray = UGeoAbilitySystemLibrary::GetEffectDataArray(AbilityTag);
	if (AnimMontage)
	{
		StartSectionLength =
			AnimMontage->GetSectionLength(GeoASLib::GetAndCheckSection(AnimMontage, GeoASLib::SectionStartName));
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

	float const StartTime = GeoLib::GetServerTime(GetWorld(), true) - Payload.ServerSpawnTime;
	UAnimInstance* AnimInstance = GeoASLib::GetAnimInstance(Payload);

	if (!IsValid(AnimMontage) || !IsValid(AnimInstance) || StartTime > StartSectionLength)
	{
		if (StartTime > StartSectionLength)
		{
			UE_LOG(LogPattern, Warning, TEXT("We start the montage too late, starting from loop directly"));
		}
		StartPattern(Payload);
		return;
	}

	if (!GeoLib::IsServer(GetWorld()))
	{
		AnimInstance->Montage_Play(AnimMontage, 1.f, EMontagePlayReturnType::MontageLength, StartTime);
	}

	float const RemainingStartTime = StartSectionLength - StartTime;
	GetWorld()->GetTimerManager().SetTimer(StartSectionTimerHandle, this, &UPattern::OnMontageSectionStartEnded,
										   RemainingStartTime);
}

void UPattern::OnMontageSectionStartEnded()
{
	StartPattern(StoredPayload);
}

void UPattern::StartPattern(FAbilityPayload const& Payload)
{
	UAnimInstance* AnimInstance = GeoASLib::GetAnimInstance(Payload);
	if (IsValid(AnimMontage) && !GeoLib::IsServer(GetWorld()) && IsValid(AnimInstance))
	{
		if (!AnimInstance->Montage_IsPlaying(AnimMontage))
		{
			UE_LOG(LogPattern, Warning, TEXT("Montage is NOT playing %s"), *AnimMontage->GetName());
			AnimInstance->Montage_Play(AnimMontage);
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

	OnStartPattern(Payload);
}

void UPattern::OnStartPattern_Implementation(FAbilityPayload const& Payload)
{
	// for Blueprint use mainly
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

void UTickablePattern::StartPattern(FAbilityPayload const& Payload)
{
	Super::StartPattern(Payload);
	CalculateTimeAndTickPattern();
}

void UTickablePattern::CalculateTimeAndTickPattern()
{
	float const ServerTime = GeoLib::GetServerTime(GetWorld(), true);
	TickPattern(ServerTime, ServerTime - StoredPayload.ServerSpawnTime - StartSectionLength);
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
