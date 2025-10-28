// Fill out your copyright notice in the Description page of Project Settings.

#include "GeoInputGameInstanceSubsystem.h"

#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "GeoInputComponent.h"
#include "GeoStateSubsystem.h"
#include "GeoTrinity/GeoTrinity.h"
#include "HAL/IConsoleManager.h"
#include "InputStep.h"

// Console variable to toggle on-screen debug messages from ServerAddInputBuffer.
// Usage in console: geo.ServerAddInputBuffer.ScreenDebug 1 (enable) or 0 (disable)
static TAutoConsoleVariable<int32> CVarGeoAddInputBufferScreenDebug(TEXT("geo.ServerAddInputBuffer.ScreenDebug"), 0,
	TEXT("When > 0, displays on-screen debug messages from UGeoInputGameInstanceSubsystem::ServerAddInputBuffer."), ECVF_Default);

void UGeoInputGameInstanceSubsystem::ClientUpdateInputBuffer(const TArray<FInputAgent>& UpdatedInputAgents)
{
	// Append Inputs to the
	for (const FInputAgent& InputAgent : UpdatedInputAgents)
	{
		NewInputAgents.FindOrAdd(InputAgent.Owner.Get()).InputSteps.Append(InputAgent.InputSteps);
	}
}

void UGeoInputGameInstanceSubsystem::ServerAddInputBuffer(const FInputStep& InputStep, AGeoPawn* GeoPawn)
{
	if (CVarGeoAddInputBufferScreenDebug.GetValueOnGameThread() > 0)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Emerald, InputStep.ToString());
			GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Blue, GeoPawn ? GeoPawn->GetName() : TEXT("<null pawn>"));
		}
	}

	FInputAgent& InputAgent = GetInputAgent(GeoPawn);

	InputAgent.InputSteps.Add(InputStep);
	if (InputAgent.InputSteps.Num() > MaxBufferInputs)
	{
		// Keep the newest MaxBufferInputs by trimming from the front if needed
		const int32 Excess = InputAgent.InputSteps.Num() - MaxBufferInputs;
		if (Excess > 0)
		{
			InputAgent.InputSteps.RemoveAt(0, Excess, EAllowShrinking::No);
		}
	}
}

void UGeoInputGameInstanceSubsystem::ProcessAgents(const float DeltaTime)
{

	bool bHasNewInput = NewInputAgents.Num() != 0;
	TArray<AGeoPawn*> GeoPawnsToRemove;
	for (TPair<AGeoPawn*, FInputAgent>& Pair : InputAgentsHistory)
	{
		AGeoPawn* GeoPawn = Pair.Key;
		if (!IsValid(GeoPawn))
		{
			GeoPawnsToRemove.Add(GeoPawn);
			continue;
		}

		// When no new info, replay the last input heard from this pawn. Use local deltatime
		if (!bHasNewInput && Pair.Value.InputSteps.Num() > 0)
		{
			GeoPawn->GetGeoInputComponent()->ProcessInput(Pair.Value.InputSteps.Last(), DeltaTime);
		}
	}

	for (AGeoPawn* GeoPawn : GeoPawnsToRemove)
	{
		UE_LOG(LogGeoTrinity, Warning, TEXT("Pawn %s has been removed from the list !"), *GeoPawn->GetName());

		InputAgentsHistory.Remove(GeoPawn);
	}

	if (bHasNewInput)
	{
		FGeoTime FurthestPastServerTime = FGeoTime::GetAccurateRealTime();
		for (TPair<AGeoPawn*, FInputAgent>& Pair : NewInputAgents)
		{
			for (const FInputStep& InputStep : Pair.Value.InputSteps)
			{
				if (InputStep.ServerTime() < FurthestPastServerTime)
				{
					FurthestPastServerTime = InputStep.ServerTime();
				}
			}

			FInputAgent& InputAgent = Pair.Value;
			// Ensure inputs are sorted by time ascending so the newest is last
			InputAgent.InputSteps.Sort(
				[&InputAgent](const FInputStep& A, const FInputStep& B)
				{
					return A.Time < B.Time;
				});
		}
		UGeoStateSubsystem::GetInstance(GetWorld())->RollBackToTime(FurthestPastServerTime);

		// Replay all input since this time
		for (TPair<AGeoPawn*, FInputAgent>& Pair : InputAgentsHistory)
		{
			AGeoPawn* GeoPawn = Pair.Key;
			if (!IsValid(GeoPawn))
			{
				continue;
			}

			for (int i = Pair.Value.InputSteps.Num() - 1; i >= 0; i--)
			{
				const FInputStep& InputStep = Pair.Value.InputSteps[i];
				if (InputStep.Time < FurthestPastServerTime)
				{
					continue;
				}

				GeoPawn->GetGeoInputComponent()->ProcessInput(InputStep);
			}
		}
	}
}

