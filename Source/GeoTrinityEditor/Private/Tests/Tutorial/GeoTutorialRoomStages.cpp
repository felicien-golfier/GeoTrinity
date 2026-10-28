// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "GeoTutorialRoomStages.h"

#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Arena/GeoArena.h"
#include "Actor/Buttons/GeoTeleporter.h"
#include "Actor/Deployable/Zones/GeoEffectZone.h"
#include "Actor/GeoGemDrop.h"
#include "Characters/EnemyCharacter.h"
#include "Characters/GeoSummonedDummy.h"
#include "Characters/HiddenCasterCharacter.h"
#include "Characters/PlayableCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Encounter/GeoEncounterSubsystem.h"
#include "EngineUtils.h"
#include "GameClasses/GeoGameInstance.h"
#include "GameClasses/Player/GeoPlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "Gem/GeoGemProfileSave.h"
#include "Gem/GeoGemSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Editor.h"

namespace GeoTutorialStages
{
	// Short unique fragments of the Cat lines of tutorial_encounters.py: keep in step with the script.
	constexpr TCHAR const* LandingFirstLine = TEXT("Landed. Welcome");
	constexpr TCHAR const* LandingWalkLine = TEXT("Hold W, A, S or D");
	constexpr TCHAR const* LandingPortalLine = TEXT("See the round portal");
	constexpr TCHAR const* LandingNagLine = TEXT("Still standing there");
	constexpr TCHAR const* LandingLines[] = {
		TEXT("Landed. Welcome"),	  TEXT("bowels of the game"),  TEXT("Rule one"),
		TEXT("How do I know that"),	  TEXT("you're the Healer"),   TEXT("Hold W, A, S or D"),
		TEXT("See the round portal"), TEXT("Still standing there")};

	constexpr TCHAR const* DashArrivalLine = TEXT("You reached a portal by walking");
	constexpr TCHAR const* DashFallLines[] = {TEXT("you fell and died"),	TEXT("Dead again. In the same hole"),
											  TEXT("Again. Press"),			TEXT("A hole is beating you"),
											  TEXT("carve it on a plaque"), TEXT("I started a tally")};
	constexpr TCHAR const* DashFirstFallLines[] = {TEXT("you fell and died"), TEXT("Here's the trick"),
												   TEXT("does NOT make you invincible")};
	constexpr TCHAR const* DashStunnedLine = TEXT("Nobody does that on the first try");
	constexpr TCHAR const* DashReliefLine = TEXT("In the story I'll tell");
	constexpr TCHAR const* DashUselessLine = TEXT("Useless humans with a dash");
	constexpr TCHAR const* DashIdleLines[] = {TEXT("I'm just here, whistling"), TEXT("Staring at a hole"),
											  TEXT("more than four keys"), TEXT("naming the pixels")};

	constexpr TCHAR const* ShootIntroLine = TEXT("New room, new toy");
	constexpr TCHAR const* ShootQuestLine = TEXT("Oh look, a quest");
	constexpr TCHAR const* ShootQuips[] = {TEXT("3 out of 100"), TEXT("Don't mind the respawn"), TEXT("named Greg")};
	constexpr TCHAR const* ShootStopLine = TEXT("I'm bored and I'm only watching");
	constexpr TCHAR const* ShootAbandonedLine = TEXT("Quest abandoned");
	constexpr TCHAR const* ShootNextToyLine = TEXT("Next toy");

	constexpr TCHAR const* DeployIntroLine = TEXT("Second toy");
	constexpr TCHAR const* DeployFullLine = TEXT("healer job suits you");

	constexpr TCHAR const* SpecialIntroLine = TEXT("Third toy");
	constexpr TCHAR const* SpecialStings[] = {TEXT("You killed one Dummy and healed"), TEXT("Two Dummies handled")};
	constexpr TCHAR const* SpecialSurrenderLine = TEXT("I surrender");
	constexpr TCHAR const* SpecialDoneLine = TEXT("You hurt one and helped the other");

	constexpr TCHAR const* DummiesIntroLine = TEXT("Plot twist");
	constexpr TCHAR const* DummiesDoneLine = TEXT("officially beaten the cardboard");

	constexpr TCHAR const* ZonesIntroLine = TEXT("Welcome to the Color Room");
	constexpr float ColorZoneMaxRadius = 1350.f; // CT_Tutorial row ColorZoneMaxRadius (tutorial_assets.py)
	constexpr TCHAR const* ZonesLearnedLine = TEXT("Colors learned?");
	constexpr TCHAR const* ZonesLethalLine = TEXT("Big wave is coming");
	constexpr TCHAR const* ZonesLethalDeathLine = TEXT("Oh, it hurts?");
	constexpr TCHAR const* ZonesRetryLine = TEXT("dancing, but you die");
	constexpr TCHAR const* ZonesSurvivedLine = TEXT("nicely labeled");

	constexpr TCHAR const* MinibossIntroLine = TEXT("Meet the Miniboss");
	constexpr TCHAR const* MinibossTips[] = {TEXT("those Dummies land hard"), TEXT("A dead Dummy switches sides"),
											 TEXT("Every third Dummy")};
	constexpr TCHAR const* MinibossCallouts[] = {TEXT("Genji"), TEXT("Poor dummy"), TEXT("**I NEED HEALING.**")};
	constexpr TCHAR const* MinibossKillLine = TEXT("come back to the real world");

	constexpr TCHAR const* FirstRoomLines[] = {TEXT(""),		 DashArrivalLine,  ShootIntroLine, DeployIntroLine,
											   SpecialIntroLine, DummiesIntroLine, ZonesIntroLine, MinibossIntroLine};

	FName const DashAbility = "Ability.Spell.Dash";
	FName const BasicBeamAbility = "Ability.Spell.BasicBeam";
	FName const HealingZoneAbility = "Ability.Spell.HealingZone";
	FName const SpecialAbility = "Ability.Spell.MoiraBeam";
	FName const LateralWaveAbility = "Ability.Spell.LateralWave";
	FName const SalveAbility = "Ability.Spell.TutorialSalve";
	FName const AllySalveAbility = "Ability.Spell.TutorialSalve.Ally";
	FName const ShooterAbility = "Ability.Spell.ProjectileToAllPlayers";

	/** Each class ability and the room that unlocks it. */
	TPair<FName, int32> const UnlockRooms[] = {{DashAbility, GeoTutorial::Dash},
											   {BasicBeamAbility, GeoTutorial::Shoot},
											   {HealingZoneAbility, GeoTutorial::Deploy},
											   {SpecialAbility, GeoTutorial::Special}};

	FName const ShootDummyTag = "TutorialShootDummy";
	FName const AllyTag = "TutorialAlly";
	FName const SpecialAllyTag = "TutorialSpecialAlly";
	FName const SpecialEnemyTag = "TutorialRudeDummy";
	FName const ChaserTag = "TutorialChaser";
	FName const ShooterTag = "TutorialShooter";
	FName const MoverTag = "TutorialMover";
	FName const MinibossTag = "TutorialMiniboss";
	FName const MinibossAllyTag = "TutorialMinibossAlly";

	constexpr int32 QuestKills = 12;
	constexpr int32 QuestDummies = 9;
	constexpr float HurtAllyMaxRatio = 0.5f;
	constexpr float DashStartInset = 120.f;
	constexpr float DashFarFloorX = 200.f;
	constexpr float RespawnReach = 200.f;
	constexpr int32 DashFallCount = 7;
	constexpr double PauseHoldSeconds = 7.;
	constexpr float SafeLaneRadius = 190.f;
	constexpr TCHAR const* MinibossChips[] = {TEXT("Power"), TEXT("Guard"),		TEXT("Vigor"), TEXT("Mend"),
											  TEXT("Swift"), TEXT("Precision"), TEXT("Edge")};
	constexpr int32 MinibossPointCount = 3;
	constexpr float MinibossPointReach = 100.f;
	constexpr int32 MinibossEnterFrames = 10;
	constexpr float PlayerStartReach = 300.f;
	constexpr float WaveLaneBackoff = 300.f;
	constexpr double WaveSurviveSeconds = 1.5;

	/** Where the pawn stands for each wave of one run of the Zones room's safe-spot waves (Y from the wave start's
	 * row); DeathWave is the wave it dies on, 0 = it clears. tutorial_encounters.py SAFE_WAVE_LANES: wave 1 has lanes
	 * -650, 0 and 650, wave 2 -300 and 500, wave 3 250; a spot 400 from every lane (SafeLaneRadius 190) is out of them.
	 */
	struct FSafeSeries
	{
		TArray<float> LaneY;
		int32 DeathWave = 0;
	};

	FSafeSeries MakeSafeSeries(TArray<float> LaneY, int32 const DeathWave)
	{
		FSafeSeries Series;
		Series.LaneY = MoveTemp(LaneY);
		Series.DeathWave = DeathWave;
		return Series;
	}

	TArray<FSafeSeries> GetSafeSeries(bool const bAll)
	{
		TArray<FSafeSeries> Series;
		if (bAll)
		{
			Series.Add(MakeSafeSeries({-650.f, 100.f}, 2));
			Series.Add(MakeSafeSeries({650.f, 500.f, -500.f}, 3));
		}

		Series.Add(MakeSafeSeries({0.f, -300.f, 250.f}, 0));
		return Series;
	}

	FString ExitPad(int32 const Room)
	{
		return GeoTutorial::ExitPadName(Room);
	}

