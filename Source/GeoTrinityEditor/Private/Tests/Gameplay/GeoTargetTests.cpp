// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "AbilitySystem/Lib/GeoTarget.h"
#include "Actor/GeoTargetPoint.h"
#include "Characters/EnemyCharacter.h"
#include "Characters/GeoEnemySetup.h"
#include "Characters/PlayableCharacter.h"
#include "Engine/GameInstance.h"
#include "GameClasses/Match/GeoGameState.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagsManager.h"
#include "Misc/AutomationTest.h"
#include "Tests/Gameplay/Fake/GeoTestEncounter.h"
#include "Tests/Gameplay/Fake/GeoTestPatternAbility.h"
#include "Tests/Gameplay/Fake/GeoTestWorld.h"
#include "Tool/Team.h"

#if WITH_DEV_AUTOMATION_TESTS

constexpr EAutomationTestFlags GeoTargetTestFlags =
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

/** A world with a game state and a caster arena, its boss the caster; no player yet. */
static AGeoTestCasterArena* PrepareTargetWorld(FAutomationTestBase& Test, UWorld& World)
{
	World.SetGameInstance(NewObject<UGameInstance>(GEngine));
	Test.AddExpectedError(TEXT("Missing DefaultAttributes"), EAutomationExpectedErrorFlags::Contains, -1);
	World.SetGameState(World.SpawnActor<AGeoGameState>());
	return World.SpawnActor<AGeoTestCasterArena>(FVector(0.f, 1000.f, 0.f), FRotator::ZeroRotator);
}

/** A live player at Location, possessed by its own controller. */
static APlayableCharacter* SpawnPlayer(UWorld& World, FVector const& Location)
{
	FTransform const Transform(Location);
	APlayableCharacter* const Player =
		World.SpawnActorDeferred<APlayableCharacter>(APlayableCharacter::StaticClass(), Transform);
	Player->SetActorHiddenInGame(true);
	Player->FinishSpawning(Transform);
	World.SpawnActor<APlayerController>()->SetPawn(Player);
	return Player;
}

/** Sets Caster's health to Ratio of a 100 max health. Caster must not be invulnerable (the hidden caster is). */
static void SetHealthRatio(AActor* const Caster, float const Ratio)
{
	UGeoAbilitySystemComponent* const ASC = GeoASLib::GetGeoAscFromActor(Caster);
	ASC->SetNumericAttributeBase(UGeoAttributeSetBase::GetMaxHealthAttribute(), 100.f);
	ASC->SetNumericAttributeBase(UGeoAttributeSetBase::GetHealthAttribute(), 100.f * Ratio);
}

static FGeoTarget MakeTarget(EGeoTarget const Mode)
{
	FGeoTarget Target;
	Target.Mode = Mode;
	return Target;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoTargetModesTest, "GeoTrinity.Abilities.Target.Modes", GeoTargetTestFlags)

