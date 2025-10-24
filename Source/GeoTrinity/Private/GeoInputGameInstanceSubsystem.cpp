// Fill out your copyright notice in the Description page of Project Settings.

#include "GeoInputGameInstanceSubsystem.h"

#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "GeoInputComponent.h"
#include "GeoTrinity/GeoTrinity.h"
#include "HAL/IConsoleManager.h"
#include "InputStep.h"

// Console variable to toggle on-screen debug messages from ServerAddInputBuffer.
// Usage in console: geo.ServerAddInputBuffer.ScreenDebug 1 (enable) or 0 (disable)
static TAutoConsoleVariable<int32> CVarGeoAddInputBufferScreenDebug(TEXT("geo.ServerAddInputBuffer.ScreenDebug"), 0,
	TEXT("When > 0, displays on-screen debug messages from UGeoInputGameInstanceSubsystem::ServerAddInputBuffer."), ECVF_Default);

void UGeoInputGameInstanceSubsystem::ClientUpdateInputBuffer(const TArray<FInputAgent>& UpdatedInputAgents)
{
	for (const FInputAgent& UpdatedInputAgent : UpdatedInputAgents)
	{
		FInputAgent& InputAgent = GetInputAgent(UpdatedInputAgent.Owner);
		InputAgent.InputSteps = UpdatedInputAgent.InputSteps;
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
	TArray<AGeoPawn*> GeoPawnsToRemove;
	for (TPair<AGeoPawn*, FInputAgent>& Pair : InputAgents)
	{
		AGeoPawn* GeoPawn = Pair.Key;
		if (!IsValid(GeoPawn))
		{
			GeoPawnsToRemove.Add(GeoPawn);
			continue;
		}

		FInputAgent& InputAgent = Pair.Value;
		if (InputAgent.InputSteps.Num() == 0)
		{
			// No input to process
			continue;
		}

		// Choose the most recent step. Prefer server-time if provided, otherwise fall back to client time minus ping
		int32 BestIndex = 0;
		double BestScore = TNumericLimits<double>::Lowest();
		for (int32 i = 0; i < InputAgent.InputSteps.Num(); ++i)
		{
			const FInputStep& InputStep = InputAgent.InputSteps[i];
			double Score;
			if (InputStep.ServerTimeSeconds > 0.0)
			{
				Score = InputStep.ServerTimeSeconds;
			}
			else
			{
				double StepTime = static_cast<double>(InputStep.TimeSeconds) + InputStep.TimePartialSeconds;
				double Adjusted = StepTime - static_cast<double>(InputStep.Ping) / 1000.0;   // subtract RTT seconds
				Score = Adjusted;
			}

			if (Score > BestScore)
			{
				BestScore = Score;
				BestIndex = i;
			}
		}

		const FInputStep SelectedInputStep = InputAgent.InputSteps[BestIndex];
		GeoPawn->GetGeoInputComponent()->ProcessInput(SelectedInputStep);
	}

	for (AGeoPawn* GeoPawn : GeoPawnsToRemove)
	{
		UE_LOG(LogGeoTrinity, Warning, TEXT("Pawn %s has been removed from the list !"), *GeoPawn->GetName());

		InputAgents.Remove(GeoPawn);
		GeoPawns.Remove(GeoPawn);
	}
}

void UGeoInputGameInstanceSubsystem::UpdateClients()
{
	// Build a flat array of all input agents to send to clients
	TArray<FInputAgent> AgentsArray;
	AgentsArray.Reserve(InputAgents.Num());
	for (const TPair<AGeoPawn*, FInputAgent>& It : InputAgents)
	{
		AgentsArray.Add(It.Value);
	}

	// Send to each known pawn (those that have sent inputs), excluding their own agent entry
	for (const TWeakObjectPtr<AGeoPawn>& WeakPawn : GeoPawns)
	{
		AGeoPawn* PlayerPawn = WeakPawn.Get();
		if (!IsValid(PlayerPawn))
		{
			continue;
		}
		UGeoInputComponent* GeoInputComponent = PlayerPawn->GetGeoInputComponent();
		if (!IsValid(GeoInputComponent))
		{
			continue;
		}

		TArray<FInputAgent> FilteredAgents = AgentsArray;
		FilteredAgents.RemoveAll(
			[PlayerPawn](const FInputAgent& Agent)
			{
				return Agent.Owner == PlayerPawn;
			});
		if (FilteredAgents.Num() > 0)
		{
			GeoInputComponent->SendForeignInputClientRPC(FilteredAgents);
		}
	}
}

FInputAgent& UGeoInputGameInstanceSubsystem::GetInputAgent(AGeoPawn* GeoPawn)
{
	FInputAgent& InputAgent = InputAgents.FindOrAdd(GeoPawn, FInputAgent());
	if (!IsValid(InputAgent.Owner))
	{
		InputAgent.Owner = GeoPawn;
		GeoPawns.Add(GeoPawn);
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