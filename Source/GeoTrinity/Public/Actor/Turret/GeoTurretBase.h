// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Data/EffectData.h"
#include "Actor/Deployable/GeoDeployableBase.h"
#include "Actor/GeoInteractableActor.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "GeoTurretBase.generated.h"

class AGeoProjectile;
class UCapsuleComponent;

struct FTurretData : FInteractableActorData
{
	TArray<TInstancedStruct<struct FEffectData>> EffectDataArray;
};

UCLASS()
class GEOTRINITY_API AGeoTurretBase : public AGeoDeployableBase
{
	GENERATED_BODY()

public:
	AGeoTurretBase();

	virtual void InitInteractableData(FInteractableActorData* Data) override;
	virtual void OnRecalled() override;

	virtual FGenericTeamId GetGenericTeamId() const override { return TeamID; }

	bool IsBlinking() const;
	bool WasBlinkingOnRecall() const { return bWasBlinkingOnRecall; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	AActor* FindBestTarget() const;
	void ScheduleFire();
	void TryFire();

	TArray<TInstancedStruct<FEffectData>> EffectDataArray;
	AActor* CharacterOwner = nullptr;
	float TurretLevel = 1.f;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AGeoProjectile> TurretProjectileClass;

	UPROPERTY(EditDefaultsOnly)
	float FireInterval = 1.f;

	UPROPERTY(EditDefaultsOnly)
	float BlinkThreshold = 0.2f;

private:
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UCapsuleComponent> CapsuleComponent;

	FTimerHandle FireTimerHandle;
	FGenericTeamId TeamID;
	bool bWasBlinkingOnRecall = false;
};
