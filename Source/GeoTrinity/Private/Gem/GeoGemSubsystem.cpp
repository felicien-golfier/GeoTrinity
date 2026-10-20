// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Gem/GeoGemSubsystem.h"

#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameClasses/Match/GeoPlayerState.h"
#include "GameFramework/PlayerController.h"
#include "Gem/GeoGemCatalog.h"
#include "Gem/GeoGemComponent.h"
#include "Gem/GeoGemProfileSave.h"
#include "GeoTrinity/GeoTrinity.h"

void UGeoGemSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Profile = UGeoGemProfileSave::Load(GetLocalPlayerChecked()->GetLocalPlayerIndex());
}

UGeoGemProfileSave* UGeoGemSubsystem::GetFirstLocalProfile(UWorld const* World)
{
	ULocalPlayer const* const LocalPlayer = World ? World->GetFirstLocalPlayerFromController() : nullptr;
	UGeoGemSubsystem const* const GemSubsystem = LocalPlayer ? LocalPlayer->GetSubsystem<UGeoGemSubsystem>() : nullptr;
	return GemSubsystem ? GemSubsystem->GetProfile() : nullptr;
}

void UGeoGemSubsystem::CommitChanges()
{
	ULocalPlayer const* LocalPlayer = GetLocalPlayerChecked();
	Profile->Save(LocalPlayer->GetLocalPlayerIndex());

	APlayerController const* PlayerController = LocalPlayer->GetPlayerController(GetWorld());
	if (AGeoPlayerState const* PlayerState =
			PlayerController ? PlayerController->GetPlayerState<AGeoPlayerState>() : nullptr)
	{
		PlayerState->GetGemComponent()->SendLoadoutsToServer(*Profile);
	}

	OnProfileCommitted.Broadcast();
}

void UGeoGemSubsystem::GrantReward(FGeoGemReward const& Reward)
{
	UGeoGemCatalog const* Catalog = UGeoGemCatalog::Get();
	if (!Catalog)
	{
		return;
	}

	int32 const LevelsGained = Profile->AddClassXp(*Catalog, Reward.PlayerClass, Reward.Xp);
	int32 GemCount = 0;
	for (FGeoGemStack const& Stack : Reward.Gems)
	{
		Profile->AddGems(Stack.Id, Stack.Count);
		GemCount += Stack.Count;
	}
	Profile->AddShards(Reward.Shards);
	UE_LOG(LogGeoTrinity, Display, TEXT("Gems: %s +%d XP (+%d level), %d gem(s), %d shard(s)"),
		   *UEnum::GetValueAsString(Reward.PlayerClass), Reward.Xp, LevelsGained, GemCount, Reward.Shards);
	CommitChanges();
}

#if !UE_BUILD_SHIPPING
static UGeoGemSubsystem* GetFirstLocalGemSubsystem(UWorld const* World)
{
	ULocalPlayer const* LocalPlayer = World ? World->GetFirstLocalPlayerFromController() : nullptr;
	return LocalPlayer ? LocalPlayer->GetSubsystem<UGeoGemSubsystem>() : nullptr;
}

static EPlayerClass ParsePlayerClass(FString const& Name)
{
	int64 const Value = StaticEnum<EPlayerClass>()->GetValueByNameString(Name);
	return Value == INDEX_NONE ? EPlayerClass::None : static_cast<EPlayerClass>(Value);
}

static FAutoConsoleCommandWithWorldAndArgs
	GGemsGrantCommand(TEXT("Geo.Gems.Grant"),
					  TEXT("Geo.Gems.Grant <GemId|All> <Count> — adds gems to the first local player's stacks"),
					  FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
						  [](TArray<FString> const& Args, UWorld* World)
						  {
							  UGeoGemSubsystem* Gems = GetFirstLocalGemSubsystem(World);
							  UGeoGemCatalog const* Catalog = UGeoGemCatalog::Get();
							  if (Gems && Catalog && Args.Num() == 2)
							  {
								  int32 const Count = FCString::Atoi(*Args[1]);
								  for (TPair<EGeoGemTier, FGeoGemList> const& Tier : Catalog->GemsByTier)
								  {
									  for (FGeoGemInfo const& Gem : Tier.Value.Gems)
									  {
										  if (Args[0] == TEXT("All") || Gem.Id == FName(Args[0]))
										  {
											  Gems->GetProfile()->AddGems(Gem.Id, Count);
										  }
									  }
								  }
								  Gems->CommitChanges();
							  }
						  }));

static FAutoConsoleCommandWithWorldAndArgs GGemsSetLevelCommand(
	TEXT("Geo.Gems.SetLevel"), TEXT("Geo.Gems.SetLevel <Triangle|Circle|Square> <Level> — sets a class level"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
		[](TArray<FString> const& Args, UWorld* World)
		{
			UGeoGemSubsystem* Gems = GetFirstLocalGemSubsystem(World);
			if (Gems && Args.Num() == 2)
			{
				Gems->GetProfile()->SetClassLevel(ParsePlayerClass(Args[0]), FCString::Atoi(*Args[1]));
				Gems->CommitChanges();
			}
		}));

static FAutoConsoleCommandWithWorldAndArgs
	GGemsSetEchoLevelCommand(TEXT("Geo.Gems.SetEchoLevel"),
							 TEXT("Geo.Gems.SetEchoLevel <Level> — sets the shared echo level"),
							 FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
								 [](TArray<FString> const& Args, UWorld* World)
								 {
									 UGeoGemSubsystem* Gems = GetFirstLocalGemSubsystem(World);
									 UGeoGemCatalog const* Catalog = UGeoGemCatalog::Get();
									 if (Gems && Catalog && Args.Num() == 1)
									 {
										 Gems->GetProfile()->SetEchoLevel(*Catalog, FCString::Atoi(*Args[0]));
										 Gems->CommitChanges();
									 }
								 }));

