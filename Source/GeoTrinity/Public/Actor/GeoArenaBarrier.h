// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "GeoArenaBarrier.generated.h"

class UBoxComponent;

/**
 * Replicated barrier that blocks the arena entrance. Collision and visibility sync to all clients
 * via OnRep_bIsClosed. Visual/VFX are handled by BlueprintImplementableEvent.
 */
UCLASS()
class GEOTRINITY_API AGeoArenaBarrier : public AActor
{
	GENERATED_BODY()

public:
	AGeoArenaBarrier();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Server-only. Opens or closes the barrier and replicates state to all clients. */
	void SetClosed(bool bNewClosed);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UBoxComponent> BlockingVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMeshComponent> MeshComponent;

protected:
	/** BP implementable event so designers can add VFX / dissolve on state change. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Barrier")
	void OnBarrierStateChanged(bool bClosed);

private:
	UPROPERTY(ReplicatedUsing = OnRep_bIsClosed, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	bool bIsClosed = false;

	UFUNCTION()
	void OnRep_bIsClosed();
};
