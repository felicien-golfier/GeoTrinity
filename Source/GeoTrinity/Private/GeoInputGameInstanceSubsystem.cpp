// Fill out your copyright notice in the Description page of Project Settings.

#include "GeoInputGameInstanceSubsystem.h"

#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "GeoInputComponent.h"
#include "GeoPlayerController.h"
#include "GeoStateSubsystem.h"
#include "GeoTrinity/GeoTrinity.h"
#include "HAL/IConsoleManager.h"
#include "InputStep.h"

// Console variable to toggle on-screen debug messages from AddInputBuffer.
// Usage in console: geo.AddInputBuffer.ScreenDebug 1 (enable) or 0 (disable)
static TAutoConsoleVariable<int32> CVarGeoAddInputBufferScreenDebug(TEXT("geo.AddInputBuffer.ScreenDebug"), 0,
	TEXT("When > 0, displays on-screen debug messages from UGeoInputGameInstanceSubsystem::AddInputBuffer."),
	ECVF_Default);

void UGeoInputGameInstanceSubsystem::ClientUpdateInputAgents(const TArray<FInputAgent>& UpdatedInputAgents)
{
	// ONLY CALLED ON CLIENT !
	for (const FInputAgent& InputAgent : UpdatedInputAgents)
	{
		for (FInputStep Step : InputAgent.InputSteps)
		{
			AddNewInput(Step, InputAgent.Owner.Get());
		}
	}
}

void UGeoInputGameInstanceSubsystem::AddNewInput(const FInputStep& InputStep, AGeoPawn* GeoPawn)
{

	if (CVarGeoAddInputBufferScreenDebug.GetValueOnGameThread() > 0)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Emerald, InputStep.ToString());
			GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Blue, GeoPawn ? GeoPawn->GetName() : TEXT("<null pawn>"));
		}
	}

	if (FInputAgent* ExistingAgent = NewInputAgents.FindByKey(GeoPawn))
	{
		ExistingAgent->InputSteps.Add(InputStep);
	}
	else
	{
		NewInputAgents.Add(FInputAgent({InputStep}, GeoPawn));
	}
}

void UGeoInputGameInstanceSubsystem::ProcessAgents(const float DeltaTime)
{
	// Clean up invalid pawns
	InputAgentsHistory.RemoveAll(
		[](const FInputAgent& Agent)
		{
			return !Agent.Owner.IsValid();
		});

	// When no new input, replay last input for each pawn
	if (NewInputAgents.Num() == 0)
	{
		for (FInputAgent& Agent : InputAgentsHistory)
		{
			if (Agent.Owner.IsValid() && Agent.InputSteps.Num() > 0)
			{
				// Use local deltatime when no input received.
				Agent.Owner->GetGeoInputComponent()->ProcessInput(Agent.InputSteps.Last(), DeltaTime);
			}
		}

		return;
	}

	// Find out what's the earliest new arrived time across all agents
	FGeoTime FurthestPastServerTime = FGeoTime::GetAccurateRealTime();
	for (FInputAgent& InputAgent : NewInputAgents)
	{
		for (const FInputStep& InputStep : InputAgent.InputSteps)
		{
			if (InputStep.Time < FurthestPastServerTime)
			{
				FurthestPastServerTime = InputStep.Time;
			}
		}

		FInputAgent& ExistingAgent = GetInputAgent(InputAgent.Owner.Get());
		// Merge new inputs into existing agent
		ExistingAgent.InputSteps.Append(InputAgent.InputSteps);

		// sort Inputs by time
		ExistingAgent.InputSteps.Sort(
			[](const FInputStep& A, const FInputStep& B)
			{
				return A.Time < B.Time;
			});

		// Remove old inputs
		if (ExistingAgent.InputSteps.Num() > MaxBufferInputs)
		{
			ExistingAgent.InputSteps.RemoveAt(0, ExistingAgent.InputSteps.Num() - MaxBufferInputs);
		}
	}

	UGeoStateSubsystem::GetInstance(GetWorld())->RollBackToTime(FurthestPastServerTime);

	// Replay all inputs in deterministic order (array order is preserved)
	for (FInputAgent& Agent : InputAgentsHistory)
	{
		if (!Agent.Owner.IsValid())
		{
			continue;
		}

		for (const FInputStep& InputStep : Agent.InputSteps)
		{
			if (InputStep.Time <= FurthestPastServerTime)
			{
				continue;
			}

			Agent.Owner->GetGeoInputComponent()->ProcessInput(InputStep);
		}
	}
}

