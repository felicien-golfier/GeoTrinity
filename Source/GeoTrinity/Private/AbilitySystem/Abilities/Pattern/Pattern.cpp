#include "AbilitySystem/Abilities/Pattern/Pattern.h"

#include "AbilitySystem/Data/EffectData.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Projectile/GeoProjectile.h"
#include "System/GeoActorPoolingSubsystem.h"

void UPattern::StartPattern_Implementation(const FPatternPayload& Payload)
{
	// To be overriden by your own pattern !
}

void UProjectilePattern::StartPattern_Implementation(const FPatternPayload& Payload)
{
	// Basic Projectile Pattern just spawns a projectile in the correct direction.
	SpawnProjectile(Payload, 0.f);
}

void UProjectilePattern::SpawnProjectile(const FPatternPayload& Payload, float Yaw)
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
	GeoProjectile->EffectDataArray = UGeoAbilitySystemLibrary::GetEffectDataArray(EffectDataAsset);

	GeoProjectile->Init();   // Equivalent to the DeferredSpawn
}
