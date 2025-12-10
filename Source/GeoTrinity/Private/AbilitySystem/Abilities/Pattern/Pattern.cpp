#include "AbilitySystem/Abilities/Pattern/Pattern.h"

#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "Actor/Projectile/GeoProjectile.h"
#include "System/GeoActorPoolingSubsystem.h"

void UPattern::StartPattern(const FPatternPayload& Payload)
{
	OnStartPattern(Payload);
	// To be overriden by your own partern !
}

void UPattern::SpawnProjectile(const FPatternPayload& Payload, float Yaw)
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

	// TODO: minimum damage Effects for now. find out a solution. Maybe hook the ability ?
	FDamageEffectParams params;
	params.WorldContextObject = Payload.Owner;
	params.DamageGameplayEffectClass = DamageEffectClass;
	params.SourceASC = Payload.Owner->GetComponentByClass<UGeoAbilitySystemComponent>();
	params.BaseDamage = 5.f;
	GeoProjectile->DamageEffectParams = params;

	GeoProjectile->Init();   // Equivalent to the DeferredSpawn
}
