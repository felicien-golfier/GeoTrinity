#include "AbilitySystem/Abilities/Pattern/Pattern.h"

#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "GameFramework/Character.h"
#include "Tool/GameplayLibrary.h"

void UPattern::OnCreate(FGameplayTag AbilityTag)
{
	checkf(EffectDataArray.IsEmpty(),
		TEXT(
			"EffectDataArray is not empty when creating a Pattern ! Ensure you call this function only at the spawn of the pattern."));
	EffectDataArray = UGeoAbilitySystemLibrary::GetEffectDataArray(AbilityTag);
}

void UPattern::StartPattern_Implementation(const FAbilityPayload& Payload)
{
	if (IsValid(AnimMontage) && !GameplayLibrary::IsServer(GetWorld()))
	{
		ACharacter* OwnerCharacter = Cast<ACharacter>(Payload.Owner);
		if (IsValid(OwnerCharacter))
		{
			UAnimInstance* AnimInstance =
				(OwnerCharacter->GetMesh()) ? OwnerCharacter->GetMesh()->GetAnimInstance() : nullptr;
			if (AnimMontage && AnimInstance)
			{
				float StartTime = GameplayLibrary::GetServerTime(GetWorld()) - Payload.ServerSpawnTime;
				AnimInstance->Montage_Play(AnimMontage, 1.f, EMontagePlayReturnType::MontageLength, StartTime);
			}
		}
	}
}

void UTickablePattern::StartPattern_Implementation(const FAbilityPayload& Payload)
{
	Super::StartPattern_Implementation(Payload);

	if (TimeSyncTimerHandle.IsValid() || bPatternIsActive)
	{
		UE_LOG(LogTemp, Error,
			TEXT("Starting pattern when the Previous pattern of the same instance is still active !"));
		EndPattern();
	}

	StoredPayload = Payload;
	bPatternIsActive = true;
	CalculateDeltaTimeAndTickPattern();
}

void UTickablePattern::CalculateDeltaTimeAndTickPattern()
{
	const float ServerTime = GameplayLibrary::GetServerTime(GetWorld(), true);
	TickPattern(ServerTime, ServerTime - StoredPayload.ServerSpawnTime);
	if (bPatternIsActive)
	{
		TimeSyncTimerHandle = GetWorld()->GetTimerManager().SetTimerForNextTick(this,
			&UTickablePattern::CalculateDeltaTimeAndTickPattern);
	}
}

void UTickablePattern::TickPattern(const float ServerTime, const float SpentTime)
{
	// To be overriden by your own Tickable pattern !
}

void UTickablePattern::EndPattern()
{
	bPatternIsActive = false;
	GetWorld()->GetTimerManager().ClearTimer(TimeSyncTimerHandle);
}
