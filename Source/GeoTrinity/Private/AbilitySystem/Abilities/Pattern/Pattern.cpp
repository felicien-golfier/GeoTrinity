#include "AbilitySystem/Abilities/Pattern/Pattern.h"

#include "AbilitySystem/Data/EffectData.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Projectile/GeoProjectile.h"
#include "System/GeoActorPoolingSubsystem.h"

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