	float Distance2D(AActor const* const Actor, FVector2D const Location)
	{
		return FVector2D::Distance(FVector2D(Actor->GetActorLocation()), Location);
	}

	AEnemyCharacter* FindFirstLiving(FGeoTutorialBot const& Bot, FName const Tag)
	{
		TArray<AEnemyCharacter*> const Living = Bot.FindLiving(Tag);
		return Living.IsEmpty() ? nullptr : Living[0];
	}

	AEnemyCharacter* FindMiniboss(FGeoTutorialBot const& Bot)
	{
		return FindFirstLiving(Bot, MinibossTag);
	}

	/** The Miniboss ally's health ratio; an ally that died fails the test instead of crashing it. */
	void SetAllyRatio(FGeoTutorialBot& Bot, float const Ratio)
	{
		AEnemyCharacter* const Ally = FindFirstLiving(Bot, MinibossAllyTag);
		if (Bot.Test.TestNotNull(TEXT("The Miniboss ally is alive"), Ally))
		{
			FGeoTutorialBot::SetHealthRatio(Ally, Ratio);
		}
	}

	AGeoSummonedDummy* FindLivingSummon(FGeoTutorialBot const& Bot)
	{
		for (TActorIterator<AGeoSummonedDummy> It(Bot.GetWorld()); It; ++It)
		{
			if (IsValid(*It) && !It->IsDead())
			{
				return *It;
			}
		}

		return nullptr;
	}

	/** Every living dummy of the Miniboss room but the boss: the summons and the allies, the sources of the wave's
	 * lanes. */
	TArray<AEnemyCharacter*> FindLivingDummies(FGeoTutorialBot const& Bot)
	{
		TArray<AEnemyCharacter*> Dummies;
		for (TActorIterator<AEnemyCharacter> It(Bot.GetWorld()); It; ++It)
		{
			if (IsValid(*It) && !It->IsDead() && !It->ActorHasTag(MinibossTag))
			{
				Dummies.Add(*It);
			}
		}

		return Dummies;
	}

	/** Watch of the ability the Miniboss (respawned by a wipe) fires. */
	int32 WatchMiniboss(FGeoTutorialBot& Bot, FName const AbilityTag)
	{
		return Bot.WatchActivations(
			[&Bot]() -> AActor*
			{
				return FindMiniboss(Bot);
			},
			AbilityTag);
	}

	/** A stage that ends when the pawn lives again, back on Room's entrance. */
	void AddRespawnStage(FGeoTutorialBot& Bot, FString Name, int32 const Room)
	{
		Bot.AddStage(MoveTemp(Name),
					 [&Bot, Room](bool)
					 {
						 APlayableCharacter const* const Pawn = Bot.GetPawn();
						 AActor const* const Entrance =
							 Bot.FindByLabel(FString::Printf(TEXT("Tut_%s_Entrance"), *GeoTutorial::RoomName(Room)));
						 return Pawn && !Pawn->IsDead() && Entrance
							 && Distance2D(Pawn, FVector2D(Entrance->GetActorLocation())) < RespawnReach;
					 });
	}

	/** The dummies carrying Tag are all dead. */
	void KillAll(FGeoTutorialBot const& Bot, FName const Tag)
	{
		for (AEnemyCharacter* const Dummy : Bot.FindLiving(Tag))
		{
			Dummy->Death();
		}
	}

	// -----------------------------------------------------------------------------------------------------------------
	// Landing
	// -----------------------------------------------------------------------------------------------------------------
	void AddLandingStages(FGeoTutorialBot& Bot)
	{
		TSharedRef<bool> SawLock = MakeShared<bool>(false);
		TSharedRef<bool> SawCat = MakeShared<bool>(false);
		Bot.AddStage(
			TEXT("Landing: movement locked while the Cat talks, then freed"),
			[&Bot, SawLock, SawCat](bool)
			{
				UGeoEncounterSubsystem const* const Encounters = Bot.GetWorld()->GetSubsystem<UGeoEncounterSubsystem>();
				AActor const* const Cat = Bot.FindByLabel(TEXT("Tut_Landing_Cat"));
				*SawLock |= Encounters->bMovementLocked;
				*SawCat |= Cat && !Cat->IsHidden();
				bool const bPadOpen = Bot.IsPadOpen(ExitPad(GeoTutorial::Landing));
				if (bPadOpen)
				{
					Bot.Test.TestTrue(TEXT("Landing locked the movement"), *SawLock);
					Bot.Test.TestFalse(TEXT("Landing freed the movement before the pad"), Encounters->bMovementLocked);
					Bot.Test.TestTrue(TEXT("The walk line was said"), Bot.SawLine(LandingWalkLine));
				}

				return bPadOpen;
			});
		Bot.AddStage(TEXT("Landing: the Cat was shown, and hides after its portal jump"),
					 [&Bot, SawCat](bool)
					 {
						 AActor const* const Cat = Bot.FindByLabel(TEXT("Tut_Landing_Cat"));
						 *SawCat |= Cat && !Cat->IsHidden();
						 bool const bHidden = Cat && Cat->IsHidden();
						 if (bHidden)
						 {
							 Bot.Test.TestTrue(TEXT("The Cat was shown before it hid"), *SawCat);
						 }

						 return bHidden;
					 });
	}

	void WatchExitPadsOpenWithLastLine(FGeoTutorialBot& Bot)
	{
		struct FClosingLine
		{
			int32 Room;
			TCHAR const* Line;
		};
		// Landing's is Path.EarlyPortal's: its pad may open in the run before the FullRun restart, with no HUD.
		static FClosingLine const ClosingLines[] = {
			{GeoTutorial::Dash, DashUselessLine},	  {GeoTutorial::Shoot, ShootNextToyLine},
			{GeoTutorial::Deploy, DeployFullLine},	  {GeoTutorial::Special, SpecialDoneLine},
			{GeoTutorial::Dummies, DummiesDoneLine},  {GeoTutorial::Zones, ZonesSurvivedLine},
			{GeoTutorial::Miniboss, MinibossKillLine}};
		TSharedRef<TSet<int32>> Opened = MakeShared<TSet<int32>>();
		Bot.Observers.Add(
			[&Bot, Opened]()
			{
				for (FClosingLine const& Closing : ClosingLines)
				{
					if (!Opened->Contains(Closing.Room) && Bot.IsPadOpen(ExitPad(Closing.Room)))
					{
						Opened->Add(Closing.Room);
						Bot.Test.TestTrue(
							*FString::Printf(TEXT("Room %d: the exit opens with '%s'"), Closing.Room, Closing.Line),
							!Bot.Lines.IsEmpty() && Bot.Lines.Last().Contains(Closing.Line));
					}
				}
			});
	}

	void AddLandingNagStages(FGeoTutorialBot& Bot)
	{
		Bot.AddExitPadStage(GeoTutorial::Landing);
		Bot.AddLineStage(TEXT("Landing: idle on the open pad, the nag"), LandingNagLine);
		FGeoCatLineCategories Puns;
		Puns.Names = {"Pun"};
		Bot.AddFileLineStage(TEXT("Landing: then a Pun joke"), Puns);
	}

	void AddEarlyPortalStages(FGeoTutorialBot& Bot)
	{
		TSharedRef<int32> PortalLine = MakeShared<int32>(INDEX_NONE);
		Bot.KeepLineFragment = LandingPortalLine;
		Bot.AddStage(
			TEXT("Landing: the pad opens with the last line, which stays"),
			[&Bot, PortalLine](bool)
			{
				*PortalLine = Bot.FindLine(LandingPortalLine);
				return *PortalLine != INDEX_NONE && Bot.IsPadOpen(ExitPad(GeoTutorial::Landing));
			},
			150.);
		// The kept line never ends by itself here: only the arrival, while the Landing step still runs, can start Dash.
		Bot.AddTeleportStages(GeoTutorial::Landing);
		Bot.AddLineStage(TEXT("Landing: the Dash room starts at once, the Landing line still standing"),
						 DashArrivalLine);
		Bot.AddStage(TEXT("Landing: the arrival stopped the Landing Cat"),
					 [&Bot](bool)
					 {
						 return Bot.FindByLabel(TEXT("Tut_Landing_Cat"))->IsHidden();
					 });
		Bot.AddWaitStage(TEXT("Landing: a second for a late Landing line"), 1.);
		Bot.AddStage(TEXT("Landing: no Landing line after the pad"),
					 [&Bot, PortalLine](bool)
					 {
						 for (TCHAR const* const Fragment : LandingLines)
						 {
							 Bot.Test.TestEqual(*FString::Printf(TEXT("'%s' not said after the pad"), Fragment),
												Bot.FindLine(Fragment, *PortalLine + 1), INDEX_NONE);
						 }

						 return true;
					 });
	}

	// -----------------------------------------------------------------------------------------------------------------
	// Dash
	// -----------------------------------------------------------------------------------------------------------------
	/** The pawn dashes over the void with the real ability and lands on the far platform; the exit pad opens. */
	void AddRealDashStages(FGeoTutorialBot& Bot)
	{
		float const CentreX = GeoTutorial::RoomCentreX(GeoTutorial::Dash);
		TSharedRef<int32> Frame = MakeShared<int32>(0);
		Bot.AddStage(TEXT("Dash: the real dash crosses the void"),
					 [&Bot, CentreX, Frame](bool const bFirstTick)
					 {
						 if (bFirstTick)
						 {
							 *Frame = 0;
							 Bot.SetInvincible(true);
							 Bot.MovePawn(FVector2D(CentreX - DashStartInset, 0.f));
							 Bot.GetPawn()->SetActorRotation(FRotator::ZeroRotator);
						 }

						 if (++*Frame == 5)
						 {
							 Bot.ActivatePlayerAbility(DashAbility);
						 }

						 bool const bPadOpen = Bot.IsPadOpen(ExitPad(GeoTutorial::Dash));
						 if (bPadOpen)
						 {
							 Bot.Test.TestTrue(TEXT("The dash landed on the far platform"),
											   Bot.GetPawn()->GetActorLocation().X > CentreX + DashFarFloorX);
						 }

						 return bPadOpen;
					 });
	}

