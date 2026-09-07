// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Actor/Deployable/GeoDeployableFXComponent.h"

#include "Actor/Deployable/GeoDeployableBase.h"

AActor* UGeoDeployableFXComponent::GetFXInstigator() const
{
	return GetOwner<AGeoDeployableBase>()->GetData()->Instigator;
}

// ---------------------------------------------------------------------------------------------------------------------
int32 UGeoDeployableFXComponent::GetAbilityLevel() const
{
	return GetOwner<AGeoDeployableBase>()->GetData()->Level;
}
