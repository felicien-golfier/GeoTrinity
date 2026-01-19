#include "AbilitySystem/Abilities/Pattern/Pattern.h"

#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Projectile/GeoProjectile.h"
#include "GeoPlayerController.h"
#include "System/GeoActorPoolingSubsystem.h"
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
	// To be overriden by your own pattern !
}

void UTickablePattern::StartPattern_Implementation(const FAbilityPayload& Payload)
{
	if (TimeSyncTimerHandle.IsValid() || bPatternIsActive)
	{
		UE_LOG(LogTemp, Error,
			TEXT("Starting pattern when the Previous pattern of the same instance is still active !"));
		EndPattern();
	}

	StoredPayload = Payload;
	PreviousFrameTime = GetWorld()->GetTimeSeconds();
	// Call this only next frame to have a proper Delta Time.
	TimeSyncTimerHandle =
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UTickablePattern::CalculateDeltaTimeAndTickPattern);

	bPatternIsActive = true;
}

void UTickablePattern::CalculateDeltaTimeAndTickPattern()
{
	const double CurrentTime = GetWorld()->GetTimeSeconds();
	TickPattern(CurrentTime - PreviousFrameTime);
	PreviousFrameTime = CurrentTime;
	if (bPatternIsActive)
	{
		TimeSyncTimerHandle = GetWorld()->GetTimerManager().SetTimerForNextTick(this,
			&UTickablePattern::CalculateDeltaTimeAndTickPattern);
	}
}

void UTickablePattern::TickPattern(float DeltaSeconds)
{
	// To be overriden by your own Tickable pattern !
}
void UTickablePattern::EndPattern()
{
	bPatternIsActive = false;
	PreviousFrameTime = 0;
	GetWorld()->GetTimerManager().ClearTimer(TimeSyncTimerHandle);
}

void UProjectilePattern::StartPattern_Implementation(const FAbilityPayload& Payload)
{
	// Basic Projectile Pattern just spawns a projectile in the correct direction.
	SpawnProjectile(Payload, 0.f);
}

void UProjectilePattern::SpawnProjectile(const FAbilityPayload& Payload, float Yaw)
{

	const FTransform SpawnTransform{FRotator(0.f, Yaw, 0.f), FVector(Payload.Origin, 50.f)};

	// Create projectile
	checkf(ProjectileClass, TEXT("No ProjectileClass in the projectile spell!"));

	AGeoProjectile* GeoProjectile =
		UGeoActorPoolingSubsystem::Get(GetWorld())
			->RequestActor(ProjectileClass, SpawnTransform, Payload.Owner, Cast<APawn>(Payload.Instigator), false);

	if (!GeoProjectile)
	{
		UE_LOG(LogTemp, Error, TEXT("No valid Projectile pooled ( ;-) ) !"));
		return;
	}

	GeoProjectile->Payload = Payload;
	GeoProjectile->EffectDataArray = EffectDataArray;

	GeoProjectile->Init();   // Equivalent to the DeferredSpawn
}