	void AddDashStages(FGeoTutorialBot& Bot, int32 const Falls)
	{
		float const CentreX = GeoTutorial::RoomCentreX(GeoTutorial::Dash);
		Bot.AddStage(TEXT("Dash: arrival unlocks the dash"),
					 [&Bot](bool)
					 {
						 return Bot.IsGranted(DashAbility);
					 });
		Bot.AddLineStage(TEXT("Dash: the arrival line"), DashArrivalLine);
		for (int32 Fall = 1; Fall <= Falls; ++Fall)
		{
			Bot.AddStage(FString::Printf(TEXT("Dash: fall %d over the void"), Fall),
						 [&Bot, CentreX, Fall](bool const bFirstTick)
						 {
							 if (bFirstTick)
							 {
								 Bot.SetInvincible(false);
								 Bot.MovePawn(FVector2D(CentreX + 50.f, 0.f));
							 }

							 return Bot.GetCounter(GeoTutorial::Dash, "Falls") >= Fall;
						 });
			AddRespawnStage(Bot, FString::Printf(TEXT("Dash: fall %d respawns on the entrance"), Fall),
							GeoTutorial::Dash);
			if (Fall == 1)
			{
				for (TCHAR const* const Fragment : DashFirstFallLines)
				{
					Bot.AddLineStage(TEXT("Dash: the first fall lines"), Fragment);
				}
			}
			else
			{
				Bot.AddLineStage(FString::Printf(TEXT("Dash: the line of fall %d"), Fall),
								 DashFallLines[FMath::Min(Fall, int32(UE_ARRAY_COUNT(DashFallLines))) - 1]);
			}
		}

		AddRealDashStages(Bot);
		if (Falls == 0)
		{
			Bot.AddLineStage(TEXT("Dash: the stunned line"), DashStunnedLine);
		}
		else
		{
			Bot.AddLineStage(TEXT("Dash: the relief line"), DashReliefLine);
			Bot.AddStage(TEXT("Dash: the relief line counts the falls"),
						 [&Bot, Falls](bool)
						 {
							 Bot.Test.TestTrue(TEXT("The relief line says the falls"),
											   Bot.SawLine(FString::Printf(TEXT("Only %d deaths"), Falls)));
							 return true;
						 });
		}

		Bot.AddLineStage(TEXT("Dash: the closing line"), DashUselessLine);
		Bot.AddStage(TEXT("Dash: the other branch's line was not said"),
					 [&Bot, Falls](bool)
					 {
						 Bot.ExpectNoLine(Falls == 0 ? DashReliefLine : DashStunnedLine);
						 return true;
					 });
	}

	void AddDashIdleStages(FGeoTutorialBot& Bot)
	{
		Bot.AddStage(TEXT("Dash: arrival unlocks the dash"),
					 [&Bot](bool)
					 {
						 return Bot.IsGranted(DashAbility);
					 });
		for (TCHAR const* const Fragment : DashIdleLines)
		{
			Bot.AddLineStage(TEXT("Dash: an idle tier"), Fragment);
		}

		FGeoCatLineCategories Puns;
		Puns.Names = {"Pun"};
		Bot.AddFileLineStage(TEXT("Dash: past the tiers, a Pun joke"), Puns);
		Bot.AddFileLineStage(TEXT("Dash: and another one"), Puns);
	}

	// -----------------------------------------------------------------------------------------------------------------
	// Shoot
	// -----------------------------------------------------------------------------------------------------------------
	/** The first dummy dies and the quest brings nine. */
	void AddShootOpeningStages(FGeoTutorialBot& Bot)
	{
		Bot.AddStage(TEXT("Shoot: the basic attack is unlocked"),
					 [&Bot](bool)
					 {
						 return Bot.IsGranted(BasicBeamAbility);
					 });
		Bot.AddSpawnStage(GeoTutorial::Shoot, ShootDummyTag, 1);
		Bot.AddStage(TEXT("Shoot: the first dummy dies"),
					 [&Bot](bool)
					 {
						 KillAll(Bot, ShootDummyTag);
						 return Bot.FindLiving(ShootDummyTag).IsEmpty();
					 });
		Bot.AddSpawnStage(GeoTutorial::Shoot, ShootDummyTag, QuestDummies);
	}

	/** Kills one quest dummy and waits for its replacement on the spot where it died. */
	void AddRespawnWhereItDiedStage(FGeoTutorialBot& Bot, TSharedRef<int32> Kills)
	{
		TSharedRef<FVector2D> Spot = MakeShared<FVector2D>(FVector2D::ZeroVector);
		Bot.AddStage(TEXT("Shoot: a killed quest dummy is replaced where it died"),
					 [&Bot, Kills, Spot](bool const bFirstTick)
					 {
						 if (bFirstTick)
						 {
							 AEnemyCharacter* const Dummy = FindFirstLiving(Bot, ShootDummyTag);
							 *Spot = FVector2D(Dummy->GetActorLocation());
							 Dummy->Death();
							 ++*Kills;
						 }

						 TArray<AEnemyCharacter*> const Living = Bot.FindLiving(ShootDummyTag);
						 return Living.Num() == QuestDummies
							 && Living.ContainsByPredicate(
								 [Spot](AEnemyCharacter const* const Dummy)
								 {
									 return Distance2D(Dummy, *Spot) < 50.f;
								 });
					 });
	}

	void AddShootStages(FGeoTutorialBot& Bot, EGeoShootBranch const Branch)
	{
		TSharedRef<int32> Kills = MakeShared<int32>(0);
		AddShootOpeningStages(Bot);
		if (Branch == EGeoShootBranch::PauseHolds)
		{
			TSharedRef<FVector2D> Spot = MakeShared<FVector2D>(FVector2D::ZeroVector);
			TSharedRef<double> Start = MakeShared<double>(0.);
			Bot.AddStage(TEXT("Shoot: a pause menu holds the respawn and the cut"),
						 [&Bot, Spot, Start](bool const bFirstTick)
						 {
							 if (bFirstTick)
							 {
								 AEnemyCharacter* const Dummy = FindFirstLiving(Bot, ShootDummyTag);
								 *Spot = FVector2D(Dummy->GetActorLocation());
								 Bot.bHoldPauseMenu = true;
								 Bot.GetController()->OpenPauseMenu();
								 Dummy->Death();
								 *Start = FPlatformTime::Seconds();
							 }

							 bool const bHolds = Bot.FindLiving(ShootDummyTag).Num() == QuestDummies - 1
								 && Bot.GetCounter(GeoTutorial::Shoot, "Cut") == 0
								 && !Bot.IsPadOpen(ExitPad(GeoTutorial::Shoot));
							 if (!bHolds)
							 {
								 Bot.Test.AddError(TEXT("The pause menu did not hold the quest's respawn and cut"));
							 }

							 return !bHolds || FPlatformTime::Seconds() - *Start >= PauseHoldSeconds;
						 });
			Bot.AddStage(TEXT("Shoot: closing the menu lets the respawn run"),
						 [&Bot, Spot](bool)
						 {
							 Bot.bHoldPauseMenu = false;
							 TArray<AEnemyCharacter*> const Living = Bot.FindLiving(ShootDummyTag);
							 return Living.Num() == QuestDummies
								 && Living.ContainsByPredicate(
									 [Spot](AEnemyCharacter const* const Dummy)
									 {
										 return Distance2D(Dummy, *Spot) < 50.f;
									 });
						 });
		}
		else if (Branch == EGeoShootBranch::Cut)
		{
			Bot.AddExitPadStage(GeoTutorial::Shoot);
			Bot.AddLineStage(TEXT("Shoot: the Cat gives up on the 45 s cut"), ShootStopLine);
			Bot.AddLineStage(TEXT("Shoot: the quest is abandoned"), ShootAbandonedLine);
			Bot.AddStage(TEXT("Shoot: no kill, no quip, no dummy left"),
						 [&Bot](bool)
						 {
							 for (TCHAR const* const Quip : ShootQuips)
							 {
								 Bot.ExpectNoLine(Quip);
							 }

							 return Bot.FindLiving(ShootDummyTag).IsEmpty();
						 });
		}
		else
		{
			AddRespawnWhereItDiedStage(Bot, Kills);
			Bot.AddStage(TEXT("Shoot: twelve kills end the quest"),
						 [&Bot, Kills](bool)
						 {
							 TArray<AEnemyCharacter*> const Living = Bot.FindLiving(ShootDummyTag);
							 if (*Kills < QuestKills && !Living.IsEmpty())
							 {
								 Living[0]->Death();
								 ++*Kills;
							 }

							 return Bot.IsPadOpen(ExitPad(GeoTutorial::Shoot));
						 });
			Bot.AddLineStage(TEXT("Shoot: the quest line"), ShootQuestLine);
			for (TCHAR const* const Quip : ShootQuips)
			{
				Bot.AddLineStage(TEXT("Shoot: a quip at 3 / 6 / 9 kills"), Quip);
			}

			Bot.AddLineStage(TEXT("Shoot: the twelfth kill ends it"), ShootStopLine);
			Bot.AddLineStage(TEXT("Shoot: the exit lines"), ShootAbandonedLine);
			Bot.AddLineStage(TEXT("Shoot: the exit lines"), ShootNextToyLine);
			Bot.AddStage(TEXT("Shoot: leaving the quest removes every dummy"),
						 [&Bot, Kills](bool)
						 {
							 Bot.Test.TestEqual(TEXT("Quest kills"), *Kills, QuestKills);
							 return Bot.FindLiving(ShootDummyTag).IsEmpty();
						 });
		}
	}

