// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "GeoTarget.generated.h"

/** How an ability chooses its target locations (GeoASLib::PickTargets). A new way of choosing is a new value. */
UENUM(BlueprintType, meta = (ScriptName = "GeoTargetMode"))
enum class EGeoTarget : uint8
{
	/** Along the context's yaw, Distance away from its origin; 0 = the origin itself. */
	Forward,
	/** The caster's own actor, followed while it exists: where it stands, not the context's origin (the fire socket).
	 */
	Self,
	/** Each live player. */
	AllPlayers,
	/** Live players from a seeded start, Count plus one per ExtraBelowHealth ratio above the caster's health ratio. */
	RandomPlayers,
	/** The live boss of the fighting arena (AGeoArena::GetFightingArena); none outside a fight. */
	Boss,
	/** The first point tagged TargetPointTag in the context's room. */
	TargetPoint,
};

/** What an ability targets: a mode and the params of every mode, each shown only for its own. */
USTRUCT(BlueprintType)
struct GEOTRINITY_API FGeoTarget
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoTarget")
	EGeoTarget Mode = EGeoTarget::Forward;

	/** How far ahead of the origin, along the yaw. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoTarget",
			  meta = (EditCondition = "Mode == EGeoTarget::Forward", EditConditionHides, ClampMin = "0"))
	float Distance = 0.f;

	/** Targets picked at full health. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoTarget",
			  meta = (EditCondition = "Mode == EGeoTarget::RandomPlayers", EditConditionHides, ClampMin = "1"))
	int32 Count = 1;

	/** One more target per ratio above the caster's health ratio (0.5 = one more below half health). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoTarget",
			  meta = (EditCondition = "Mode == EGeoTarget::RandomPlayers", EditConditionHides))
	TArray<float> ExtraBelowHealth;

	/** Distance range a target beyond the alive player count is scattered by, around the player it shares. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoTarget",
			  meta = (EditCondition = "Mode == EGeoTarget::RandomPlayers", EditConditionHides, ClampMin = "0"))
	float MinScatterRadius = 300.f;

	/** Upper bound of the scatter distance. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoTarget",
			  meta = (EditCondition = "Mode == EGeoTarget::RandomPlayers", EditConditionHides, ClampMin = "0"))
	float MaxScatterRadius = 800.f;

	/** Which AGeoTargetPoint of the context's room (TargetPoint.*). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoTarget",
			  meta = (EditCondition = "Mode == EGeoTarget::TargetPoint", EditConditionHides,
					  Categories = "TargetPoint"))
	FGameplayTag TargetPointTag;
};

/** What targeting reads from the ability firing: built by the ability (UGeoGameplayAbility::MakeTargetContext). */
struct FGeoTargetContext
{
	/** The avatar casting: its world, its health (RandomPlayers). */
	AActor* Caster = nullptr;
	FVector2D Origin = FVector2D::ZeroVector;
	/** Facing, in degrees: Forward's direction, and the direction of a zero-length aim. */
	float Yaw = 0.f;
	int32 Seed = 0;
	/** The room whose points TargetPoint reads (Arena.*); empty outside a pattern ability. */
	FGameplayTag ArenaTag;
};

/** One chosen target: follows Actor (plus Offset) while it exists, else stays at Location. */
struct FGeoTargetPick
{
	TWeakObjectPtr<AActor> Actor;
	FVector2D Offset = FVector2D::ZeroVector;
	/** Where the pick stood when chosen, Offset included; read once Actor is gone. */
	FVector2D Location = FVector2D::ZeroVector;
};
