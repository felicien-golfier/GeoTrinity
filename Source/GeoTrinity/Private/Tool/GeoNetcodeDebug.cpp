// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Tool/GeoNetcodeDebug.h"

#include "Characters/Component/GeoCharacterMovementComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/HitResult.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Tool/UGeoGameplayLibrary.h"
#include "VisualLogger/VisualLogger.h"

static TAutoConsoleVariable CVarDebugNetcode(
	TEXT("Geo.DebugNetcode"), false,
	TEXT("Draw and visual-log lag compensation, in PIE on the host only. Everything at the current time T is cyan, "
		 "everything a lagging player's view puts in the past is magenta, linked by a magenta line. Pose history "
		 "(white arrows); a remote player's server pose (magenta points) and, in single-process PIE, where its own "
		 "client has it at T (cyan circle and arrows); rewound poses (magenta) beside the current pose (cyan); hazard "
		 "samples (green inside, red outside) over their interpolation span (yellow). A current pose is drawn +15 Y "
		 "so it stays visible over its rewound one. Bullets have their own Geo.DebugNetcode.Projectiles."));

static TAutoConsoleVariable CVarDebugNetcodeProjectiles(
	TEXT("Geo.DebugNetcode.Projectiles"), false,
	TEXT("Draw and visual-log bullets, in PIE on the host only, with Geo.DebugNetcode's colour code: bullets (cyan, "
		 "with heading), their wall sweeps (orange) and their rewound position at a hit (magenta, linked to where the "
		 "target stood then) beside where they are at T (cyan, drawn +15 Y)."));

static FColor const CurrentColor = FColor::Cyan;
static FColor const PastColor = FColor::Magenta;

// Shifts what is drawn at the current time sideways, so it stays visible where it overlaps its past counterpart.
static FVector const CurrentDrawOffset(0.f, 15.f, 0.f);

void FGeoNetcodeDebug::DrawRecordedPose(AActor const* Actor, FGeoPose const& Pose, float const Duration)
{
	if (ShouldDraw(Actor, CVarDebugNetcode.GetValueOnGameThread()))
	{
		FVector const Tip = Pose.Location + FRotator(0.f, Pose.Yaw, 0.f).Vector() * 60.f;
		DrawDebugDirectionalArrow(Actor->GetWorld(), Pose.Location, Tip, 20.f, FColor::White, false, Duration);
		UE_VLOG_ARROW(Actor, LogGeoTrinity, Log, Pose.Location, Tip, FColor::White, TEXT("Recorded pose at %.3f"),
					  Pose.ServerTime);
	}
}

void FGeoNetcodeDebug::DrawClientPose(APawn const* ServerPawn, float const Duration)
{
	if (ShouldDraw(ServerPawn, CVarDebugNetcode.GetValueOnGameThread()) && ServerPawn->IsPlayerControlled()
		&& !ServerPawn->IsLocallyControlled())
	{
		if (APawn const* const ClientPawn = FindClientPawn(ServerPawn))
		{
			UWorld const* const World = ServerPawn->GetWorld();
			FVector const ServerLocation = ServerPawn->GetActorLocation();
			FVector const ClientLocation = ClientPawn->GetActorLocation();
			FVector const Tip = ClientLocation + FRotator(0.f, ClientPawn->GetActorRotation().Yaw, 0.f).Vector() * 60.f;
			// Foreground, or the pawn's own mesh hides whatever sits at its location.
			DrawDebugPoint(World, ServerLocation, 12.f, PastColor, false, Duration, SDPG_Foreground);
			DrawDebugDirectionalArrow(World, ClientLocation, Tip, 20.f, CurrentColor, false, Duration, SDPG_Foreground,
									  2.f);
			DrawDebugCircle(World, ClientLocation, ClientPawn->GetSimpleCollisionRadius(), 24, CurrentColor, false, 0.f,
							SDPG_Foreground, 4.f, FVector(1.f, 0.f, 0.f), FVector(0.f, 1.f, 0.f), false);
			DrawDebugLine(World, ServerLocation, ClientLocation, PastColor, false, 0.f, SDPG_Foreground, 2.f);
			UE_VLOG_LOCATION(ServerPawn, LogGeoTrinity, Log, ServerLocation, 12.f, PastColor,
							 TEXT("Server's pose of the client"));
			UE_VLOG_LOCATION(ServerPawn, LogGeoTrinity, Log, ClientLocation, ClientPawn->GetSimpleCollisionRadius(),
							 CurrentColor, TEXT("Client's own pose"));
			UE_VLOG_SEGMENT(ServerPawn, LogGeoTrinity, Log, ServerLocation, ClientLocation, PastColor,
							TEXT("Client ahead by %.0f units"),
							FVector::Dist2D(ClientLocation, ServerLocation));
		}
	}
}