	// -----------------------------------------------------------------------------------------------------------------
	// Deploy
	// -----------------------------------------------------------------------------------------------------------------
	void AddDeployStages(FGeoTutorialBot& Bot)
	{
		Bot.AddStage(TEXT("Deploy: the Healing Zone is unlocked"),
					 [&Bot](bool)
					 {
						 return Bot.IsGranted(HealingZoneAbility);
					 });
		Bot.AddSpawnStage(GeoTutorial::Deploy, AllyTag, 1);
		TSharedRef<TWeakObjectPtr<AGeoDeployableBase>> Zone = MakeShared<TWeakObjectPtr<AGeoDeployableBase>>();
		Bot.AddStage(TEXT("Deploy: the ally starts hurt, real Healing Zones heal it"),
					 [&Bot, Zone](bool const bFirstTick)
					 {
						 AEnemyCharacter* const Ally = FindFirstLiving(Bot, AllyTag);
						 if (bFirstTick)
						 {
							 Bot.Test.TestTrue(TEXT("Deploy ally spawns hurt"),
											   FGeoTutorialBot::HealthRatio(Ally) < HurtAllyMaxRatio);
						 }

						 bool const bZoneNeeded = Ally && FGeoTutorialBot::HealthRatio(Ally) < 1.f
							 && (!Zone->IsValid() || !(*Zone)->IsActive());
						 if (bZoneNeeded)
						 {
							 *Zone = GeoASLib::SpawnDeployableFromAbility(*GeoASLib::GetGeoAscFromActor(Bot.GetPawn()),
																		  Ally->GetActorLocation());
							 Bot.Test.TestTrue(TEXT("The Healing Zone spawns"), Zone->IsValid());
						 }

						 return Bot.IsPadOpen(ExitPad(GeoTutorial::Deploy));
					 });
		Bot.AddLineStage(TEXT("Deploy: the full health line"), DeployFullLine);
	}

	// -----------------------------------------------------------------------------------------------------------------
	// Special
	// -----------------------------------------------------------------------------------------------------------------
	/** Both dummies come back hurt and alive for a new round, the exit still closed. */
	void AddSpecialRoundSpawnStage(FGeoTutorialBot& Bot, int32 const Round)
	{
		Bot.AddStage(FString::Printf(TEXT("Special round %d: a hurt ally and a rude dummy"), Round),
					 [&Bot, Round](bool)
					 {
						 TArray<AEnemyCharacter*> const Allies = Bot.FindLiving(SpecialAllyTag);
						 bool const bSpawned = Allies.Num() == 1 && Bot.FindLiving(SpecialEnemyTag).Num() == 1
							 && FGeoTutorialBot::HealthRatio(Allies[0]) < HurtAllyMaxRatio;
						 if (bSpawned)
						 {
							 Bot.Test.TestFalse(*FString::Printf(TEXT("Round %d starts with the exit closed"), Round),
												Bot.IsPadOpen(ExitPad(GeoTutorial::Special)));
						 }

						 return bSpawned;
					 });
	}

	/** Kills the rude dummy and heals the ally: both goals of the round. The beam cannot be aimed headless, so the heal
	 * is set. */
	void CompleteSpecialGoals(FGeoTutorialBot const& Bot)
	{
		FindFirstLiving(Bot, SpecialEnemyTag)->Death();
		FGeoTutorialBot::SetHealthRatio(FindFirstLiving(Bot, SpecialAllyTag), 1.f);
	}

	/** Round Round: both goals, with the Special used first when bWithSpecial. */
	void AddSpecialRoundStage(FGeoTutorialBot& Bot, int32 const Round, bool const bWithSpecial)
	{
		Bot.AddStage(FString::Printf(TEXT("Special round %d: both goals %s the Special"), Round,
									 bWithSpecial ? TEXT("with") : TEXT("without")),
					 [&Bot, bWithSpecial](bool)
					 {
						 if (bWithSpecial)
						 {
							 Bot.ActivatePlayerAbility(SpecialAbility);
						 }

						 CompleteSpecialGoals(Bot);
						 return true;
					 });
	}

	/** The Special's step ends: the Cat's closing line, the pad. */
	void AddSpecialDoneStages(FGeoTutorialBot& Bot)
	{
		Bot.AddExitPadStage(GeoTutorial::Special);
		Bot.AddLineStage(TEXT("Special: done"), SpecialDoneLine);
		Bot.AddStage(TEXT("Special: leaving the step removes the dummies"),
					 [&Bot](bool)
					 {
						 return Bot.FindLiving(SpecialAllyTag).IsEmpty() && Bot.FindLiving(SpecialEnemyTag).IsEmpty();
					 });
	}

	void AddSpecialOrderStages(FGeoTutorialBot& Bot)
	{
		TSharedRef<int32> Frame = MakeShared<int32>(0);
		Bot.AddStage(TEXT("Special order: the Special first"),
					 [&Bot](bool const bFirstTick)
					 {
						 if (bFirstTick)
						 {
							 Bot.ActivatePlayerAbility(SpecialAbility);
						 }

						 return Bot.GetCounter(GeoTutorial::Special, "Progress") >= 4;
					 });
		Bot.AddStage(TEXT("Special order: the ally full"),
					 [&Bot](bool const bFirstTick)
					 {
						 if (bFirstTick)
						 {
							 FGeoTutorialBot::SetHealthRatio(FindFirstLiving(Bot, SpecialAllyTag), 1.f);
						 }

						 return Bot.GetCounter(GeoTutorial::Special, "Progress") == 5;
					 });
		Bot.AddStage(TEXT("Special order: the ally dips and is healed again, counted once"),
					 [&Bot, Frame](bool const bFirstTick)
					 {
						 if (bFirstTick)
						 {
							 *Frame = 0;
						 }

						 AEnemyCharacter* const Ally = FindFirstLiving(Bot, SpecialAllyTag);
						 *Frame += 1;
						 if (*Frame == 1)
						 {
							 FGeoTutorialBot::SetHealthRatio(Ally, 0.5f);
						 }
						 else if (*Frame == 5)
						 {
							 FGeoTutorialBot::SetHealthRatio(Ally, 1.f);
						 }

						 bool const bDone = *Frame >= 12;
						 if (bDone)
						 {
							 Bot.Test.TestEqual(TEXT("The ally healed twice counts once"),
												Bot.GetCounter(GeoTutorial::Special, "Progress"), 5);
						 }

						 return bDone;
					 });
		Bot.AddStage(TEXT("Special order: then the kill"),
					 [&Bot](bool const bFirstTick)
					 {
						 if (bFirstTick)
						 {
							 FindFirstLiving(Bot, SpecialEnemyTag)->Death();
						 }

						 return Bot.IsPadOpen(ExitPad(GeoTutorial::Special));
					 });
		Bot.AddLineStage(TEXT("Special: done"), SpecialDoneLine);
	}

	void AddSpecialStages(FGeoTutorialBot& Bot, EGeoSpecialBranch const Branch)
	{
		Bot.AddStage(TEXT("Special: the ability is unlocked"),
					 [&Bot](bool)
					 {
						 return Bot.IsGranted(SpecialAbility);
					 });
		int32 WithoutSpecialRounds = 0;
		if (Branch == EGeoSpecialBranch::TwoStings)
		{
			WithoutSpecialRounds = 2;
		}
		else if (Branch == EGeoSpecialBranch::Surrender)
		{
			WithoutSpecialRounds = 3;
		}

		for (int32 Round = 1; Round <= WithoutSpecialRounds; ++Round)
		{
			AddSpecialRoundSpawnStage(Bot, Round);
			AddSpecialRoundStage(Bot, Round, false);
			if (Round <= int32(UE_ARRAY_COUNT(SpecialStings)))
			{
				Bot.AddLineStage(FString::Printf(TEXT("Special round %d: the sting"), Round), SpecialStings[Round - 1]);
			}
		}

		if (Branch == EGeoSpecialBranch::Surrender)
		{
			Bot.AddLineStage(TEXT("Special: the surrender"), SpecialSurrenderLine);
			Bot.AddExitPadStage(GeoTutorial::Special);
			Bot.AddStage(TEXT("Special: no done line"),
						 [&Bot](bool)
						 {
							 Bot.ExpectNoLine(SpecialDoneLine);
							 return true;
						 });
		}
		else if (Branch == EGeoSpecialBranch::Order)
		{
			AddSpecialRoundSpawnStage(Bot, 1);
			AddSpecialOrderStages(Bot);
		}
		else
		{
			AddSpecialRoundSpawnStage(Bot, WithoutSpecialRounds + 1);
			AddSpecialRoundStage(Bot, WithoutSpecialRounds + 1, true);
			AddSpecialDoneStages(Bot);
			Bot.AddStage(TEXT("Special: no surrender"),
						 [&Bot](bool)
						 {
							 Bot.ExpectNoLine(SpecialSurrenderLine);
							 return true;
						 });
			if (Branch == EGeoSpecialBranch::FirstTry)
			{
				Bot.AddWaitStage(TEXT("Special: a second for a second round that must not come"), 1.);
				Bot.AddStage(TEXT("Special: round 2 never spawns"),
							 [&Bot](bool)
							 {
								 Bot.Test.TestTrue(TEXT("No dummy of a second round"),
												   Bot.FindLiving(SpecialAllyTag).IsEmpty()
													   && Bot.FindLiving(SpecialEnemyTag).IsEmpty());
								 Bot.ExpectNoLine(SpecialStings[0]);
								 return true;
							 });
			}
		}
	}

