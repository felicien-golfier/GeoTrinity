// Fill out your copyright notice in the Description page of Project Settings.

#include "GeoInputGameInstanceSubsystem.h"

#include "Engine/Engine.h"
#include "HAL/IConsoleManager.h"
#include "InputStep.h"

// Console variable to toggle on-screen debug messages from AddInputBuffer.
// Usage in console: geo.AddInputBuffer.ScreenDebug 1 (enable) or 0 (disable)
static TAutoConsoleVariable<int32> CVarGeoAddInputBufferScreenDebug(TEXT("geo.AddInputBuffer.ScreenDebug"), 0,
	TEXT("When > 0, displays on-screen debug messages from UGeoInputGameInstanceSubsystem::AddInputBuffer."), ECVF_Default);

void UGeoInputGameInstanceSubsystem::AddInputBuffer(const FInputStep& InputStep, AGeoPawn* GeoPawn)
{
	if (CVarGeoAddInputBufferScreenDebug.GetValueOnGameThread() > 0)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Emerald, InputStep.ToString());
			GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Blue, GeoPawn ? GeoPawn->GetName() : TEXT("<null pawn>"));
		}
	}

	FInputAgent& InputAgent = InputAgents.FindOrAdd(GeoPawn, FInputAgent());
	InputAgent.InputSteps.Add(InputStep);
	if (InputAgent.InputSteps.Num() > MaxBufferInputs)
	{
		InputAgent.InputSteps.RemoveAt(MaxBufferInputs);
	}
}
void UGeoInputGameInstanceSubsystem::ProcessAgents(const float DeltaTime)
{
	// Fill it you dumb !
}

void UGeoInputGameInstanceSubsystem::Tick(float DeltaTime)
{
	checkf(IsInitialized(), TEXT("Ticking should have been disabled for an uninitialized subsystem!"));
	ProcessAgents(DeltaTime);
}

// ----------------------------------------------------------------------------------
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