bool FGeoNetcodeDebug::ShouldDraw(UObject const* WorldContext, bool const bEnabled)
{
	UWorld const* const World = WorldContext->GetWorld();
	return bEnabled && (!World->IsPlayInEditor() || GeoLib::IsServer(World));
}

APawn const* FGeoNetcodeDebug::FindClientPawn(APawn const* ServerPawn)
{
	int32 const PlayerId = ServerPawn->GetPlayerState()->GetPlayerId();
	for (FWorldContext const& Context : GEngine->GetWorldContexts())
	{
		UWorld const* const World = Context.World();
		if (World && World != ServerPawn->GetWorld() && World->GetNetMode() == NM_Client)
		{
			for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
			{
				APawn const* const ClientPawn = It->Get()->GetPawn();
				if (IsValid(ClientPawn) && ClientPawn->GetPlayerState()
					&& ClientPawn->GetPlayerState()->GetPlayerId() == PlayerId)
				{
					return ClientPawn;
				}
			}
		}
	}

	return nullptr;
}

void FGeoNetcodeDebug::DrawRewoundPose(AActor const* Actor, FGeoPose const& RewoundPose, FGeoPose const& CurrentPose)
{
	if (ShouldDraw(Actor, CVarDebugNetcode.GetValueOnGameThread()))
	{
		FVector const RewoundTip = RewoundPose.Location + FRotator(0.f, RewoundPose.Yaw, 0.f).Vector() * 60.f;
		DrawDebugDirectionalArrow(Actor->GetWorld(), RewoundPose.Location, RewoundTip, 20.f, PastColor, false, 2.f);
		UE_VLOG_ARROW(Actor, LogGeoTrinity, Log, RewoundPose.Location, RewoundTip, PastColor,
					  TEXT("Rewound pose at %.3f"), RewoundPose.ServerTime);

		FVector const CurrentLocation = CurrentPose.Location + CurrentDrawOffset;
		FVector const CurrentTip = CurrentLocation + FRotator(0.f, CurrentPose.Yaw, 0.f).Vector() * 40.f;
		DrawDebugDirectionalArrow(Actor->GetWorld(), CurrentLocation, CurrentTip, 12.f, CurrentColor, false, 2.f);
		UE_VLOG_ARROW(Actor, LogGeoTrinity, Log, CurrentLocation, CurrentTip, CurrentColor,
					  TEXT("Current pose at %.3f (drawn +15 Y)"), CurrentPose.ServerTime);

		DrawDebugLine(Actor->GetWorld(), RewoundPose.Location, CurrentLocation, PastColor, false, 0.f);
		UE_VLOG_SEGMENT(Actor, LogGeoTrinity, Log, RewoundPose.Location, CurrentLocation, PastColor,
						TEXT("Rewound %.0f ms, %.0f units"), (CurrentPose.ServerTime - RewoundPose.ServerTime) * 1000.f,
						FVector::Dist2D(RewoundPose.Location, CurrentPose.Location));
	}
}

void FGeoNetcodeDebug::DrawHazardJudge(AActor const* Target, FVector2D const LastLocation, float const LastTime,
									   FVector2D const Location, float const SpentTime, int32 const NextSampleIndex,
									   float const Duration)
{
	if (ShouldDraw(Target, CVarDebugNetcode.GetValueOnGameThread()))
	{
		float const ServerTime = GeoLib::GetServerTime(Target);
		float const PerceivedServerTime = GeoLib::GetPerceivedServerTime(Target);
		UE_VLOG(Target, LogGeoTrinity, Log,
				TEXT("Hazard judge: server %.3f, perceived %.3f (%.0f ms behind), spent %.3f, sample %d, ends at %.3f"),
				ServerTime, PerceivedServerTime, (ServerTime - PerceivedServerTime) * 1000.f, SpentTime,
				NextSampleIndex, Duration);

		float const Z = Target->GetActorLocation().Z;
		FVector const Start(LastLocation, Z);
		FVector const End(Location, Z);
		DrawDebugLine(Target->GetWorld(), Start, End, FColor::Yellow, false, 2.f);
		UE_VLOG_SEGMENT(Target, LogGeoTrinity, Log, Start, End, FColor::Yellow, TEXT("Interpolated from %.3f to %.3f"),
						LastTime, SpentTime);
	}
}

