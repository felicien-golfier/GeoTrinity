#include "AbilitySystem/Abilities/Pattern/Pattern.h"

#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Tool/GameplayLibrary.h"

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
			GameplayLibrary::GetAndCheckSection(AnimMontage, GameplayLibrary::SectionStartName));
		(void)GameplayLibrary::GetAndCheckSection(AnimMontage, GameplayLibrary::SectionFireName);
		(void)GameplayLibrary::GetAndCheckSection(AnimMontage, GameplayLibrary::SectionEndName);
	}
}

void UPattern::InitPattern(const FAbilityPayload& Payload)
{
	if (bPatternIsActive)
	{
		UE_LOG(LogPattern, Error,
			   TEXT("Starting pattern when the Previous pattern of the same instance is still active !"));
		EndPattern();
	}

	bPatternIsActive = true;
	StoredPayload = Payload;

	const float StartTime = GameplayLibrary::GetServerTime(GetWorld(), true) - Payload.ServerSpawnTime;
	UAnimInstance* AnimInstance = GameplayLibrary::GetAnimInstance(Payload);

	if (!IsValid(AnimMontage) || !IsValid(AnimInstance) || StartTime > StartSectionLength)
	{
		if (StartTime > StartSectionLength)
		{
			UE_LOG(LogPattern, Warning, TEXT("We start the montage too late, starting from loop directly"));
		}
		StartPattern(Payload);
		return;
	}

	if (!GameplayLibrary::IsServer(GetWorld()))
	{
		AnimInstance->Montage_Play(AnimMontage, 1.f, EMontagePlayReturnType::MontageLength, StartTime);
	}

	const float RemainingStartTime = StartSectionLength - StartTime;
	GetWorld()->GetTimerManager().SetTimer(StartSectionTimerHandle, this, &UPattern::OnMontageSectionStartEnded,
										   RemainingStartTime);
}

void UPattern::OnMontageSectionStartEnded()
{
	StartPattern(StoredPayload);
}

void UPattern::StartPattern(const FAbilityPayload& Payload)
{
	UAnimInstance* AnimInstance = GameplayLibrary::GetAnimInstance(Payload);
	if (IsValid(AnimMontage) && !GameplayLibrary::IsServer(GetWorld()) && IsValid(AnimInstance))
	{
		if (!AnimInstance->Montage_IsPlaying(AnimMontage))
		{
			UE_LOG(LogPattern, Warning, TEXT("Montage is NOT playing %s"), *AnimMontage->GetName());
			AnimInstance->Montage_Play(AnimMontage);
		}

		const FName SectionName = AnimInstance->Montage_GetCurrentSection(AnimMontage);
		if (SectionName == GameplayLibrary::SectionStartName)
		{
			AnimInstance->Montage_JumpToSection(GameplayLibrary::SectionFireName);
		}
		else if (SectionName != GameplayLibrary::SectionFireName)
		{
			UE_LOG(LogPattern, Warning, TEXT("AnimMontage in the section %s where it should be only Start or Loop !"),
				   *SectionName.ToString());
		}
	}

	OnStartPattern(Payload);
}

void UPattern::OnStartPattern_Implementation(const FAbilityPayload& Payload)
{
	// for Blueprint use mainly
}

void UPattern::JumpMontageToEndSection() const
{
	UAnimInstance* AnimInstance = GameplayLibrary::GetAnimInstance(StoredPayload);
	if (IsValid(AnimMontage) && !GameplayLibrary::IsServer(GetWorld()) && IsValid(AnimInstance)
		&& AnimInstance->Montage_IsPlaying(AnimMontage)
		&& AnimInstance->Montage_GetCurrentSection(AnimMontage) != GameplayLibrary::SectionEndName)
	{
		AnimInstance->Montage_JumpToSection(GameplayLibrary::SectionEndName);
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
	// TODO: EndAbility after Montage EndSection or directly right now.
}

void UTickablePattern::InitPattern(const FAbilityPayload& Payload)
{
	if (TimeSyncTimerHandle.IsValid())
	{
		UE_LOG(LogPattern, Error,
			   TEXT("Starting pattern when the Previous pattern of the same instance is still active !"));
		EndPattern();
	}

	Super::InitPattern(Payload);
}

void UTickablePattern::StartPattern(const FAbilityPayload& Payload)
{
	Super::StartPattern(Payload);
	CalculateTimeAndTickPattern();
}

void UTickablePattern::CalculateTimeAndTickPattern()
{
	const float ServerTime = GameplayLibrary::GetServerTime(GetWorld(), true);
	TickPattern(ServerTime, ServerTime - StoredPayload.ServerSpawnTime - StartSectionLength);
	if (IsPatternActive())
	{
		TimeSyncTimerHandle =
			GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UTickablePattern::CalculateTimeAndTickPattern);
	}
}

void UTickablePattern::TickPattern(const float ServerTime, const float SpentTime)
{
	// To be overriden by your own Tickable pattern !
}

void UTickablePattern::EndPattern()
{
	Super::EndPattern();
	GetWorld()->GetTimerManager().ClearTimer(TimeSyncTimerHandle);
}
