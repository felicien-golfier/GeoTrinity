// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Tool/GeoNetcodeDebug.h"

#include "Characters/Component/GeoCharacterMovementComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/HitResult.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Tool/UGeoGameplayLibrary.h"
#include "VisualLogger/VisualLogger.h"

static TAutoConsoleVariable CVarDebugNetcode(
	TEXT("Geo.DebugNetcode"), false,
	TEXT("Draw and visual-log lag compensation: pose history (white arrows) and rewound poses (magenta, linked to the "
		 "current pose), hazard samples (green inside, red outside) over their interpolation span (yellow), bullets "
		 "(cyan, with heading), their wall sweeps (orange) and their rewound position at a hit (red)"));

void FGeoNetcodeDebug::DrawRecordedPose(AActor const* Actor, FGeoPose const& Pose, float const Duration)
{
	if (CVarDebugNetcode.GetValueOnGameThread())
	{
		FVector const Tip = Pose.Location + FRotator(0.f, Pose.Yaw, 0.f).Vector() * 60.f;
		DrawDebugDirectionalArrow(Actor->GetWorld(), Pose.Location, Tip, 20.f, FColor::White, false, Duration);
		UE_VLOG_ARROW(Actor, LogGeoTrinity, Log, Pose.Location, Tip, FColor::White, TEXT("Recorded pose at %.3f"),
					  Pose.ServerTime);
	}
}

void FGeoNetcodeDebug::DrawRewoundPose(AActor const* Actor, FGeoPose const& RewoundPose, FGeoPose const& CurrentPose)
{
	if (CVarDebugNetcode.GetValueOnGameThread())
	{
		FVector const Tip = RewoundPose.Location + FRotator(0.f, RewoundPose.Yaw, 0.f).Vector() * 60.f;
		DrawDebugDirectionalArrow(Actor->GetWorld(), RewoundPose.Location, Tip, 20.f, FColor::Magenta, false, 0.f);
		DrawDebugLine(Actor->GetWorld(), RewoundPose.Location, CurrentPose.Location, FColor::Magenta, false, 0.f);
		UE_VLOG_ARROW(Actor, LogGeoTrinity, Log, RewoundPose.Location, Tip, FColor::Magenta,
					  TEXT("Rewound pose at %.3f"), RewoundPose.ServerTime);
		UE_VLOG_SEGMENT(Actor, LogGeoTrinity, Log, RewoundPose.Location, CurrentPose.Location, FColor::Magenta,
						TEXT("Rewound %.0f ms"), (CurrentPose.ServerTime - RewoundPose.ServerTime) * 1000.f);
	}
}

void FGeoNetcodeDebug::DrawHazardJudge(AActor const* Target, FVector2D const LastLocation, float const LastTime,
									   FVector2D const Location, float const SpentTime, int32 const NextSampleIndex,
									   int32 const SampleCount)
{
	if (CVarDebugNetcode.GetValueOnGameThread())
	{
		float const ServerTime = GeoLib::GetServerTime(Target);
		float const PerceivedServerTime = GeoLib::GetPerceivedServerTime(Target);
		UE_VLOG(Target, LogGeoTrinity, Log,
				TEXT("Hazard judge: server %.3f, perceived %.3f (%.0f ms behind), spent %.3f, sample %d/%d"),
				ServerTime, PerceivedServerTime, (ServerTime - PerceivedServerTime) * 1000.f, SpentTime,
				NextSampleIndex, SampleCount);

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
	if (CVarDebugNetcode.GetValueOnGameThread())
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
	if (CVarDebugNetcode.GetValueOnGameThread())
	{
		FVector const BulletLocation(Location, ArbitraryCharacterZ);
		FVector const HeadingTip = BulletLocation + FVector(Direction, 0.f) * (Radius + 40.f);
		DrawDebugSphere(Owner->GetWorld(), BulletLocation, Radius, 12, FColor::Cyan, false, 0.f);
		DrawDebugDirectionalArrow(Owner->GetWorld(), BulletLocation, HeadingTip, 15.f, FColor::Cyan, false, 0.f);
		UE_VLOG_ARROW(Owner, LogGeoTrinity, Log, BulletLocation, HeadingTip, FColor::Cyan,
					  TEXT("Bullet at spent time %.3f"), SpentTime);
	}
}

void FGeoNetcodeDebug::DrawBulletHit(AActor const* Target, FVector2D const BulletLocation, float const Radius,
									 FVector2D const TargetLocation, float const SpentTime)
{
	if (CVarDebugNetcode.GetValueOnGameThread())
	{
		FVector const RewoundBulletLocation(BulletLocation, ArbitraryCharacterZ);
		DrawDebugSphere(Target->GetWorld(), RewoundBulletLocation, Radius, 12, FColor::Red, false, 2.f);
		DrawDebugLine(Target->GetWorld(), RewoundBulletLocation, FVector(TargetLocation, ArbitraryCharacterZ),
					  FColor::Red, false, 2.f);
		UE_VLOG_LOCATION(Target, LogGeoTrinity, Log, RewoundBulletLocation, Radius, FColor::Red,
						 TEXT("Bullet hit at spent time %.3f"), SpentTime);
	}
}

void FGeoNetcodeDebug::DrawWallSweep(UObject const* Owner, FVector const& Start, FVector const& End,
									 float const Radius, bool const bHitWall, FHitResult const& Hit)
{
	if (CVarDebugNetcode.GetValueOnGameThread())
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
