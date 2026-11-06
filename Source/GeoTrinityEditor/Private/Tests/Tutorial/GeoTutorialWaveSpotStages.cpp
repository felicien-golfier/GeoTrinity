// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "GeoTutorialWaveSpotStages.h"

#include "Characters/HiddenCasterCharacter.h"
#include "Characters/PlayableCharacter.h"
#include "Encounter/GeoEncounter.h"
#include "Encounter/GeoEncounterSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

namespace GeoWaveSpots
{
	FName const LateralWaveAbility = "Ability.Spell.LateralWave";
	FName const ColorsStep = "ZoneColors";
	FName const LethalStep = "LethalWave";
	FName const SafeSpotsStep = "SafeSpots";

	constexpr int32 MaxSpots = 20;
	/** A lane is 2 * 190 wide (tutorial_waves.py): a spot this far from every lane's centre is out of it. */
	constexpr float OutOfLaneDistance = 250.f;
	constexpr float SpotX[] = {-650.f, -325.f, 0.f, 325.f, 650.f};
	constexpr float SpotY[] = {-1050.f, -700.f, -350.f, 0.f, 350.f, 700.f, 1050.f};

	/** Y of each lane of a safe-spot wave (tutorial_encounters.py SAFE_WAVE_LANES); SafeWave 0 is the lethal wave,
	 * none. */
	TArray<float> GetLanes(int32 const SafeWave)
	{
		TArray<float> Lanes;
		if (SafeWave == 1)
		{
			Lanes = {-650.f, 0.f, 650.f};
		}
		else if (SafeWave == 2)
		{
			Lanes = {-300.f, 500.f};
		}
		else if (SafeWave == 3)
		{
			Lanes = {250.f};
		}

		return Lanes;
	}

	/** The spots a pawn dies at on SafeWave: the grid minus the lanes, thinned to MaxSpots evenly. */
	TArray<FVector2D> GetSpots(int32 const SafeWave)
	{
		float const CentreX = GeoTutorial::RoomCentreX(GeoTutorial::Zones);
		TArray<float> const Lanes = GetLanes(SafeWave);
		TArray<FVector2D> Candidates;
		for (float const X : SpotX)
		{
			for (float const Y : SpotY)
			{
				bool const bOutOfEveryLane = !Lanes.ContainsByPredicate(
					[Y](float const LaneY)
					{
						return FMath::Abs(Y - LaneY) < OutOfLaneDistance;
					});
				if (bOutOfEveryLane)
				{
					Candidates.Add(FVector2D(CentreX + X, Y));
				}
			}
		}

		int32 const Count = FMath::Min(Candidates.Num(), MaxSpots);
		TArray<FVector2D> Spots;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			Spots.Add(Candidates[Index * Candidates.Num() / Count]);
		}

		return Spots;
	}

	/** Where one spot's stage stands. */
	struct FRun
	{
		bool bStarted = false;
		bool bPlaced = false;
		/** The wave this stage waits for was seen firing in an earlier frame: from now on its end is a survival. */
		bool bSeenWave = false;
		int32 BaseCount = 0;
		double StartSeconds = 0.;
	};
} // namespace GeoWaveSpots