static FAutoConsoleCommandWithWorldAndArgs GGemsAddXpCommand(
	TEXT("Geo.Gems.AddXp"),
	TEXT("Geo.Gems.AddXp <Triangle|Circle|Square> <Xp> — earns class XP; past the max level it feeds the echo"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
		[](TArray<FString> const& Args, UWorld* World)
		{
			UGeoGemSubsystem* Gems = GetFirstLocalGemSubsystem(World);
			UGeoGemCatalog const* Catalog = UGeoGemCatalog::Get();
			if (Gems && Catalog && Args.Num() == 2)
			{
				Gems->GetProfile()->AddClassXp(*Catalog, ParsePlayerClass(Args[0]), FCString::Atoi(*Args[1]));
				Gems->CommitChanges();
			}
		}));

static FAutoConsoleCommandWithWorldAndArgs GGemsEchoCommand(
	TEXT("Geo.Gems.Echo"),
	TEXT(
		"Geo.Gems.Echo <Triangle|Circle|Square> <GemId|None> — echoes a gem in the class's build, or empties the echo"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
		[](TArray<FString> const& Args, UWorld* World)
		{
			UGeoGemSubsystem* Gems = GetFirstLocalGemSubsystem(World);
			UGeoGemCatalog const* Catalog = UGeoGemCatalog::Get();
			if (Gems && Catalog && Args.Num() == 2)
			{
				EPlayerClass const PlayerClass = ParsePlayerClass(Args[0]);
				if (Args[1] == TEXT("None"))
				{
					Gems->GetProfile()->ClearEcho(PlayerClass);
				}
				else
				{
					bool const bEchoed = Gems->GetProfile()->SetEcho(*Catalog, PlayerClass, FName(Args[1]));
					UE_LOG(LogGeoTrinity, Display, TEXT("Geo.Gems.Echo: %s"),
						   bEchoed ? TEXT("echoed") : TEXT("refused"));
				}

				Gems->CommitChanges();
			}
		}));

static FAutoConsoleCommandWithWorldAndArgs GGemsFillCommand(
	TEXT("Geo.Gems.Fill"),
	TEXT(
		"Geo.Gems.Fill <Triangle|Circle|Square> <GemId> — fills every socket of the gem's tier a class has open, replacing slotted gems"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
		[](TArray<FString> const& Args, UWorld* World)
		{
			UGeoGemSubsystem* Gems = GetFirstLocalGemSubsystem(World);
			UGeoGemCatalog const* Catalog = UGeoGemCatalog::Get();
			if (Gems && Catalog && Args.Num() == 2)
			{
				int32 const Filled =
					Gems->GetProfile()->FillSockets(*Catalog, ParsePlayerClass(Args[0]), FName(Args[1]));
				UE_LOG(LogGeoTrinity, Display, TEXT("Geo.Gems.Fill: %d socket(s) filled"), Filled);
				Gems->CommitChanges();
			}
		}));

static FAutoConsoleCommandWithWorldAndArgs GGemsUnequipAllCommand(
	TEXT("Geo.Gems.UnequipAll"), TEXT("Geo.Gems.UnequipAll <Triangle|Circle|Square> — empties every socket of a class"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
		[](TArray<FString> const& Args, UWorld* World)
		{
			UGeoGemSubsystem* Gems = GetFirstLocalGemSubsystem(World);
			if (Gems && Args.Num() == 1)
			{
				for (int32 SocketIndex = 0; SocketIndex < GeoGem::GetSockets().Num(); ++SocketIndex)
				{
					Gems->GetProfile()->Unequip(ParsePlayerClass(Args[0]), SocketIndex);
				}
				Gems->CommitChanges();
			}
		}));

static FAutoConsoleCommandWithWorld GGemsDumpCommand(
	TEXT("Geo.Gems.Dump"), TEXT("Logs the first local player's shards, stacks and class levels"),
	FConsoleCommandWithWorldDelegate::CreateLambda(
		[](UWorld* World)
		{
			UGeoGemSubsystem const* Gems = GetFirstLocalGemSubsystem(World);
			UGeoGemCatalog const* Catalog = UGeoGemCatalog::Get();
			if (Gems && Catalog)
			{
				UGeoGemProfileSave const* Profile = Gems->GetProfile();
				UE_LOG(LogGeoTrinity, Display, TEXT("Gems: %d shards, echo level %d, %d XP"), Profile->GetShards(),
					   Profile->GetEchoLevel(), Profile->GetEchoXp());
				for (TPair<EGeoGemTier, FGeoGemList> const& Tier : Catalog->GemsByTier)
				{
					for (FGeoGemInfo const& Gem : Tier.Value.Gems)
					{
						UE_LOG(LogGeoTrinity, Display, TEXT("  %s: %d owned, %d breakable"), *Gem.Id.ToString(),
							   Profile->GetOwnedCount(Gem.Id), Profile->GetBreakableCount(Gem.Id));
					}
				}
				for (EPlayerClass const PlayerClass :
					 {EPlayerClass::Triangle, EPlayerClass::Circle, EPlayerClass::Square})
				{
					UE_LOG(LogGeoTrinity, Display, TEXT("  %s: level %d, %d XP, echo %s"),
						   *UEnum::GetValueAsString(PlayerClass), Profile->GetClassLevel(PlayerClass),
						   Profile->GetClassXp(PlayerClass), *Profile->GetEcho(PlayerClass).ToString());
				}
			}
		}));
#endif
