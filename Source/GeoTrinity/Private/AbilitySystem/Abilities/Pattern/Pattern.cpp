#include "AbilitySystem/Abilities/Pattern/Pattern.h"

#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Tool/UGameplayLibrary.h"

void UPattern::OnCreate(FGameplayTag AbilityTag)
{
	checkf(
		EffectDataArray.IsEmpty(),
		TEXT(
			"EffectDataArray is not empty when creating a Pattern ! Ensure you call this function only at the spawn of the pattern."));
	EffectDataArray = UGeoAbilitySystemLibrary::GetEffectDataArray(AbilityTag);
	if (AnimMontage)
	{
		StartSectionLength = AnimMontage->GetSectionLength(
			UGameplayLibrary::GetAndCheckSection(AnimMontage, UGameplayLibrary::SectionStartName));
		(void)UGameplayLibrary::GetAndCheckSection(AnimMontage, UGameplayLibrary::SectionFireName);
		(void)UGameplayLibrary::GetAndCheckSection(AnimMontage, UGameplayLibrary::SectionEndName);
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

	float const StartTime = UGameplayLibrary::GetServerTime(GetWorld(), true) - Payload.ServerSpawnTime;
	UAnimInstance* AnimInstance = UGameplayLibrary::GetAnimInstance(Payload);

	if (!IsValid(AnimMontage) || !IsValid(AnimInstance) || StartTime > StartSectionLength)
	{
		if (StartTime > StartSectionLength)
		{
			UE_LOG(LogPattern, Warning, TEXT("We start the montage too late, starting from loop directly"));
		}
		StartPattern(Payload);
		return;
	}

	if (!UGameplayLibrary::IsServer(GetWorld()))
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
	UAnimInstance* AnimInstance = UGameplayLibrary::GetAnimInstance(Payload);
	if (IsValid(AnimMontage) && !UGameplayLibrary::IsServer(GetWorld()) && IsValid(AnimInstance))
	{
		if (!AnimInstance->Montage_IsPlaying(AnimMontage))
		{
			UE_LOG(LogPattern, Warning, TEXT("Montage is NOT playing %s"), *AnimMontage->GetName());
			AnimInstance->Montage_Play(AnimMontage);
		}

		FName const SectionName = AnimInstance->Montage_GetCurrentSection(AnimMontage);
		if (SectionName == UGameplayLibrary::SectionStartName)
		{
			AnimInstance->Montage_JumpToSection(UGameplayLibrary::SectionFireName);
		}
		else if (!SectionName.ToString().Contains(UGameplayLibrary::SectionFireName.ToString()))
		{
			UE_LOG(LogPattern, Warning,
				   TEXT("AnimMontage in the section %s where it should be only %s or containing section name %s !"),
				   *SectionName.ToString(), *UGameplayLibrary::SectionStartName.ToString(),
				   *UGameplayLibrary::SectionFireName.ToString());
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
	UAnimInstance* AnimInstance = UGameplayLibrary::GetAnimInstance(StoredPayload);
	if (IsValid(AnimMontage) && !UGameplayLibrary::IsServer(GetWorld()) && IsValid(AnimInstance)
		&& AnimInstance->Montage_IsPlaying(AnimMontage)
		&& AnimInstance->Montage_GetCurrentSection(AnimMontage) != UGameplayLibrary::SectionEndName)
	{
		AnimInstance->Montage_JumpToSection(UGameplayLibrary::SectionEndName);
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
	float const ServerTime = UGameplayLibrary::GetServerTime(GetWorld(), true);
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