namespace GeoTutorialStages
{
	void AddWaveSpotStages(FGeoTutorialBot& Bot, int32 const WaveNumber, bool const bAfterColors)
	{
		using namespace GeoWaveSpots;

		int32 const SafeWave = WaveNumber - 1;
		TArray<FVector2D> const Spots = GetSpots(SafeWave);
		TFunction<AActor*()> const GetCaster = [&Bot]() -> AActor*
		{
			UWorld* const World = Bot.GetWorld();
			return World ? World->GetSubsystem<UGeoEncounterSubsystem>()->GetSharedCaster() : nullptr;
		};
		// Binds on the bot's first tick, before the first wave: the shared caster is spawned by this very call.
		int32 const Watch = Bot.WatchActivations(GetCaster, LateralWaveAbility);
		float const CentreX = GeoTutorial::RoomCentreX(GeoTutorial::Zones);
		for (int32 Index = 0; Index < Spots.Num(); ++Index)
		{
			FVector2D const Spot = Spots[Index];
			FString const Label =
				FString::Printf(TEXT("Wave %d, spot %d/%d (%.0f, %.0f)%s"), WaveNumber, Index + 1, Spots.Num(),
								Spot.X - CentreX, Spot.Y, bAfterColors ? TEXT(", after the colours") : TEXT(""));
			TSharedRef<FRun> Run = MakeShared<FRun>();
			Bot.AddStage(
				Label,
				[&Bot, GetCaster, Watch, SafeWave, Spot, Label, bAfterColors, CentreX, Run](bool)
				{
					APlayableCharacter* const Pawn = Bot.GetPawn();
					AGeoEncounter* const Encounter = Bot.FindEncounter(GeoTutorial::Zones);
					bool bDone = false;
					if (!Run->bStarted)
					{
						// The pawn of the stage before respawns after its death: start from a living one.
						if (Pawn && !Pawn->IsDead())
						{
							Run->bStarted = true;
							Run->BaseCount = Bot.GetWatchCount(Watch);
							Run->StartSeconds = Bot.GetWorld()->GetTimeSeconds();
							FGeoTutorialBot::SetHealthRatio(Pawn, 1.f);
							Bot.SetInvincible(bAfterColors);
							if (bAfterColors)
							{
								Encounter->GoToStep(ColorsStep);
								AActor const* const Shield = Bot.FindByLabel(TEXT("Tut_Zones_ZonePoint_Shield"));
								AActor const* const Heal = Bot.FindByLabel(TEXT("Tut_Zones_ZonePoint_Heal"));
								if (Bot.Test.TestTrue(TEXT("The Shield and Heal zone points exist"), Shield && Heal))
								{
									Bot.MovePawn(
										(FVector2D(Shield->GetActorLocation()) + FVector2D(Heal->GetActorLocation()))
										* 0.5f);
								}
							}
							else if (SafeWave == 0)
							{
								Encounter->GoToStep(LethalStep);
								Bot.MovePawn(Spot);
								Run->bPlaced = true;
							}
							else
							{
								Encounter->GoToStep(SafeSpotsStep);
								Bot.MovePawn(FVector2D(CentreX, GetLanes(1)[0]));
							}
						}
					}
					else if (Pawn)
					{
						int32 const Fired = Bot.GetWatchCount(Watch) - Run->BaseCount;
						int32 const NeededFired = FMath::Max(SafeWave, 1);
						if (bAfterColors && !Run->bPlaced && Encounter->GetCurrentStepName() == LethalStep)
						{
							Bot.SetInvincible(false);
							Bot.MovePawn(Spot);
							Run->bPlaced = true;
						}
						else if (SafeWave >= 1 && !Run->bPlaced && !Pawn->IsDead())
						{
							// Survive the waves before in their first lane, then take the spot as the wave fires.
							if (Fired >= SafeWave)
							{
								Bot.MovePawn(Spot);
								Run->bPlaced = true;
							}
							else
							{
								FVector2D const Lane(CentreX, GetLanes(FMath::Max(Fired, 1))[0]);
								if (FVector2D::Distance(FVector2D(Pawn->GetActorLocation()), Lane) > 1.f)
								{
									Bot.MovePawn(Lane);
								}
							}
						}

						if (Pawn->IsDead())
						{
							bDone = true;
							if (Run->bPlaced)
							{
								Bot.Test.TestTrue(
									*FString::Printf(TEXT("%s: killed by the wave, not before it"), *Label),
									Fired >= NeededFired);
								UE_LOG(LogTemp, Display, TEXT("TutorialBot wavespot: %s killed after %.2f s"), *Label,
									   Bot.GetWorld()->GetTimeSeconds() - Run->StartSeconds);
							}
							else
							{
								Bot.Test.AddError(FString::Printf(TEXT("%s: died on wave %d before the spot was taken"),
																  *Label, Fired + 1));
							}
						}
						else if (Run->bPlaced && Fired >= NeededFired)
						{
							if (Run->bSeenWave
								&& (Fired > NeededFired
									|| !FGeoTutorialBot::IsAbilityActive(GetCaster(), LateralWaveAbility)))
							{
								bDone = true;
								Bot.Test.AddError(
									FString::Printf(TEXT("%s: the wave ended and the pawn is alive"), *Label));
							}

							Run->bSeenWave = true;
						}
					}

					if (bDone)
					{
						Bot.SetInvincible(true);
					}

					return bDone;
				});
		}

		Bot.AddStage(TEXT("Wave spots: the pawn lives again"),
					 [&Bot](bool)
					 {
						 APlayableCharacter const* const Pawn = Bot.GetPawn();
						 return Pawn && !Pawn->IsDead();
					 });
	}
} // namespace GeoTutorialStages

#endif