	// -----------------------------------------------------------------------------------------------------------------
	// Dummies
	// -----------------------------------------------------------------------------------------------------------------
	void AddDummiesBehaviourStages(FGeoTutorialBot& Bot)
	{
		float const CentreX = GeoTutorial::RoomCentreX(GeoTutorial::Dummies);
		static constexpr float DummyX = 300.f;
		static constexpr float PointBlank = 60.f;
		static constexpr double WakeSeconds = 0.5;
		static constexpr float DummyY[] = {-600.f, 0.f, 600.f};
		int32 const ShooterWatch = Bot.WatchActivations(
			[&Bot]() -> AActor*
			{
				return FindFirstLiving(Bot, ShooterTag);
			},
			ShooterAbility);
		TSharedRef<double> Start = MakeShared<double>(0.);
		TSharedRef<float> ChaserDistance = MakeShared<float>(0.f);
		Bot.AddStage(TEXT("Dummies: each dummy wakes at point-blank"),
					 [&Bot, CentreX, Start](bool const bFirstTick)
					 {
						 if (bFirstTick)
						 {
							 *Start = FPlatformTime::Seconds();
						 }

						 double const Elapsed = FPlatformTime::Seconds() - *Start;
						 int32 const Index =
							 FMath::Min(int32(Elapsed / WakeSeconds), int32(UE_ARRAY_COUNT(DummyY)) - 1);
						 Bot.MovePawn(FVector2D(CentreX + DummyX - PointBlank, DummyY[Index]));
						 return Elapsed >= WakeSeconds * UE_ARRAY_COUNT(DummyY);
					 });
		Bot.AddStage(TEXT("Dummies: the chaser closes in"),
					 [&Bot, CentreX, ChaserDistance](bool const bFirstTick)
					 {
						 AEnemyCharacter const* const Chaser = FindFirstLiving(Bot, ChaserTag);
						 if (bFirstTick)
						 {
							 Bot.MovePawn(FVector2D(CentreX - 500.f, 0.f));
							 *ChaserDistance = Distance2D(Chaser, FVector2D(CentreX - 500.f, 0.f));
						 }

						 return Distance2D(Chaser, FVector2D(Bot.GetPawn()->GetActorLocation()))
							 < *ChaserDistance - 150.f;
					 });
		Bot.AddStage(TEXT("Dummies: the shooter fires"),
					 [&Bot, ShooterWatch](bool)
					 {
						 return Bot.GetWatchCount(ShooterWatch) >= 1
							 || FGeoTutorialBot::IsAbilityActive(FindFirstLiving(Bot, ShooterTag), ShooterAbility);
					 });
	}

	void AddDummiesStages(FGeoTutorialBot& Bot, bool const bDieMidFight)
	{
		TArray<FName> const Dummies = {ChaserTag, ShooterTag, MoverTag};
		for (FName const Tag : Dummies)
		{
			Bot.AddSpawnStage(GeoTutorial::Dummies, Tag, 1);
		}

		if (bDieMidFight)
		{
			Bot.AddStage(TEXT("Dummies: the player dies with the three alive"),
						 [&Bot](bool)
						 {
							 Bot.KillPlayer();
							 return true;
						 });
			AddRespawnStage(Bot, TEXT("Dummies: the player respawns on the entrance"), GeoTutorial::Dummies);
			Bot.AddStage(TEXT("Dummies: the three are still there, the fight goes on"),
						 [&Bot, Dummies](bool)
						 {
							 for (FName const Tag : Dummies)
							 {
								 Bot.Test.TestEqual(*FString::Printf(TEXT("%s survives the death"), *Tag.ToString()),
													Bot.FindLiving(Tag).Num(), 1);
							 }

							 return true;
						 });
		}
		else
		{
			AddDummiesBehaviourStages(Bot);
		}

		Bot.AddStage(TEXT("Dummies: killing the three ends the room"),
					 [&Bot, Dummies](bool)
					 {
						 for (FName const Tag : Dummies)
						 {
							 KillAll(Bot, Tag);
						 }

						 return Bot.IsPadOpen(ExitPad(GeoTutorial::Dummies));
					 });
		Bot.AddLineStage(TEXT("Dummies: the closing line"), DummiesDoneLine);
	}

	// -----------------------------------------------------------------------------------------------------------------
	// Zones
	// -----------------------------------------------------------------------------------------------------------------
	/** Zone points, as the user placed them: the Damage zone grows on its point, Shield and Heal appear on theirs. */
	FVector2D GetZonePoint(FGeoTutorialBot const& Bot, FString const& Color)
	{
		AActor const* const Point = Bot.FindByLabel(TEXT("Tut_Zones_ZonePoint_") + Color);
		return Bot.Test.TestNotNull(*FString::Printf(TEXT("%s zone point"), *Color), Point)
			? FVector2D(Point->GetActorLocation())
			: FVector2D::ZeroVector;
	}

	/** The colours step: the Damage zone grows past ColorZoneMaxRadius's start and hurts the pawn, Shield and Heal
	 * appear after the growth, the pawn stands in each, the step runs on right after the second, and 2 s later every
	 * zone is gone. */
	void AddZoneColorsStages(FGeoTutorialBot& Bot)
	{
		TSharedRef<float> FirstRadius = MakeShared<float>(0.f);
		TSharedRef<bool> bHurt = MakeShared<bool>(false);
		Bot.AddStage(TEXT("Zones: the Damage zone grows over the room and hurts the pawn"),
					 [&Bot, FirstRadius, bHurt](bool const bFirstTick)
					 {
						 FVector2D const Spot = GetZonePoint(Bot, TEXT("Damage"));
						 AGeoEffectZone const* const Zone = Bot.FindZoneAt(Spot);
						 if (bFirstTick)
						 {
							 Bot.SetInvincible(false);
							 GeoASLib::GetGeoAscFromActor(Bot.GetPawn())
								 ->GetGameplayAttributeValueChangeDelegate(UGeoAttributeSetBase::GetHealthAttribute())
								 .AddLambda(
									 [bHurt](FOnAttributeChangeData const& Data)
									 {
										 *bHurt |= Data.NewValue < Data.OldValue;
									 });
						 }

						 bool bDone = false;
						 if (Zone)
						 {
							 if (*FirstRadius == 0.f)
							 {
								 *FirstRadius = Zone->GetSimpleCollisionRadius();
								 Bot.MovePawn(Spot);
							 }

							 bDone = Zone->GetSimpleCollisionRadius() >= ColorZoneMaxRadius - 1.f;
							 if (bDone)
							 {
								 Bot.Test.TestTrue(TEXT("The Damage zone grew"),
												   Zone->GetSimpleCollisionRadius() > *FirstRadius);
								 Bot.Test.TestTrue(TEXT("The Damage zone hurt the pawn"), *bHurt);
								 Bot.SetInvincible(true);
							 }
						 }

						 return bDone;
					 });
		for (FString const Color : {TEXT("Shield"), TEXT("Heal")})
		{
			TSharedRef<bool> bMoved = MakeShared<bool>(false);
			Bot.AddStage(FString::Printf(TEXT("Zones: the %s zone appears and the pawn stands in it"), *Color),
						 [&Bot, Color, bMoved](bool)
						 {
							 FVector2D const Spot = GetZonePoint(Bot, Color);
							 if (!*bMoved && Bot.FindZoneAt(Spot))
							 {
								 *bMoved = true;
								 Bot.MovePawn(Spot);
							 }

							 return *bMoved;
						 });
		}

		Bot.AddStage(TEXT("Zones: every zone is still up after the last entry, gone with the step"),
					 [&Bot](bool const bFirstTick)
					 {
						 bool const bAnyUp = Bot.FindZoneAt(GetZonePoint(Bot, TEXT("Damage")))
							 || Bot.FindZoneAt(GetZonePoint(Bot, TEXT("Shield")))
							 || Bot.FindZoneAt(GetZonePoint(Bot, TEXT("Heal")));
						 if (bFirstTick)
						 {
							 Bot.Test.TestTrue(TEXT("The step runs on after the second entry"), bAnyUp);
						 }

						 return !bAnyUp;
					 });
	}


