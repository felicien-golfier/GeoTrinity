#pragma once

#include "Pattern.generated.h"

class UGameplayEffect;
class AGeoProjectile;
USTRUCT(BlueprintType)
struct FPatternPayload
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadOnly)
	FVector2D Origin;   // position X,Y

	UPROPERTY(Transient, BlueprintReadOnly)
	float Yaw;   // orientation

	UPROPERTY(Transient, BlueprintReadOnly)
	double ServerSpawnTime;   // server world time (seconds)

	UPROPERTY(Transient, BlueprintReadOnly)
	int32 Seed;   // seed pour variations RNG

	UPROPERTY(Transient, BlueprintReadOnly)
	TSubclassOf<class UPattern> PatternClass;

	UPROPERTY(Transient, BlueprintReadOnly)
	AActor* Owner;

	UPROPERTY(Transient, BlueprintReadOnly)
	AActor* Instigator;
};

UCLASS(BlueprintType, Blueprintable)
class GEOTRINITY_API UPattern : public UObject
{
	GENERATED_BODY()

public:
	virtual void StartPattern(const FPatternPayload& Payload);

	UFUNCTION(BlueprintImplementableEvent)
	void OnStartPattern(const FPatternPayload& Payload);

	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void SpawnProjectile(const FPatternPayload& Payload, float Yaw);

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<AGeoProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClass;
};