void UGeoInputGameInstanceSubsystem::UpdateClients()
{
	// Build a flat array of all input agents to send to clients
	TArray<FInputAgent> AgentsArray;
	AgentsArray.Reserve(InputAgentsHistory.Num());
	for (const TPair<AGeoPawn*, FInputAgent>& It : InputAgentsHistory)
	{
		AgentsArray.Add(It.Value);
	}

	// Send to each known pawn (those that have sent inputs), excluding their own agent entry
	TArray<AGeoPawn*> KnownPawns;
	InputAgentsHistory.GetKeys(KnownPawns);
	for (const AGeoPawn* KnownPawn : KnownPawns)
	{
		if (!IsValid(KnownPawn))
		{
			continue;
		}
		UGeoInputComponent* GeoInputComponent = KnownPawn->GetGeoInputComponent();
		if (!IsValid(GeoInputComponent))
		{
			continue;
		}

		TArray<FInputAgent> FilteredAgents = AgentsArray;
		FilteredAgents.RemoveAll(
			[KnownPawn](const FInputAgent& Agent)
			{
				return Agent.Owner == KnownPawn;
			});
		if (FilteredAgents.Num() > 0)
		{
			GeoInputComponent->SendForeignInputClientRPC(FilteredAgents);
		}
	}
}

FInputAgent& UGeoInputGameInstanceSubsystem::GetInputAgent(AGeoPawn* GeoPawn)
{
	FInputAgent& InputAgent = InputAgentsHistory.FindOrAdd(GeoPawn, FInputAgent());
	if (!InputAgent.Owner.IsValid())
	{
		InputAgent.Owner = GeoPawn;
	}
	return InputAgent;
}
// ----------------------------------------------------------------------------------
void UGeoInputGameInstanceSubsystem::Tick(float DeltaTime)
{
	checkf(IsInitialized(), TEXT("Ticking should have been disabled for an uninitialized subsystem!"));

	ProcessAgents(DeltaTime);

	// Only update clients from the server
	if (UWorld* World = GetWorld())
	{
		if (World->GetNetMode() != NM_Client)
		{
			UpdateClients();
		}
	}
}

UWorld* UGeoInputGameInstanceSubsystem::GetTickableGameObjectWorld() const
{
	return GetWorld();
}

ETickableTickType UGeoInputGameInstanceSubsystem::GetTickableTickType() const
{
	// If this is a template or has not been initialized yet, set to never tick and it will be enabled when it is initialized
	if (IsTemplate() || !bInitialized)
	{
		return ETickableTickType::Never;
	}

	// Otherwise default to conditional
	return ETickableTickType::Conditional;
}

bool UGeoInputGameInstanceSubsystem::IsTickable() const
{
	// This function is now deprecated and subclasses should implement IsTickable instead
	// This should never be false because Initialize should always be called before the first tick and Deinitialize cancels the tick
	ensureMsgf(bInitialized, TEXT("Tickable subsystem %s tried to tick when not initialized! Check for missing Super call"),
		*GetFullName());

	return bInitialized;
}

TStatId UGeoInputGameInstanceSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UGeoInputGameInstanceSubsystem, STATGROUP_Tickables);
}

void UGeoInputGameInstanceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	check(!bInitialized);
	bInitialized = true;

	// Refresh the tick type after initialization
	SetTickableTickType(GetTickableTickType());
}

void UGeoInputGameInstanceSubsystem::Deinitialize()
{
	check(bInitialized);
	bInitialized = false;

	// Always cancel tick as this is about to be destroyed
	SetTickableTickType(ETickableTickType::Never);
}

UGeoInputGameInstanceSubsystem* UGeoInputGameInstanceSubsystem::GetInstance(const UWorld* World)
{
	return World->GetGameInstance()->GetSubsystem<UGeoInputGameInstanceSubsystem>();
}