	/** The pawn stands in a lane of each wave of Series (or outside them all on its death wave): a lane that does not
	 * shield, or a death elsewhere, fails the run. */
	void AddSafeSeriesStage(FGeoTutorialBot& Bot, int32 const Watch, FSafeSeries const& Series, int32 const Index,
							TSharedRef<int32> LethalWaveCount)
	{
		float const CentreX = GeoTutorial::RoomCentreX(GeoTutorial::Zones);
		TSharedRef<int32> Base = MakeShared<int32>(0);
		TSharedRef<bool> bAlive = MakeShared<bool>(false);
		Bot.AddStage(FString::Printf(TEXT("Zones: safe-spot run %d"), Index + 1),
					 [&Bot, Watch, Series, Index, CentreX, Base, bAlive, LethalWaveCount](bool const bFirstTick)
					 {
						 if (bFirstTick)
						 {
							 *Base = Index == 0 ? *LethalWaveCount : Bot.GetWatchCount(Watch);
							 *bAlive = false;
						 }

						 int32 const Fired = Bot.GetWatchCount(Watch) - *Base;
						 APlayableCharacter const* const Pawn = Bot.GetPawn();
						 bool bDone = false;
						 if (Pawn && !Pawn->IsDead())
						 {
							 if (!*bAlive)
							 {
								 // Back from a death: waves of the run before still fired while it waited for the
								 // respawn.
								 *Base = Bot.GetWatchCount(Watch);
							 }

							 *bAlive = true;
							 int32 const Wave = FMath::Clamp(Bot.GetWatchCount(Watch) - *Base, 1, Series.LaneY.Num());
							 FVector2D const Lane(CentreX, Series.LaneY[Wave - 1]);
							 if (FVector2D::Distance(FVector2D(Pawn->GetActorLocation()), Lane) > 1.f)
							 {
								 Bot.MovePawn(Lane);
							 }
						 }
						 else if (Pawn && *bAlive)
						 {
							 *bAlive = false;
							 bDone = true;
							 Bot.Test.TestEqual(TEXT("The wave the pawn died on"), Fired, Series.DeathWave);
						 }

						 if (Series.DeathWave == 0 && Bot.IsPadOpen(ExitPad(GeoTutorial::Zones)))
						 {
							 bDone = true;
							 Bot.Test.TestEqual(TEXT("Safe-spot waves fired"), Fired, Series.LaneY.Num());
						 }

						 return bDone;
					 });
	}

	void AddZonesStages(FGeoTutorialBot& Bot, bool const bAllSeries)
	{
		float const CentreX = GeoTutorial::RoomCentreX(GeoTutorial::Zones);
		TArray<FSafeSeries> const AllSeries = GetSafeSeries(bAllSeries);
		int32 const Caster = Bot.WatchActivations(
			// Spawns the shared caster on the bot's first tick, so the watch is bound before the first wave fires.
			[&Bot]() -> AActor*
			{
				UWorld* const World = Bot.GetWorld();
				return World ? World->GetSubsystem<UGeoEncounterSubsystem>()->GetSharedCaster() : nullptr;
			},
			LateralWaveAbility);
		Bot.AddLineStage(TEXT("Zones: the first line"), ZonesIntroLine);
		AddZoneColorsStages(Bot);
		Bot.AddLineStage(TEXT("Zones: colors learned"), ZonesLearnedLine);
		TSharedRef<int32> LethalWaveCount = MakeShared<int32>(0);
		Bot.AddStage(TEXT("Zones: the lethal wave kills the pawn"),
					 [&Bot, CentreX, Caster, LethalWaveCount](bool const bFirstTick)
					 {
						 if (bFirstTick)
						 {
							 Bot.SetInvincible(false);
							 Bot.MovePawn(FVector2D(CentreX, 0.f));
						 }

						 bool const bDead = Bot.GetPawn()->IsDead();
						 if (bDead)
						 {
							 *LethalWaveCount = Bot.GetWatchCount(Caster);
						 }

						 return bDead;
					 });
		Bot.AddLineStage(TEXT("Zones: the lethal wave's warning"), ZonesLethalLine);
		Bot.AddLineStage(TEXT("Zones: after the death"), ZonesLethalDeathLine);
		for (int32 Index = 0; Index < AllSeries.Num(); ++Index)
		{
			AddSafeSeriesStage(Bot, Caster, AllSeries[Index], Index, LethalWaveCount);
		}

		int32 const Deaths = AllSeries.Num() - 1;
		Bot.AddStage(TEXT("Zones: the retry line and the survived line"),
					 [&Bot, Deaths](bool)
					 {
						 Bot.SetInvincible(true);
						 Bot.Test.TestEqual(TEXT("Retry lines"), Bot.CountLines(ZonesRetryLine), Deaths);
						 Bot.Test.TestTrue(TEXT("The survived line"), Bot.SawLine(ZonesSurvivedLine));
						 return true;
					 });
	}

	// -----------------------------------------------------------------------------------------------------------------
	// Miniboss
	// -----------------------------------------------------------------------------------------------------------------
	/** The boss and the fight running: what every Miniboss branch starts from. */
	void AddMinibossFightStages(FGeoTutorialBot& Bot)
	{
		Bot.AddLineStage(TEXT("Miniboss: the intro line"), MinibossIntroLine);
		Bot.AddStage(TEXT("Miniboss: the boss stands and fires the salve"),
					 [&Bot](bool)
					 {
						 AEnemyCharacter* const Boss = FindMiniboss(Bot);
						 return Boss && FGeoTutorialBot::IsAbilityActive(Boss, SalveAbility);
					 });
	}

