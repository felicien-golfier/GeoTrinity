// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "System/GeoBulletSubsystem.h"

#include "AbilitySystem/Data/GeoAbilityTargetTypes.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Projectile/GeoProjectile.h"
#include "Engine/World.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Settings/GameDataSettings.h"
#include "Tool/GeoNetcodeDebug.h"
#include "Tool/Team.h"
#include "Tool/UGeoGameplayLibrary.h"

UGeoBulletSubsystem* UGeoBulletSubsystem::Get(UWorld const* World)
{
	UGeoBulletSubsystem* BulletSubsystem = World->GetSubsystem<UGeoBulletSubsystem>();
	ensureMsgf(BulletSubsystem, TEXT("GeoBulletSubsystem is invalid!"));
	return BulletSubsystem;
}

void UGeoBulletSubsystem::FireBullet(FAbilityPayload const& Payload, FExternalProjectileParams const& Params,
									 TArray<TInstancedStruct<FEffectData>> const& Effects, int32 const TeamAttitude,
									 bool const bSeenThroughReplication)
{
	UGameDataSettings const* const Settings = GetDefault<UGameDataSettings>();
	bool const bPlayerInstigator =
		GeoASLib::GetTeamId(Payload.SourceAvatar).GetId() == static_cast<uint8>(ETeam::Player);

	float const Speed =
		ResolveFlightParam(Params.OverrideSpeed, Params.ProjectileSpeed, Settings->GeneralSpellSpeed, TEXT("speed"));
	float const DistanceSpan = ResolveFlightParam(
		Params.OverrideDistanceSpan, Params.DistanceSpan,
		bPlayerInstigator ? Settings->GeneralSpellDistance : Settings->EnemySpellDistance, TEXT("distance span"));

	if (ensureMsgf(Speed > 0.f, TEXT("%hs: %s fired a bullet that does not move"), __FUNCTION__,
				   *GetNameSafe(Payload.SourceAvatar)))
	{
		FGeoBullet& Bullet = PendingBullets.AddDefaulted_GetRef();
		Bullet.Payload = Payload;
		Bullet.Effects = Effects;
		Bullet.Params = Params;
		Bullet.Direction = FVector2D(FRotator(0.f, Payload.Yaw, 0.f).Vector());
		Bullet.Speed = Speed;
		Bullet.Radius =
			ResolveFlightParam(Params.OverrideRadius, Params.Radius, Settings->GeneralProjectileRadius, TEXT("radius"));
		Bullet.FlightDuration = DistanceSpan / Speed;
		Bullet.TeamAttitude = TeamAttitude;

		float const SpentTime = GeoLib::GetServerTime(GetWorld(), true) - Payload.ServerSpawnTime;
		if (GeoLib::IsServer(GetWorld()))
		{
			Bullet.Judge.Start(Payload.ServerSpawnTime, Bullet.FlightDuration, bSeenThroughReplication,
							   /*bEndsOnHit*/ true, /*bRemovesInfiniteEffectsOnLeave*/ false);
		}
		if (!GeoLib::IsDedicatedServer(GetWorld()) && SpentTime < Bullet.FlightDuration)
		{
			SpawnVisual(Bullet, SpentTime);
		}
	}
}

void UGeoBulletSubsystem::Tick(float const DeltaTime)
{
	Super::Tick(DeltaTime);

	UWorld const* const World = GetWorld();
	if (Bullets.Num() == 0 && PendingBullets.Num() == 0 || !World || !World->GetGameState())
	{
		return;
	}

	Bullets.Append(MoveTemp(PendingBullets));
	PendingBullets.Reset();

	float const ServerTime = GeoLib::GetServerTime(World, true);
	bool const bIsServer = GeoLib::IsServer(World);

	AActor const* CandidatesOwner = nullptr;
	int32 CandidatesAttitude = 0;
	TArray<AActor*> Candidates;

	// Backwards, so dropping a bullet leaves the ones still to judge where they are.
	for (int32 BulletIndex = Bullets.Num() - 1; BulletIndex >= 0; --BulletIndex)
	{
		StopAtWall(Bullets[BulletIndex], ServerTime);

		if (bIsServer)
		{
			// One walk of the world per owner: every bullet of a boss's salve is judged against the same candidates.
			if (Bullets[BulletIndex].Payload.SourceOwner != CandidatesOwner
				|| Bullets[BulletIndex].TeamAttitude != CandidatesAttitude)
			{
				CandidatesOwner = Bullets[BulletIndex].Payload.SourceOwner;
				CandidatesAttitude = Bullets[BulletIndex].TeamAttitude;
				Candidates = FGeoHazardJudge::FindCandidates(CandidatesOwner, CandidatesAttitude);
			}

			Bullets[BulletIndex].Judge.Judge(
				Bullets[BulletIndex].Payload, Bullets[BulletIndex].Effects, Candidates,
				[this, BulletIndex, ServerTime](AActor const* Target, FVector2D const Location, float const SpentTime)
				{
					FGeoBullet const& Bullet = Bullets[BulletIndex];
					bool const bInside = GeoASLib::IsInCircle(Target, Location, Bullet.GetLocation(SpentTime),
															  Bullet.Radius, ETargetOverlapMode::IncludeRadius,
															  GeoASLib::GetTeamId(Bullet.Payload.SourceOwner));
					if (bInside)
					{
						float const CurrentSpentTime =
							FMath::Min(ServerTime - Bullet.Payload.ServerSpawnTime, Bullet.FlightDuration);
						FGeoNetcodeDebug::DrawBulletHit(Target, Bullet.GetLocation(SpentTime),
														Bullet.GetLocation(CurrentSpentTime), Bullet.Radius, Location,
														SpentTime);
					}

					return bInside;
				});
		}

		FGeoBullet& Bullet = Bullets[BulletIndex];
		float const SpentTime = ServerTime - Bullet.Payload.ServerSpawnTime;
		if (IsValid(Bullet.Visual))
		{
			Bullet.Visual->SetActorLocation(
				FVector(Bullet.GetLocation(FMath::Min(SpentTime, Bullet.FlightDuration)), ArbitraryCharacterZ));
		}

		if (SpentTime < Bullet.FlightDuration)
		{
			FGeoNetcodeDebug::DrawBullet(this, Bullet.GetLocation(SpentTime), Bullet.Direction, Bullet.Radius,
										 SpentTime);
		}

		bool const bFlightOver = SpentTime >= Bullet.FlightDuration;
		// A client has nobody to judge, so its bullets live exactly as long as they are drawn.
		bool const bBulletOver = bIsServer ? Bullet.Judge.IsOver(ServerTime) : bFlightOver || !IsValid(Bullet.Visual);
		// Checked again: moving it may just have ended it on a player.
		if (IsValid(Bullet.Visual) && (bFlightOver || bBulletOver))
		{
			Bullet.Visual->EndProjectileLife();
		}
		if (bBulletOver)
		{
			Bullets.RemoveAt(BulletIndex);
		}
	}
}

