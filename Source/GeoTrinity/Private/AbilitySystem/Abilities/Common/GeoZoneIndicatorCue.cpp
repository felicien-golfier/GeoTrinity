// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Common/GeoZoneIndicatorCue.h"

#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Characters/Component/GeoIndicatorComponent.h"
#include "Tool/UGeoGameplayLibrary.h"

bool UGeoZoneIndicatorCue::OnExecute_Implementation(AActor* MyTarget, FGameplayCueParameters const& Parameters) const
{
	if (!ensureMsgf(IsValid(MyTarget), TEXT("%hs: no target to draw the telegraph for"), __FUNCTION__))
	{
		return false;
	}

	AActor const* const SourceActor = Cast<AActor>(Parameters.SourceObject.Get());
	USceneComponent* const AttachParent = SourceActor ? SourceActor->GetRootComponent() : nullptr;

	FGeoIndicatorState State;
	State.Location = AttachParent ? AttachParent->GetComponentTransform().InverseTransformPosition(Parameters.Location)
								  : FVector(Parameters.Location);
	State.Size.X = Parameters.RawMagnitude;
	State.Colors = GeoASLib::GetCueMeaningColors(Parameters);
	State.StartServerTime = GeoLib::GetServerTime(MyTarget, true) - Parameters.Normal.Y;
	State.Duration = Parameters.Normal.X;
	UGeoIndicatorComponent::SpawnIndicator(MyTarget, State, AttachParent);
	return false;
}