void FGeoNetcodeDebug::DrawHazardSample(AActor const* Target, FVector2D const Location, float const SpentTime,
										int32 const SampleIndex, bool const bInside)
{
	if (ShouldDraw(Target, CVarDebugNetcode.GetValueOnGameThread()))
	{
		FVector const SampleLocation(Location, Target->GetActorLocation().Z);
		FColor const Color = bInside ? FColor::Green : FColor::Red;
		DrawDebugPoint(Target->GetWorld(), SampleLocation, 8.f, Color, false, 2.f);
		UE_VLOG_LOCATION(Target, LogGeoTrinity, Log, SampleLocation, 8.f, Color, TEXT("Sample %d at %.3f"),
						 SampleIndex, SpentTime);
	}
}

void FGeoNetcodeDebug::DrawBullet(UObject const* Owner, FVector2D const Location, FVector2D const Direction,
								  float const Radius, float const SpentTime)
{
	if (ShouldDraw(Owner, CVarDebugNetcodeProjectiles.GetValueOnGameThread()))
	{
		FVector const BulletLocation(Location, ArbitraryCharacterZ);
		FVector const HeadingTip = BulletLocation + FVector(Direction, 0.f) * (Radius + 40.f);
		DrawDebugSphere(Owner->GetWorld(), BulletLocation, Radius, 12, CurrentColor, false, 0.f);
		DrawDebugDirectionalArrow(Owner->GetWorld(), BulletLocation, HeadingTip, 15.f, CurrentColor, false, 0.f);
		UE_VLOG_ARROW(Owner, LogGeoTrinity, Log, BulletLocation, HeadingTip, CurrentColor,
					  TEXT("Bullet at spent time %.3f"), SpentTime);
	}
}

void FGeoNetcodeDebug::DrawBulletHit(AActor const* Target, FVector2D const BulletLocation,
									 FVector2D const CurrentBulletLocation, float const Radius,
									 FVector2D const TargetLocation, float const SpentTime)
{
	if (ShouldDraw(Target, CVarDebugNetcodeProjectiles.GetValueOnGameThread()))
	{
		FVector const RewoundBulletLocation(BulletLocation, ArbitraryCharacterZ);
		DrawDebugSphere(Target->GetWorld(), RewoundBulletLocation, Radius, 12, PastColor, false, 2.f);
		DrawDebugLine(Target->GetWorld(), RewoundBulletLocation, FVector(TargetLocation, ArbitraryCharacterZ),
					  PastColor, false, 2.f);
		UE_VLOG_LOCATION(Target, LogGeoTrinity, Log, RewoundBulletLocation, Radius, PastColor,
						 TEXT("Bullet hit at spent time %.3f"), SpentTime);

		FVector const CurrentLocation = FVector(CurrentBulletLocation, ArbitraryCharacterZ) + CurrentDrawOffset;
		DrawDebugSphere(Target->GetWorld(), CurrentLocation, Radius, 12, CurrentColor, false, 2.f);
		DrawDebugLine(Target->GetWorld(), RewoundBulletLocation, CurrentLocation, PastColor, false, 2.f);
		UE_VLOG_LOCATION(Target, LogGeoTrinity, Log, CurrentLocation, Radius, CurrentColor,
						 TEXT("Bullet now, %.0f units ahead of its hit (drawn +15 Y)"),
						 FVector2D::Distance(BulletLocation, CurrentBulletLocation));
	}
}

void FGeoNetcodeDebug::DrawWallSweep(UObject const* Owner, FVector const& Start, FVector const& End,
									 float const Radius, bool const bHitWall, FHitResult const& Hit)
{
	if (ShouldDraw(Owner, CVarDebugNetcodeProjectiles.GetValueOnGameThread()))
	{
		DrawDebugLine(Owner->GetWorld(), Start, End, FColor::Orange, false, 2.f);
		UE_VLOG_SEGMENT(Owner, LogGeoTrinity, Log, Start, End, FColor::Orange, TEXT("Wall sweep"));
		if (bHitWall)
		{
			DrawDebugSphere(Owner->GetWorld(), Hit.Location, Radius, 12, FColor::Orange, false, 2.f);
			UE_VLOG_LOCATION(Owner, LogGeoTrinity, Log, Hit.Location, Radius, FColor::Orange,
							 TEXT("Bullet stopped on %s"), *GetNameSafe(Hit.GetActor()));
		}
	}
}
