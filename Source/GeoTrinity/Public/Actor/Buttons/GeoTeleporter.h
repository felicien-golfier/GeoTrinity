// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"

#include "GeoTeleporter.generated.h"

class APlayableCharacter;
class UNiagaraComponent;
class UNiagaraSystem;
class UTextRenderComponent;

/**
 * Overlap pad that teleports the touching playable character — and every other player sharing its machine, since couch
 * coop players share one camera — to the next AGeoTeleporter sharing its TeleportTag (sorted by actor name, wrapping).
 * Purely a position move — the camera reframes on its own because the arriving player overlaps the destination room's
 * AGeoCameraVolume.
 * Inert while the match is in progress: no leaving (or joining) a live fight through a pad.
 * Closed (bOpen false): hidden and without overlap, still a valid destination. Never destroyed.
 */
UCLASS()
class GEOTRINITY_API AGeoTeleporter : public AActor
{
	GENERATED_BODY()

public:
	/** Creates the MeshComponent, TextComponent, and overlap collision used to detect arriving characters. */
	AGeoTeleporter();

	/** Opens or closes the pad on every machine. Server only. */
	void SetOpen(bool bNewOpen);

	bool IsOpen() const { return bOpen; }

	/** Recolours the portal; the material derives its hot colour from it. */
	UFUNCTION(BlueprintCallable, Category = "GeoTeleport")
	void SetPortalColor(FLinearColor NewColor);

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(FTransform const& Transform) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Runs on every machine at BeginPlay and on each change of bOpen, after the pad's visibility and overlap follow
	 * it: the open / close animation of the pad Blueprint plays here. */
	UFUNCTION(BlueprintImplementableEvent, Category = "GeoTeleport")
	void OnOpenChanged(bool bNowOpen);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GeoCollision")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GeoCollision")
	TObjectPtr<UTextRenderComponent> TextComponent;

	/** The portal (NS_TeleportPortal): INTRO on Activate, MAIN held, OUTRO on Deactivate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GeoTeleport")
	TObjectPtr<UNiagaraComponent> PortalComponent;

	/** The one colour of the portal, written to the system's User.Color; the system derives its hot colour. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoTeleport")
	FLinearColor PortalColor = FLinearColor(0.0685f, 0.3915f, 1.0f);

	/** Links teleporters (you teleport to the next pad sharing this tag). Editor-created tags work — no native tag
	 * needed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoTeleport")
	FGameplayTag TeleportTag;

	/** Label shown above the pad (e.g. to tell teleporters in the same tag group apart in-editor/in-game). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoTeleport")
	FText DisplayText;

	/** Seconds a traveller's move input stays ignored once it lands on this pad, so the camera reframes before they can
	 * walk off — and back onto a pad. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoTeleport", meta = (ClampMin = "0.01"))
	float ArrivalMoveLockDuration = 0.5f;

	/** State at level start; changed at runtime through SetOpen. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Open, Category = "GeoTeleport")
	bool bOpen = true;

private:
	UFUNCTION()
	void OnRep_Open();

	/** Overlap and label follow bOpen at once; a closed pad forgets its pending arrivals. */
	void ApplyOpen();

	/** Activates the portal (intro) when open, deactivates it gracefully (outro) when closed. */
	void PlayPortalTransition();

	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
						int32 OtherBodyIndex, bool bFromSweep, FHitResult const& SweepResult);

	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
					  int32 OtherBodyIndex);

	AGeoTeleporter* FindNextTeleporter() const;

	/** Stops Traveller and ignores its move input for ArrivalMoveLockDuration. Runs on its arrival overlap, after the
	 * teleport, on every machine holding its controller. */
	void LockArrivalMovement(APlayableCharacter& Traveller) const;

	/** Characters another teleporter just sent here — kept until they walk off the pad (end overlap) so no arrival
	 * overlap, however many times it fires, can chain-teleport them back. */
	TSet<TObjectPtr<AActor>> PendingArrivals;
};