bool FGeoTargetModesTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	AGeoTestCasterArena* const Arena = PrepareTargetWorld(*this, *World);
	FGeoTargetContext Context;
	Context.Caster = Arena->GetBoss();
	Context.Origin = FVector2D(100.f, 0.f);
	Context.Yaw = 90.f;

	FGeoTarget Forward = MakeTarget(EGeoTarget::Forward);
	TArray<FVector2D> Locations = GeoASLib::GetTargetLocations(Forward, Context);
	if (TestEqual(TEXT("Forward 0: one location"), Locations.Num(), 1))
	{
		TestTrue(TEXT("Forward 0: the origin itself"), Locations[0].Equals(Context.Origin, 0.01));
	}

	Forward.Distance = 200.f;
	Locations = GeoASLib::GetTargetLocations(Forward, Context);
	if (TestEqual(TEXT("Forward: one location"), Locations.Num(), 1))
	{
		TestTrue(TEXT("Forward: origin + yaw x distance"), Locations[0].Equals(FVector2D(100.f, 200.f), 0.01));
	}

	TArray<FVector> const Directions =
		GeoASLib::GetTargetDirections(MakeTarget(EGeoTarget::Forward), Context, FVector(Context.Origin, 0.f));
	if (TestEqual(TEXT("Zero length: one direction"), Directions.Num(), 1))
	{
		TestTrue(TEXT("Zero length: the yaw"), Directions[0].Equals(FVector(0.f, 1.f, 0.f), 0.01));
	}

	TestEqual(TEXT("RandomPlayers, no player: none"),
			  GeoASLib::GetTargetLocations(MakeTarget(EGeoTarget::RandomPlayers), Context).Num(), 0);
	TestEqual(TEXT("AllPlayers, no player: none"),
			  GeoASLib::GetTargetLocations(MakeTarget(EGeoTarget::AllPlayers), Context).Num(), 0);

	APlayableCharacter* const First = SpawnPlayer(*World, FVector(1000.f, 0.f, 0.f));
	APlayableCharacter* const Second = SpawnPlayer(*World, FVector(-1000.f, 0.f, 0.f));
	Locations = GeoASLib::GetTargetLocations(MakeTarget(EGeoTarget::AllPlayers), Context);
	if (TestEqual(TEXT("AllPlayers: one per live player"), Locations.Num(), 2))
	{
		TestTrue(TEXT("At each player"),
				 Locations.Contains(FVector2D(First->GetActorLocation()))
					 && Locations.Contains(FVector2D(Second->GetActorLocation())));
	}

	TestEqual(TEXT("Boss, no fight: none"), GeoASLib::GetTargetLocations(MakeTarget(EGeoTarget::Boss), Context).Num(),
			  0);
	Arena->StartFight();
	Locations = GeoASLib::GetTargetLocations(MakeTarget(EGeoTarget::Boss), Context);
	if (TestEqual(TEXT("Boss, fighting: one"), Locations.Num(), 1))
	{
		TestTrue(TEXT("At the boss"), Locations[0].Equals(FVector2D(Arena->GetBoss()->GetActorLocation()), 0.01));
	}

	Context.Caster = First;
	Arena->GetBoss()->Death();
	TestEqual(TEXT("Boss dead: none"), GeoASLib::GetTargetLocations(MakeTarget(EGeoTarget::Boss), Context).Num(), 0);

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoTargetPointTest, "GeoTrinity.Abilities.Target.TargetPoint", GeoTargetTestFlags)

bool FGeoTargetPointTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	AGeoTestCasterArena* const Arena = PrepareTargetWorld(*this, *World);
	FGameplayTag const PointTag = FGeoGameplayTags::Get().TargetPoint_AddSpawn;
	FGameplayTag const RoomTag = UGameplayTagsManager::Get().RequestGameplayTag(TEXT("Arena.Tutorial.Zones"));
	FGameplayTag const OtherRoomTag = UGameplayTagsManager::Get().RequestGameplayTag(TEXT("Arena.Hex"));
	for (TPair<FGameplayTag, FVector> const& Point :
		 {TPair<FGameplayTag, FVector>(OtherRoomTag, FVector(-700.f, 0.f, 0.f)),
		  TPair<FGameplayTag, FVector>(RoomTag, FVector(400.f, 250.f, 0.f))})
	{
		AGeoTargetPoint* const TargetPoint = World->SpawnActor<AGeoTargetPoint>(Point.Value, FRotator());
		TargetPoint->GameplayTags.AddTag(PointTag);
		TargetPoint->GameplayTags.AddTag(Point.Key);
	}

	FGeoTarget Target = MakeTarget(EGeoTarget::TargetPoint);
	Target.TargetPointTag = PointTag;
	FGeoTargetContext Context;
	Context.Caster = Arena->GetBoss();
	Context.ArenaTag = RoomTag;
	TArray<FVector2D> const Locations = GeoASLib::GetTargetLocations(Target, Context);
	if (TestEqual(TEXT("One point"), Locations.Num(), 1))
	{
		TestTrue(TEXT("Its room's, not another room's"), Locations[0].Equals(FVector2D(400.f, 250.f), 0.01));
	}

	AddExpectedErrorPlain(TEXT("Ensure condition failed: !Points.IsEmpty()"), EAutomationExpectedErrorFlags::Contains,
						  -1);
	AddExpectedErrorPlain(TEXT("no TargetPoint.AddSpawn point in room None"), EAutomationExpectedErrorFlags::Contains,
						  -1);
	AddExpectedError(TEXT("=== Handled ensure: ===|^\\s*$|^Stack:|\\[Callstack\\]"),
					 EAutomationExpectedErrorFlags::Contains, -1);
	Context.ArenaTag = FGameplayTag();
	TestEqual(TEXT("No room: none"), GeoASLib::GetTargetLocations(Target, Context).Num(), 0);

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoTargetArenaCenterTest, "GeoTrinity.Abilities.Target.ArenaCenter",
								 GeoTargetTestFlags)

bool FGeoTargetArenaCenterTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	AGeoTestCasterArena* const Arena = PrepareTargetWorld(*this, *World);
	FGeoTarget const Target = MakeTarget(EGeoTarget::ArenaCenter);
	FGeoTargetContext Context;
	Context.Caster = Arena->GetBoss();

	TArray<FGeoTargetPick> const Picks = GeoASLib::PickTargets(Target, Context);
	if (TestEqual(TEXT("One pick"), Picks.Num(), 1))
	{
		TestTrue(TEXT("At the arena's location"), Picks[0].Location.Equals(FVector2D(Arena->GetActorLocation()), 0.01));
		TestFalse(TEXT("No actor to follow"), Picks[0].Actor.IsValid());
	}

	AddExpectedErrorPlain(TEXT("Ensure condition failed: Arena"), EAutomationExpectedErrorFlags::Contains, -1);
	AddExpectedErrorPlain(TEXT("has no arena"), EAutomationExpectedErrorFlags::Contains, -1);
	AddExpectedError(TEXT("=== Handled ensure: ===|^\\s*$|^Stack:|\\[Callstack\\]"),
					 EAutomationExpectedErrorFlags::Contains, -1);
	Context.Caster = World->SpawnActor<AEnemyCharacter>();
	TestEqual(TEXT("A boss with no arena: none"), GeoASLib::GetTargetLocations(Target, Context).Num(), 0);

	UGeoTestTargetPatternAbility* const Ability = NewObject<UGeoTestTargetPatternAbility>();
	TestFalse(TEXT("Another target needs no arena"), Ability->RequiresArena());
	Ability->SetTarget(Target);
	TestTrue(TEXT("A pattern ability aiming there requires one"), Ability->RequiresArena());

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoTargetRandomPlayersTest, "GeoTrinity.Abilities.Target.RandomPlayers",
								 GeoTargetTestFlags)

bool FGeoTargetRandomPlayersTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	AGeoTestCasterArena* const Arena = PrepareTargetWorld(*this, *World);
	APlayableCharacter* const First = SpawnPlayer(*World, FVector(1000.f, 0.f, 0.f));
	APlayableCharacter* const Second = SpawnPlayer(*World, FVector(-1000.f, 0.f, 0.f));

	FGeoTarget Target = MakeTarget(EGeoTarget::RandomPlayers);
	Target.Count = 1;
	Target.ExtraBelowHealth = {0.5f, 0.2f};
	Target.MinScatterRadius = 300.f;
	Target.MaxScatterRadius = 800.f;
	FGeoTargetContext Context;
	Context.Caster = Arena->SpawnAdd(AEnemyCharacter::StaticClass(), FTransform(), nullptr);
	Context.Seed = 1234;

	SetHealthRatio(Context.Caster, 1.f);
	TestEqual(TEXT("Full health: Count"), GeoASLib::PickTargets(Target, Context).Num(), 1);
	SetHealthRatio(Context.Caster, 0.4f);
	TestEqual(TEXT("Below half: one more"), GeoASLib::PickTargets(Target, Context).Num(), 2);
	SetHealthRatio(Context.Caster, 0.1f);
	TArray<FGeoTargetPick> const Picks = GeoASLib::PickTargets(Target, Context);
	if (TestEqual(TEXT("Below a fifth: one more again, whatever the player count"), Picks.Num(), 3))
	{
		TestTrue(TEXT("One per player first"),
				 Picks[0].Actor != Picks[1].Actor && Picks[0].Offset.IsZero() && Picks[1].Offset.IsZero());
		AActor const* const Shared = Picks[2].Actor.Get();
		TestTrue(TEXT("Beyond the alive count: on a player"), Shared == First || Shared == Second);
		float const Scatter = Picks[2].Offset.Size();
		TestTrue(TEXT("Scattered within the radii"), Scatter >= 300.f - 0.1f && Scatter <= 800.f + 0.1f);
	}

	TArray<FGeoTargetPick> const Again = GeoASLib::PickTargets(Target, Context);
	bool bSame = Again.Num() == Picks.Num();
	for (int32 Index = 0; bSame && Index < Picks.Num(); ++Index)
	{
		bSame = Again[Index].Actor == Picks[Index].Actor && Again[Index].Offset.Equals(Picks[Index].Offset);
	}
	TestTrue(TEXT("Same seed, same picks"), bSame);

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoTargetResolveTest, "GeoTrinity.Abilities.Target.Resolve", GeoTargetTestFlags)