void UGeoInputGameInstanceSubsystem::UpdateClients()
{
	// For each known recipient pawn, send only inputs that arrived during the last frame
	for (const FInputAgent& InputAgent : InputAgentsHistory)
	{
		AGeoPawn* GeoPawn = InputAgent.Owner.Get();
		if (!IsValid(GeoPawn))
		{
			continue;
		}

		UGeoInputComponent* GeoInputComponent = GeoPawn->GetGeoInputComponent();
		if (!IsValid(GeoInputComponent))
		{
			continue;
		}

		TArray<FInputAgent> DeltaAgents;
		DeltaAgents.Reserve(NewInputAgents.Num());

		for (const FInputAgent SourceAgent : NewInputAgents)
		{
			AGeoPawn* SourcePawn = SourceAgent.Owner.Get();
			if (!IsValid(SourcePawn) || SourcePawn == GeoPawn)
			{
				continue;
			}

			if (SourceAgent.InputSteps.Num() > 0)
			{
				DeltaAgents.Add(SourceAgent);
			}
		}

		if (DeltaAgents.Num() > 0)
		{
			GeoInputComponent->SendForeignInputClientRPC(DeltaAgents);
		}
	}
}

FInputAgent& UGeoInputGameInstanceSubsystem::GetInputAgent(AGeoPawn* GeoPawn)
{
	if (FInputAgent* InputAgent = InputAgentsHistory.FindByKey(GeoPawn))
	{
		return *InputAgent;
	}

	FInputAgent NewAgent;
	NewAgent.Owner = GeoPawn;

	// Binary search to find insertion point (array must be sorted)
	const uint32 NewPawnID = GeoPawn->GetUniqueID();
	int32 InsertIndex = Algo::LowerBound(InputAgentsHistory, NewPawnID,
		[](const FInputAgent& Agent, uint32 ID)
		{
			return Agent.Owner.IsValid() && Agent.Owner->GetUniqueID() < ID;
		});

	InputAgentsHistory.Insert(NewAgent, InsertIndex);
	return InputAgentsHistory[InsertIndex];
}

void UGeoInputGameInstanceSubsystem::SetServerTimeOffset(AGeoPlayerController* GeoPlayerController,
	float ServerTimeOffset)
{
	ServerTimeOffsets.Add(GeoPlayerController, ServerTimeOffset);
}

bool UGeoInputGameInstanceSubsystem::HasLocalServerTimeOffset(const UWorld* World)
{
	return GetInstance(World)->HasLocalServerTimeOffset();
}

bool UGeoInputGameInstanceSubsystem::HasLocalServerTimeOffset() const
{
	checkf(GetWorld()->GetNetMode() == NM_Client, TEXT("Only valid on client !"));
	return ServerTimeOffsets.Num() > 0;
}
bool UGeoInputGameInstanceSubsystem::HasServerTimeOffset(const AGeoPlayerController* GeoPlayerController) const
{
	return ServerTimeOffsets.Contains(GeoPlayerController);
}

FGeoTime UGeoInputGameInstanceSubsystem::GetServerTime(const AGeoPlayerController* GeoPlayerController)
{
	checkf(IsValid(GeoPlayerController), TEXT("Invalid GeoPlayerController !"));
	checkf(ServerTimeOffsets.Num() > 0,
		TEXT("No ServerTimeOffset, don't call the function without check HasServerTimeOffset !"));
	return FGeoTime::GetAccurateRealTime() + ServerTimeOffsets[GeoPlayerController];
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

	// We need the content for both ProcessAgents and UpdateClients, so reset it at end tick
	NewInputAgents.Empty();
}

UWorld* UGeoInputGameInstanceSubsystem::GetTickableGameObjectWorld() const
{
	return GetWorld();
}

ETickableTickType UGeoInputGameInstanceSubsystem::GetTickableTickType() const
{
	// If this is a template or has not been initialized yet, set to never tick and it will be enabled when it is
	// initialized
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
	// This should never be false because Initialize should always be called before the first tick and Deinitialize
	// cancels the tick
	ensureMsgf(bInitialized,
		TEXT("Tickable subsystem %s tried to tick when not initialized! Check for missing Super call"), *GetFullName());

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