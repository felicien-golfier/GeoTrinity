// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "World/ZoneLayer/GeoZoneShape.h"

#include "GeoZoneLayerSubsystem.generated.h"

class AGeoZoneLayerActor;
class UGeoZoneVisualComponent;
class UTexture2D;

/**
 * The zone layer of one world (World/ZoneLayer/CLAUDE.md): every shown UGeoZoneVisualComponent, packed each tick into
 * the slot texture both planes of its AGeoZoneLayerActor read. Game, PIE and editor worlds (a hand-placed zone shows
 * in the viewport); never on a dedicated server, which draws nothing.
 */
UCLASS()
class GEOTRINITY_API UGeoZoneLayerSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	/** False on a dedicated server. */
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	/** Spawns the layer actor on a rendering machine once, then packs and uploads the slots. */
	virtual void Tick(float DeltaTime) override;
	/** Editor worlds draw too. */
	virtual bool IsTickableInEditor() const override { return true; }
	virtual TStatId GetStatId() const override
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(UGeoZoneLayerSubsystem, STATGROUP_Tickables);
	}

	/** Draws Component from the next tick on, after every shape registered before it. No-op when already drawn. */
	void Register(UGeoZoneVisualComponent* Component);
	/** Stops drawing Component; the shapes after it move down a slot. */
	void Unregister(UGeoZoneVisualComponent* Component);

	/** Packs every registered shape whose owner is not hidden in game (a blinking or expired deployable) into
	 * SlotTexels, in registration order, and counts them in ZoneCount. Over MaxZoneSlots: ensures once, drops the rest.
	 */
	void PackSlots();

	/** How many slots the last PackSlots filled. */
	int32 GetZoneCount() const { return ZoneCount; }
	/** Slot Slot's texels as the last PackSlots wrote them. */
	GeoZoneLayer::FSlotTexels GetSlotTexels(int32 Slot) const;

private:
	/** Creates SlotTexture (once per subsystem: a failed spawn retried next tick reuses it) and spawns the layer actor
	 * reading it. */
	void SpawnLayerActor();
	/** Sends SlotTexels to SlotTexture on the render thread. */
	void UploadSlotTexture() const;

	TArray<TWeakObjectPtr<UGeoZoneVisualComponent>> Shapes;

	/** MaxZoneSlots × SlotRows, row-major: row R of slot S at R * MaxZoneSlots + S. */
	TArray<FLinearColor> SlotTexels;

	int32 ZoneCount = 0;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> SlotTexture;

	UPROPERTY(Transient)
	TObjectPtr<AGeoZoneLayerActor> LayerActor;
};