bool FGeoTargetResolveTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	AActor* const Followed = World->SpawnActor<AGeoTargetPoint>(FVector(100.f, 0.f, 0.f), FRotator());
	FGeoTargetPick const Pick{Followed, FVector2D(10.f, 0.f), FVector2D(110.f, 0.f)};

	Followed->SetActorLocation(FVector(500.f, 500.f, 0.f));
	TArray<FVector2D> Locations = GeoASLib::ResolvePicks({Pick});
	TestTrue(TEXT("A live actor is followed, offset kept"), Locations[0].Equals(FVector2D(510.f, 500.f), 0.01));

	Followed->Destroy();
	Locations = GeoASLib::ResolvePicks({Pick});
	TestTrue(TEXT("A gone actor resolves to where it was picked"), Locations[0].Equals(FVector2D(110.f, 0.f), 0.01));

	DestroyTestWorld(World);
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoTargetPatternTest, "GeoTrinity.Abilities.Target.Pattern", GeoTargetTestFlags)

bool FGeoTargetPatternTest::RunTest(FString const& /*Parameters*/)
{
	UWorld* World = CreateTestWorld();
	AGeoTestCasterArena* const Arena = PrepareTargetWorld(*this, *World);
	AEnemyCharacter* const Caster = Arena->GetBoss();
	UGeoAbilitySystemComponent* const ASC = GeoASLib::GetGeoAscFromActor(Caster);
	FGameplayAbilitySpecHandle const Handle =
		ASC->GiveAbility(FGameplayAbilitySpec(UGeoTestTargetPatternAbility::StaticClass(), 1));
	UGeoTestTargetPatternAbility* const Ability =
		CastChecked<UGeoTestTargetPatternAbility>(ASC->FindAbilitySpecFromHandle(Handle)->GetPrimaryInstance());
	UGeoTestTargetPattern* const Pattern = CastChecked<UGeoTestTargetPattern>(
		ASC->CreatePatternInstance(UGeoTestTargetPattern::StaticClass(), FGameplayTag()));
	auto const Launch = [&](FGeoTarget const& Target)
	{
		ASC->CancelAbilityHandle(Handle);
		Ability->SetTarget(Target);
		ASC->TryActivateAbility(Handle);
	};

	APlayableCharacter* const First = SpawnPlayer(*World, FVector(1000.f, 1000.f, 0.f));
	SpawnPlayer(*World, FVector(-1000.f, 1000.f, 0.f));
	Launch(MakeTarget(EGeoTarget::AllPlayers));
	FVector2D const CasterOrigin(Caster->GetActorLocation());
	if (TestEqual(TEXT("Launched once"), Pattern->LaunchCount, 1)
		&& TestEqual(TEXT("One location per player"), Pattern->GetTargetLocations().Num(), 2))
	{
		TestTrue(TEXT("The payload is the caster"), Pattern->GetLaunchedPayload().Origin.Equals(CasterOrigin, 0.01));
		TestTrue(TEXT("The targets travel apart"),
				 Pattern->GetTargetLocations().Contains(FVector2D(First->GetActorLocation())));
		FVector2D const ToFirst = Pattern->GetTargetLocations()[0] - CasterOrigin;
		TestEqual(TEXT("Yaw towards the first"), Pattern->GetYawTowards(0),
				  static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(ToFirst.Y, ToFirst.X))), 0.1f);
	}

	Launch(MakeTarget(EGeoTarget::Forward));
	if (TestEqual(TEXT("Forward 0 launched"), Pattern->LaunchCount, 2))
	{
		TestTrue(TEXT("Forward 0: the caster's place"),
				 Pattern->GetTargetLocations()[0].Equals(Pattern->GetLaunchedPayload().Origin, 0.01));
		TestEqual(TEXT("Forward 0: the caster's facing"), Pattern->GetYawTowards(0), Pattern->GetLaunchedPayload().Yaw,
				  0.1f);
	}

	Launch(MakeTarget(EGeoTarget::Boss));
	TestEqual(TEXT("No target: nothing launched"), Pattern->LaunchCount, 2);
	TestFalse(TEXT("No target: the ability ended"), ASC->FindAbilitySpecFromHandle(Handle)->IsActive());

	FGeoTarget ThreeOnTwo = MakeTarget(EGeoTarget::RandomPlayers);
	ThreeOnTwo.Count = 3;
	Launch(ThreeOnTwo);
	TestEqual(TEXT("Three picks on two players: three locations"), Pattern->GetTargetLocations().Num(), 3);

	FGameplayTag const CueTag = FGeoGameplayTags::Get().Ability_Spell_ShieldBurst;
	Ability->SetPreLaunch(1.f, CueTag);
	Launch(MakeTarget(EGeoTarget::Forward));
	TestEqual(TEXT("Pre-launch, a pick with no actor: the caster marked"), Ability->GetPreLaunchMarkCount(), 1);
	TestTrue(TEXT("(its ASC)"), ASC->IsGameplayCueActive(CueTag));
	Ability->MarkForPreLaunch(ASC);
	TestEqual(TEXT("Pre-launch, a second pick on one actor: still one mark"), Ability->GetPreLaunchMarkCount(), 1);
	TestEqual(TEXT("Pre-launch: not launched yet"), Pattern->LaunchCount, 3);
	TickTestTimers(*World, 0.f);
	TickTestTimers(*World, 1.1f);
	TestEqual(TEXT("Launched after the delay"), Pattern->LaunchCount, 4);
	TestEqual(TEXT("Marks dropped at launch"), Ability->GetPreLaunchMarkCount(), 0);
	ASC->CancelAbilityHandle(Handle);

	FGeoEnemySetup AllySetup;
	AllySetup.Team = ETeam::Player;
	AEnemyCharacter* const Ally = Arena->SpawnAdd(AEnemyCharacter::StaticClass(), FTransform(), &AllySetup);
	UGeoAbilitySystemComponent* const AllyASC = GeoASLib::GetGeoAscFromActor(Ally);
	FGameplayAbilitySpecHandle const AllyHandle =
		AllyASC->GiveAbility(FGameplayAbilitySpec(UGeoTestTargetPatternAbility::StaticClass(), 1));
	UGeoTestTargetPatternAbility* const AllyAbility =
		CastChecked<UGeoTestTargetPatternAbility>(AllyASC->FindAbilitySpecFromHandle(AllyHandle)->GetPrimaryInstance());
	AllyASC->CreatePatternInstance(UGeoTestTargetPattern::StaticClass(), FGameplayTag());
	Arena->StartFight();
	AllyAbility->SetTarget(MakeTarget(EGeoTarget::Boss));
	AllyAbility->SetPreLaunch(1.f, CueTag);
	AllyASC->TryActivateAbility(AllyHandle);
	TestEqual(TEXT("Pre-launch, a pick with an actor: that actor marked"), AllyAbility->GetPreLaunchMarkCount(), 1);
	TestTrue(TEXT("(the boss, not the caster)"),
			 ASC->IsGameplayCueActive(CueTag) && !AllyASC->IsGameplayCueActive(CueTag));

	AllyASC->CancelAbilityHandle(AllyHandle);
	DestroyTestWorld(World);
	return true;
}

#endif
