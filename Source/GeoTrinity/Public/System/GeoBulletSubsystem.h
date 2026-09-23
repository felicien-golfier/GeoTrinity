// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Base/AbilityPayload.h"
#include "AbilitySystem/Data/EffectData.h"
#include "Actor/Projectile/ExternalProjectileParams.h"
#include "CoreMinimal.h"
#include "StructUtils/InstancedStruct.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tool/GeoHazardJudge.h"

#include "GeoBulletSubsystem.generated.h"

class AGeoProjectile;

/**
 * One bullet of a pattern: a straight flight at a fixed speed, from where and when its payload says it left, hitting
 * the first target it touches and stopping on the first wall. Where it is at a given moment is maths from its start alone, so every machine has it in
 * the same place at the same server time and only the server judges what it hits.
 */
USTRUCT()
struct FGeoBullet
{
	GENERATED_BODY()

	/** The shot it belongs to. Its Origin, Yaw and ServerSpawnTime are this bullet's own, not the pattern's. */
	UPROPERTY()
	FAbilityPayload Payload;

	/** Applied to the target it hits, by the judge on the server. */
	UPROPERTY()
	TArray<TInstancedStruct<FEffectData>> Effects;

	/** What it flies and looks like; its ProjectileClass is the actor drawing it. */
	UPROPERTY()
	FExternalProjectileParams Params;

	/** The actor drawing it on this machine: none on a dedicated server, and none once it has ended on this screen. */
	UPROPERTY()
	TObjectPtr<AGeoProjectile> Visual;

	FVector2D Direction = FVector2D::ZeroVector;
	float Speed = 0.f;
	float Radius = 0.f;
	/** How long it flies: its distance span at its speed, cut short where it meets a wall. */
	float FlightDuration = 0.f;
	/** How far into its flight its path has been checked for walls. */
	float SweptTime = 0.f;
	/** Which attitudes, relative to the payload owner's team, it hits. */
	int32 TeamAttitude = 0;

	/** Server only. Ends on the first target it hits, and is over once everyone has been judged. */
	FGeoHazardJudge Judge;

	/** Where the bullet is SpentTime after it left. */
	FVector2D GetLocation(float SpentTime) const { return Payload.Origin + Direction * Speed * SpentTime; }
};

/**
 * Every bullet a pattern has fired and that is still flying. Bullets are data, not actors: the server judges each one
 * against every hostile at that hostile's own time (FGeoHazardJudge) and the machines that render just draw it, with a
 * projectile actor that carries no effects of its own. They outlive the pattern that fired them, so a boss firing its
 * next salve never takes back the one before.
 */
UCLASS()
class GEOTRINITY_API UGeoBulletSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Returns the subsystem for the given world. */
	static UGeoBulletSubsystem* Get(UWorld const* World);

	/**
	 * Fires one bullet, leaving Payload.Origin along Payload.Yaw at Payload.ServerSpawnTime — a spawn time already
	 * past only means the bullet is that far on its way. Called on every machine, each drawing it and the server alone
	 * judging it.
	 *
	 * @param Params        The bullet's speed, radius, distance span and look. Those three values cannot be left on
	 *                      KeepBlueprintDefaultValue: a bullet is flown with no projectile Blueprint to read them from.
	 * @param Effects       Applied to the target it hits.
	 * @param TeamAttitude  Which attitudes, relative to the payload owner's team, it hits.
	 * @param bSeenThroughReplication  True for a shot the server alone decides (a turret, a mine): each machine fires
	 *                                 it when it hears of it, at its own current time, so it leaves the origin on every
	 *                                 screen and the server judges each player as seen that much late. False when every
	 *                                 machine fires it on the same server clock (a pattern).
	 */
	void FireBullet(FAbilityPayload const& Payload, FExternalProjectileParams const& Params,
					TArray<TInstancedStruct<FEffectData>> const& Effects, int32 TeamAttitude,
					bool bSeenThroughReplication);

	/** Stops every bullet on the walls it flew into, judges them on the server, draws them all where they now are, ends
	 * a drawing once its bullet has flown its range or is over, and drops the bullets that are over. */
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

private:
	/**
	 * Resolves one of a bullet's flight values from how its params say to read it.
	 *
	 * @param ParamName  Named in the config-bug message, since the Blueprint default a bullet cannot read is this
	 *                   value's.
	 */
	static float ResolveFlightParam(EOverrideParam Mode, float OverrideValue, float SettingsValue,
									TCHAR const* ParamName);

	/** Spawns the actor drawing Bullet, with no effects on it: what the bullet hits is the judge's word, not its
	 * actor's. It still ends itself on whatever it touches on this screen, which is how a bullet disappears on the
	 * player it just hit. */
	void SpawnVisual(FGeoBullet& Bullet, float SpentTime);

	/**
	 * Sweeps the path Bullet flew since the last check, up to ServerTime, and ends its flight on the first wall there,
	 * its drawing ending on that wall through the projectile's own hit path, and its judge with it.
	 * Runs before the judge: every time it judges is at or before now, so a flight cut short never takes back a hit.
	 */
	void StopAtWall(FGeoBullet& Bullet, float ServerTime);

	/** Lets go of an actor that ended — on whatever it touched, or with its bullet — so a pooled actor handed to the next
	 * shot is never moved by the bullet that had it. */
	UFUNCTION()
	void OnVisualEnded(AGeoProjectile* Projectile);

	UPROPERTY()
	TArray<FGeoBullet> Bullets;

	/** Bullets fired while Tick is judging Bullets — an effect a hazard applies could in principle fire another shot
	 * before Judge returns, and appending straight to Bullets could reallocate it out from under Judge's own
	 * references to the entry it is running on. Folded into Bullets at the start of the next Tick, never mid-Tick. */
	UPROPERTY()
	TArray<FGeoBullet> PendingBullets;
};