TStatId UGeoBulletSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UGeoBulletSubsystem, STATGROUP_Tickables);
}

float UGeoBulletSubsystem::ResolveFlightParam(EOverrideParam const Mode, float const OverrideValue,
											  float const SettingsValue, TCHAR const* const ParamName)
{
	ensureMsgf(Mode != EOverrideParam::KeepBlueprintDefaultValue,
			   TEXT("%hs: a bullet's %s cannot be kept from the projectile Blueprint — a bullet flies as data, with no "
					"projectile to read it from. Set it on the pattern's params."),
			   __FUNCTION__, ParamName);

	return Mode == EOverrideParam::OverrideValue ? OverrideValue : SettingsValue;
}

void UGeoBulletSubsystem::SpawnVisual(FGeoBullet& Bullet, float const SpentTime)
{
	FTransform const SpawnTransform(FRotator(0.f, Bullet.Payload.Yaw, 0.f),
									FVector(Bullet.GetLocation(SpentTime), ArbitraryCharacterZ));

	// The shot's one hit notification is the judge's to spend, never a drawing actor's.
	FAbilityPayload VisualPayload = Bullet.Payload;
	VisualPayload.HitNotified.Reset();

	// Placed at its flight position above; passing the spawn time here would have FinishSpawnProjectile's server-side
	// AdvanceProjectile sweep it forward by SpentTime a second time.
	Bullet.Visual = GeoASLib::FullySpawnProjectile(GetWorld(), Bullet.Params, SpawnTransform, VisualPayload,
												   /*EffectDataArray*/ {}, Bullet.Payload.ServerSpawnTime + SpentTime);
	if (IsValid(Bullet.Visual))
	{
		// The bullet is where it flies, so its actor is placed rather than moved.
		Bullet.Visual->ProjectileMovement->StopMovementImmediately();
		Bullet.Visual->OnProjectileEndLifeDelegate.AddUniqueDynamic(this, &ThisClass::OnVisualEnded);
	}
}

void UGeoBulletSubsystem::StopAtWall(FGeoBullet& Bullet, float const ServerTime)
{
	float const SweepEndTime = FMath::Min(ServerTime - Bullet.Payload.ServerSpawnTime, Bullet.FlightDuration);
	if (SweepEndTime <= Bullet.SweptTime)
	{
		return;
	}

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Bullet.Payload.SourceAvatar);

	// Characters only overlap this channel, so the first blocking hit is a wall.
	FVector const SweepStart(Bullet.GetLocation(Bullet.SweptTime), ArbitraryCharacterZ);
	FVector const SweepEnd(Bullet.GetLocation(SweepEndTime), ArbitraryCharacterZ);
	FHitResult Hit;
	bool const bHitWall =
		GetWorld()->SweepSingleByChannel(Hit, SweepStart, SweepEnd, FQuat::Identity, ECC_GeoProjectile,
										 FCollisionShape::MakeSphere(Bullet.Radius), QueryParams);
	FGeoNetcodeDebug::DrawWallSweep(this, SweepStart, SweepEnd, Bullet.Radius, bHitWall, Hit);

	if (bHitWall)
	{
		Bullet.FlightDuration = FMath::Lerp(Bullet.SweptTime, SweepEndTime, Hit.Time);
		Bullet.Judge.EndAt(Bullet.Payload.ServerSpawnTime + Bullet.FlightDuration);

		if (IsValid(Bullet.Visual))
		{
			Bullet.Visual->SetActorLocation(Hit.Location);
		}

		if (IsValid(Bullet.Visual)) // Visual can be destroyed on the move.
		{
			Bullet.Visual->OnSphereHit(nullptr, Hit.GetActor(), Hit.GetComponent(), FVector::ZeroVector, Hit);
		}
	}

	Bullet.SweptTime = SweepEndTime;
}

void UGeoBulletSubsystem::OnVisualEnded(AGeoProjectile* Projectile)
{
	Projectile->OnProjectileEndLifeDelegate.RemoveDynamic(this, &ThisClass::OnVisualEnded);

	// A bullet fired this frame is still pending, and its actor can already have ended on the player it spawned on.
	for (TArray<FGeoBullet>* const BulletList : {&Bullets, &PendingBullets})
	{
		for (FGeoBullet& Bullet : *BulletList)
		{
			if (Bullet.Visual == Projectile)
			{
				Bullet.Visual = nullptr;
			}
		}
	}
}