	void AddMinibossStages(FGeoTutorialBot& Bot)
	{
		int32 const WaveWatch = WatchMiniboss(Bot, LateralWaveAbility);
		TSharedRef<float> FirstZoneRadius = MakeShared<float>(0.f);
		TSharedRef<TSet<int32>> PointsReached = MakeShared<TSet<int32>>();
		TSharedRef<TMap<FName, int32>> ChipsBefore = MakeShared<TMap<FName, int32>>();
		TSharedRef<int32> ShardsBefore = MakeShared<int32>(0);
		TSharedRef<int32> DropsThrown = MakeShared<int32>(0);
		Bot.Observers.Add(
			[&Bot, PointsReached]()
			{
				AEnemyCharacter const* const Boss = FindMiniboss(Bot);
				for (int32 Index = 0; Boss && Index < MinibossPointCount; ++Index)
				{
					AActor const* const Point =
						Bot.FindByLabel(FString::Printf(TEXT("Tut_Miniboss_MinibossPoint_%d"), Index));
					if (Point && Distance2D(Boss, FVector2D(Point->GetActorLocation())) < MinibossPointReach)
					{
						PointsReached->Add(Index);
					}
				}
			});
		Bot.AddStage(TEXT("Miniboss: the gem profile before the fight"),
					 [&Bot, ChipsBefore, ShardsBefore, DropsThrown](bool)
					 {
						 UGeoGemProfileSave const* const Profile =
							 UGeoGemSubsystem::GetFirstLocalProfile(Bot.GetWorld());
						 for (TCHAR const* const Chip : MinibossChips)
						 {
							 ChipsBefore->Add(Chip, Profile->GetOwnedCount(Chip));
						 }

						 *ShardsBefore = Profile->GetShards();
						 Bot.GetWorld()->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateLambda(
							 [DropsThrown](AActor* const Actor)
							 {
								 *DropsThrown += Actor->IsA<AGeoGemDrop>() ? 1 : 0;
							 }));
						 return true;
					 });
		AddMinibossFightStages(Bot);
		Bot.AddStage(TEXT("Miniboss: a summon lands and its zone grows"),
					 [&Bot, FirstZoneRadius](bool)
					 {
						 AGeoSummonedDummy const* const Summon = FindLivingSummon(Bot);
						 AGeoEffectZone const* const Zone =
							 Summon ? Bot.FindZoneAt(FVector2D(Summon->GetActorLocation())) : nullptr;
						 float const Radius =
							 Zone ? Zone->FindComponentByClass<UCapsuleComponent>()->GetUnscaledCapsuleRadius() : 0.f;
						 if (Zone && *FirstZoneRadius <= 0.f)
						 {
							 *FirstZoneRadius = Radius;
						 }

						 return Zone && Radius > *FirstZoneRadius + 1.f;
					 });
		Bot.AddStage(TEXT("Miniboss: a killed summon turns ally and fires the salve"),
					 [&Bot](bool const bFirstTick)
					 {
						 if (bFirstTick)
						 {
							 FindLivingSummon(Bot)->Death();
						 }

						 AEnemyCharacter* const Ally = FindFirstLiving(Bot, MinibossAllyTag);
						 return Ally && FGeoTutorialBot::IsAbilityActive(Ally, AllySalveAbility);
					 });
		Bot.AddStage(TEXT("Miniboss: every third summon, the lateral wave"),
					 [&Bot, WaveWatch](bool)
					 {
						 return Bot.GetWatchCount(WaveWatch) >= 1;
					 });
		Bot.AddStage(TEXT("Miniboss: the boss walks between its points"),
					 [PointsReached](bool)
					 {
						 return PointsReached->Num() >= 2;
					 });
		Bot.AddStage(TEXT("Miniboss: its last hit opens the final exit"),
					 [&Bot](bool const bFirstTick)
					 {
						 if (bFirstTick)
						 {
							 FGeoTutorialBot::SetHealthRatio(FindMiniboss(Bot), 0.f);
						 }

						 return Bot.IsPadOpen(ExitPad(GeoTutorial::Miniboss));
					 });
		Bot.AddLineStage(TEXT("Miniboss: the kill line"), MinibossKillLine);
		Bot.AddStage(TEXT("Miniboss: the kill grants one of each chip"),
					 [&Bot, ChipsBefore](bool)
					 {
						 UGeoGemProfileSave const* const Profile =
							 UGeoGemSubsystem::GetFirstLocalProfile(Bot.GetWorld());
						 bool bGranted = true;
						 for (TCHAR const* const Chip : MinibossChips)
						 {
							 bGranted &= Profile->GetOwnedCount(Chip) == ChipsBefore->FindRef(Chip) + 1;
						 }

						 return bGranted;
					 });
		Bot.AddStage(TEXT("Miniboss: the fountain throws one drop per gem and per shard granted"),
					 [&Bot, ShardsBefore, DropsThrown](bool)
					 {
						 int32 const Shards =
							 UGeoGemSubsystem::GetFirstLocalProfile(Bot.GetWorld())->GetShards() - *ShardsBefore;
						 int32 const Expected = UE_ARRAY_COUNT(MinibossChips) + Shards;
						 if (*DropsThrown > Expected)
						 {
							 Bot.Test.AddError(
								 FString::Printf(TEXT("%d drops thrown, expected %d"), *DropsThrown, Expected));
						 }

						 return Shards > 0 && *DropsThrown == Expected;
					 });
	}

	void AddMinibossWipeStages(FGeoTutorialBot& Bot)
	{
		Bot.bKeepPlayerHealthy = true;
		AddMinibossFightStages(Bot);
		Bot.AddStage(TEXT("Miniboss: a summon lands"),
					 [&Bot](bool)
					 {
						 return FindLivingSummon(Bot) != nullptr;
					 });
		for (int32 Death = 1; Death <= int32(UE_ARRAY_COUNT(MinibossTips)); ++Death)
		{
			TSharedRef<int32> Frame = MakeShared<int32>(0);
			Bot.AddStage(FString::Printf(TEXT("Miniboss: death %d wipes the fight"), Death),
						 [&Bot, Frame](bool const bFirstTick)
						 {
							 // The respawn stands at the room's entrance: back into the arena first, then the death.
							 if (bFirstTick)
							 {
								 *Frame = 0;
								 Bot.MovePawn(FVector2D(GeoTutorial::RoomCentreX(GeoTutorial::Miniboss), 0.f));
							 }

							 if (++*Frame == MinibossEnterFrames)
							 {
								 if (AEnemyCharacter* const Boss = FindMiniboss(Bot))
								 {
									 FGeoTutorialBot::SetHealthRatio(Boss, 0.5f);
								 }

								 Bot.KillPlayer();
							 }

							 if (*Frame < MinibossEnterFrames)
							 {
								 return false;
							 }

							 AEnemyCharacter* const Boss = FindMiniboss(Bot);
							 return Boss && FindLivingSummon(Bot) == nullptr
								 && FGeoTutorialBot::HealthRatio(Boss) > 0.99f;
						 });
			Bot.AddStage(FString::Printf(TEXT("Miniboss: death %d, the player is back"), Death),
						 [&Bot](bool)
						 {
							 return !Bot.GetPawn()->IsDead();
						 });
			Bot.AddLineStage(FString::Printf(TEXT("Miniboss: the tip of death %d"), Death), MinibossTips[Death - 1]);
		}

		Bot.AddStage(TEXT("Miniboss: a fourth death"),
					 [&Bot](bool)
					 {
						 Bot.KillPlayer();
						 return true;
					 });
		Bot.AddWaitStage(TEXT("Miniboss: a fourth death says nothing"), 2.);
		Bot.AddStage(TEXT("Miniboss: each tip was said once"),
					 [&Bot](bool)
					 {
						 for (TCHAR const* const Tip : MinibossTips)
						 {
							 Bot.Test.TestEqual(*FString::Printf(TEXT("Tip '%s' said once"), Tip), Bot.CountLines(Tip),
												1);
						 }

						 return true;
					 });
	}

	void AddMinibossCalloutStages(FGeoTutorialBot& Bot)
	{
		Bot.bKeepPlayerHealthy = true;
		AddMinibossFightStages(Bot);
		Bot.AddStage(TEXT("Miniboss: a summon lands, then dies and turns ally"),
					 [&Bot](bool)
					 {
						 AGeoSummonedDummy* const Summon = FindLivingSummon(Bot);
						 if (Summon)
						 {
							 Summon->Death();
						 }

						 return Summon != nullptr;
					 });
		Bot.AddStage(TEXT("Miniboss: the ally stands"),
					 [&Bot](bool)
					 {
						 return FindFirstLiving(Bot, MinibossAllyTag) != nullptr;
					 });
		for (int32 Call = 1; Call <= int32(UE_ARRAY_COUNT(MinibossCallouts)); ++Call)
		{
			Bot.AddStage(FString::Printf(TEXT("Miniboss: the ally under 30 %%, call-out %d"), Call),
						 [&Bot](bool const bFirstTick)
						 {
							 if (bFirstTick)
							 {
								 SetAllyRatio(Bot, 0.2f);
							 }

							 return true;
						 });
			Bot.AddLineStage(FString::Printf(TEXT("Miniboss: the call-out %d line"), Call), MinibossCallouts[Call - 1]);
			Bot.AddStage(TEXT("Miniboss: the ally healed again"),
						 [&Bot](bool)
						 {
							 SetAllyRatio(Bot, 1.f);
							 return true;
						 });
			Bot.AddWaitStage(TEXT("Miniboss: a beat above the threshold"), 0.3);
		}

		Bot.AddStage(TEXT("Miniboss: a fourth time under 30 %"),
					 [&Bot](bool)
					 {
						 SetAllyRatio(Bot, 0.2f);
						 return true;
					 });
		Bot.AddWaitStage(TEXT("Miniboss: the fourth call-out is silent"), 1.5);
		Bot.AddStage(TEXT("Miniboss: each call-out was said once"),
					 [&Bot](bool)
					 {
						 for (TCHAR const* const Call : MinibossCallouts)
						 {
							 Bot.Test.TestEqual(*FString::Printf(TEXT("Call-out '%s' said once"), Call),
												Bot.CountLines(Call), 1);
						 }

						 return true;
					 });
	}

	/** At the frame the next wave fires, puts the pawn at the Y chosen by PlaceY (invincibility off), then waits for
	 * the outcome. bSurvives: the pawn lives WaveSurviveSeconds after; else it dies. */
	void AddWaveLaneStage(FGeoTutorialBot& Bot, FString Name, int32 const WaveWatch, bool const bSurvives,
						  bool const bKillDummies, TFunction<float(TArray<AEnemyCharacter*> const&)> PlaceY)
	{
		float const CentreX = GeoTutorial::RoomCentreX(GeoTutorial::Miniboss);
		TSharedRef<int32> Base = MakeShared<int32>(0);
		TSharedRef<double> FiredAt = MakeShared<double>(0.);
		Bot.AddStage(MoveTemp(Name),
					 [&Bot, WaveWatch, bSurvives, bKillDummies, CentreX, PlaceY = MoveTemp(PlaceY), Base,
					  FiredAt](bool const bFirstTick)
					 {
						 if (bFirstTick)
						 {
							 *Base = Bot.GetWatchCount(WaveWatch);
							 *FiredAt = 0.;
						 }

						 bool bDone = false;
						 if (*FiredAt == 0. && Bot.GetWatchCount(WaveWatch) > *Base)
						 {
							 TArray<AEnemyCharacter*> const Dummies = FindLivingDummies(Bot);
							 if (bSurvives)
							 {
								 Bot.Test.TestFalse(TEXT("A dummy lives to shield the pawn"), Dummies.IsEmpty());
							 }

							 float const Y = PlaceY(Dummies);
							 for (AEnemyCharacter* const Dummy : Dummies)
							 {
								 if (bKillDummies)
								 {
									 Dummy->Death();
								 }
							 }

							 Bot.MovePawn(FVector2D(CentreX - WaveLaneBackoff, Y));
							 Bot.SetInvincible(false);
							 *FiredAt = FPlatformTime::Seconds();
						 }

						 if (*FiredAt != 0.)
						 {
							 bool const bDead = Bot.GetPawn()->IsDead();
							 bDone = bSurvives ? FPlatformTime::Seconds() - *FiredAt >= WaveSurviveSeconds : bDead;
							 if (bDone)
							 {
								 Bot.SetInvincible(true);
								 Bot.Test.TestTrue(TEXT("The wave's outcome for the pawn"), bDead == !bSurvives);
							 }
						 }

						 return bDone;
					 });
	}

	void AddMinibossWaveLaneStages(FGeoTutorialBot& Bot)
	{
		int32 const WaveWatch = WatchMiniboss(Bot, LateralWaveAbility);
		Bot.bKeepPlayerHealthy = true;
		AddMinibossFightStages(Bot);
		AddWaveLaneStage(Bot, TEXT("Miniboss: in a living dummy's lane the pawn survives the wave"), WaveWatch, true,
						 false,
						 [](TArray<AEnemyCharacter*> const& Dummies)
						 {
							 return Dummies.IsEmpty() ? 0.f : Dummies[0]->GetActorLocation().Y;
						 });
		Bot.AddStage(TEXT("Miniboss: the wave is over"),
					 [&Bot](bool)
					 {
						 return !Bot.GetPawn()->IsDead();
					 });
		AddWaveLaneStage(Bot, TEXT("Miniboss: out of every lane the pawn dies"), WaveWatch, false, false,
						 [](TArray<AEnemyCharacter*> const& Dummies)
						 {
							 float BestY = 0.f;
							 float BestDistance = -1.f;
							 for (float const Candidate : {-900.f, -450.f, 0.f, 450.f, 900.f})
							 {
								 float Nearest = TNumericLimits<float>::Max();
								 for (AEnemyCharacter const* const Dummy : Dummies)
								 {
									 Nearest = FMath::Min(Nearest, FMath::Abs(Dummy->GetActorLocation().Y - Candidate));
								 }

								 if (Nearest > BestDistance)
								 {
									 BestDistance = Nearest;
									 BestY = Candidate;
								 }
							 }

							 return BestY;
						 });
		Bot.AddStage(TEXT("Miniboss: the player is back"),
					 [&Bot](bool)
					 {
						 return !Bot.GetPawn()->IsDead();
					 });
		AddWaveLaneStage(Bot, TEXT("Miniboss: with no dummy alive the pawn dies anywhere"), WaveWatch, false, true,
						 [](TArray<AEnemyCharacter*> const& Dummies)
						 {
							 return Dummies.IsEmpty() ? 0.f : Dummies[0]->GetActorLocation().Y;
						 });
	}

	/** Waits for a new play world (the pawn in it) on a map whose name holds MapName. */
	void AddTravelStage(FGeoTutorialBot& Bot, FString Name, FString MapName, FName PlayerStartTag,
						TFunction<void(UWorld*)> Leave)
	{
		TSharedRef<TWeakObjectPtr<UWorld>> OldWorld = MakeShared<TWeakObjectPtr<UWorld>>();
		Bot.AddStage(
			MoveTemp(Name),
			[&Bot, MapName, PlayerStartTag, OldWorld, Leave = MoveTemp(Leave)](bool const bFirstTick)
			{
				if (bFirstTick)
				{
					*OldWorld = Bot.GetWorld();
					Leave(Bot.GetWorld());
				}

				UWorld* const World = Bot.GetWorld();
				bool bArrived = World && World != OldWorld->Get() && Bot.GetPawn()
					&& UWorld::RemovePIEPrefix(World->GetPackage()->GetName()).Contains(MapName);
				if (bArrived && !PlayerStartTag.IsNone())
				{
					APlayerStart const* PlayerStart = nullptr;
					for (TActorIterator<APlayerStart> It(World); It; ++It)
					{
						if (It->PlayerStartTag == PlayerStartTag)
						{
							PlayerStart = *It;
						}
					}

					if (Bot.Test.TestNotNull(
							*FString::Printf(TEXT("A player start tagged %s"), *PlayerStartTag.ToString()),
							PlayerStart))
					{
						Bot.Test.TestTrue(
							*FString::Printf(TEXT("The pawn lands on the %s start"), *PlayerStartTag.ToString()),
							Distance2D(Bot.GetPawn(), FVector2D(PlayerStart->GetActorLocation())) < PlayerStartReach);
					}
				}

				return bArrived;
			},
			120.);
	}

	void AddFinalExitStages(FGeoTutorialBot& Bot)
	{
		AddMinibossFightStages(Bot);
		Bot.AddStage(TEXT("Miniboss: its death opens the final exit"),
					 [&Bot](bool const bFirstTick)
					 {
						 if (bFirstTick)
						 {
							 FindMiniboss(Bot)->Death();
						 }

						 return Bot.IsPadOpen(ExitPad(GeoTutorial::Miniboss));
					 });
		AddTravelStage(
			Bot, TEXT("Miniboss: the final exit leads to the Training Room"), TEXT("DraftMap"), "TrainingRoom",
			[&Bot](UWorld*)
			{
				Bot.GetPawn()->SetActorLocation(Bot.FindPad(ExitPad(GeoTutorial::Miniboss))->GetActorLocation(), false,
												nullptr, ETeleportType::TeleportPhysics);
			});
	}

	void AddLeaveFromMenuStages(FGeoTutorialBot& Bot)
	{
		AddTravelStage(Bot, TEXT("Leave tutorial: the pause menu leads to the Training Room"), TEXT("DraftMap"),
					   "TrainingRoom",
					   [&Bot](UWorld* const World)
					   {
						   Bot.GetController()->OpenPauseMenu();
						   World->GetGameInstance<UGeoGameInstance>()->LeaveTutorial();
					   });
	}

	void AddJumpCommandStages(FGeoTutorialBot& Bot, FString const& Argument, int32 const Room)
	{
		FString const Command = TEXT("Geo.Tutorial.Jump ") + Argument;
		AddTravelStage(Bot, FString::Printf(TEXT("Jump command: '%s' reloads the tutorial"), *Command),
					   TEXT("Tutorial"), FName(*GeoTutorial::RoomName(Room)),
					   [Command](UWorld* const World)
					   {
						   GEngine->Exec(World, *Command);
					   });
		AddJumpCheckStages(Bot, Room);
	}

	/** Waits for the tutorial reloaded at its start: the pawn in the Landing room, movement locked, no class ability,
	 * Landing's first line. */
	void AddFreshTutorialStages(FGeoTutorialBot& Bot, FString Name, TFunction<void(UWorld*)> Leave)
	{
		AddTravelStage(Bot, MoveTemp(Name), TEXT("Tutorial"), NAME_None, MoveTemp(Leave));
		Bot.AddLineStage(TEXT("Fresh tutorial: Landing's first line"), LandingFirstLine);
		Bot.AddStage(TEXT("Fresh tutorial: in Landing, locked, no class ability"),
					 [&Bot](bool)
					 {
						 Bot.Test.TestTrue(TEXT("The pawn is in the Landing room"),
										   FMath::Abs(Bot.GetPawn()->GetActorLocation().X
													  - GeoTutorial::RoomCentreX(GeoTutorial::Landing))
											   < GeoTutorial::RoomHalfX);
						 Bot.Test.TestTrue(TEXT("Landing locks the movement"),
										   Bot.GetWorld()->GetSubsystem<UGeoEncounterSubsystem>()->bMovementLocked);
						 for (TPair<FName, int32> const& Unlock : UnlockRooms)
						 {
							 Bot.Test.TestFalse(*FString::Printf(TEXT("%s not granted"), *Unlock.Key.ToString()),
												Bot.IsGranted(Unlock.Key));
						 }

						 return true;
					 });
	}

	void AddJumpPastEndStages(FGeoTutorialBot& Bot)
	{
		AddFreshTutorialStages(Bot, TEXT("Jump command: a number past the chain starts at Landing"),
							   [](UWorld* const World)
							   {
								   GEngine->Exec(World, TEXT("Geo.Tutorial.Jump 99"));
							   });
	}

	void AddReplayStages(FGeoTutorialBot& Bot)
	{
		AddLeaveFromMenuStages(Bot);
		AddFreshTutorialStages(Bot, TEXT("Replay: the tutorial opened again from the Training Room"),
							   [](UWorld* const World)
							   {
								   World->GetGameInstance<UGeoGameInstance>()->OpenTutorial();
							   });
		Bot.AddStage(TEXT("Replay: the Training Room's portal is not carried over"),
					 [&Bot](bool)
					 {
						 Bot.Test.TestEqual(TEXT("Portal of the replayed tutorial"), Bot.GetWorld()->URL.Portal,
											FString());
						 return true;
					 });
	}

	void AddJumpCheckStages(FGeoTutorialBot& Bot, int32 const Room)
	{
		Bot.AddStage(TEXT("Jump: the pawn on the room's start, earlier abilities granted, Landing never ran"),
					 [&Bot, Room](bool)
					 {
						 APlayerStart const* Start = nullptr;
						 for (TActorIterator<APlayerStart> It(Bot.GetWorld()); It; ++It)
						 {
							 if (It->PlayerStartTag == FName(*GeoTutorial::RoomName(Room)))
							 {
								 Start = *It;
							 }
						 }

						 if (Bot.Test.TestNotNull(TEXT("The room's player start"), Start))
						 {
							 Bot.Test.TestTrue(TEXT("The pawn is on the room's start"),
											   Distance2D(Bot.GetPawn(), FVector2D(Start->GetActorLocation())) < 150.f);
						 }

						 // The room's own unlock may already have run as its step started: only the others are checked.
						 for (TPair<FName, int32> const& Unlock : UnlockRooms)
						 {
							 if (Unlock.Value != Room)
							 {
								 Bot.Test.TestEqual(*FString::Printf(TEXT("%s granted by the skipped rooms only"),
																	 *Unlock.Key.ToString()),
													Bot.IsGranted(Unlock.Key), Unlock.Value < Room);
							 }
						 }
						 Bot.Test.TestFalse(TEXT("Movement is free"),
											Bot.GetWorld()->GetSubsystem<UGeoEncounterSubsystem>()->bMovementLocked);
						 Bot.ExpectNoLine(LandingFirstLine);
						 return true;
					 });
		Bot.AddLineStage(TEXT("Jump: the room's step 0 starts"), FirstRoomLines[Room]);
	}

	void AddPageCheckStage(FGeoTutorialBot& Bot)
	{
		Bot.AddStage(TEXT("Pages: one per unlock, the world paused while each showed"),
					 [&Bot](bool)
					 {
						 Bot.Test.TestEqual(TEXT("Ability pages popped"), Bot.Pages.Num(), 4);
						 for (int32 Index = 0; Index < Bot.Pages.Num(); ++Index)
						 {
							 Bot.Test.TestEqual(*FString::Printf(TEXT("Page %d popped after its unlock"), Index + 1),
												Bot.Pages[Index].GrantedAbilities, Index + 1);
							 Bot.Test.TestTrue(*FString::Printf(TEXT("The world paused for page %d"), Index + 1),
											   Bot.Pages[Index].bWorldPaused);
						 }

						 return true;
					 });
	}
} // namespace GeoTutorialStages

#endif
