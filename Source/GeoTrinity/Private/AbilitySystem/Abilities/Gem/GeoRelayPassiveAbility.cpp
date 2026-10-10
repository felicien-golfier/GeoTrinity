// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Gem/GeoRelayPassiveAbility.h"

#include "AbilitySystem/Abilities/Common/GeoDeployAbility.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Deployable/GeoDeployableBase.h"
#include "Characters/Component/GeoDeployableManagerComponent.h"

void UGeoRelayPassiveAbility::BindEvent(UGeoAbilitySystemComponent& HolderASC)
{
	HolderASC.OnDashAiming.AddUObject(this, &ThisClass::SnapDash);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoRelayPassiveAbility::UnbindEvent(UGeoAbilitySystemComponent& HolderASC)
{
	HolderASC.OnDashAiming.RemoveAll(this);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoRelayPassiveAbility::SnapDash(FVector const& Start, bool const bHasDirection, FVector& Direction,
									   float& Distance)
{
	UGeoDeployAbility const* DeployAbility =
		GeoASLib::GetGrantedAbility<UGeoDeployAbility>(*GetGeoAbilitySystemComponentFromActorInfo());
	AActor const* Avatar = GetAvatarActorFromActorInfo();
	UGeoDeployableManagerComponent const* Manager =
		DeployAbility && Avatar ? Avatar->FindComponentByClass<UGeoDeployableManagerComponent>() : nullptr;
	if (Manager)
	{
		float const MinAlignment = FMath::Cos(FMath::DegreesToRadians(SnapHalfAngle));
		FVector const DashDirection2D = Direction.GetSafeNormal2D();
		float NearestDistance = TNumericLimits<float>::Max();
		FVector NearestDirection = Direction;
		bool bSnapped = false;
		for (AGeoDeployableBase const* Deployable : Manager->GetDeployables(DeployAbility->GetDeployableActorClass()))
		{
			FVector ToDeployable = Deployable->GetActorLocation() - Start;
			ToDeployable.Z = 0.f;
			float const DeployableDistance = ToDeployable.Size();
			if (Deployable->IsActive() && !Deployable->IsBlinking() && DeployableDistance > UE_KINDA_SMALL_NUMBER
				&& DeployableDistance < NearestDistance
				&& (!bHasDirection || (DashDirection2D | ToDeployable / DeployableDistance) >= MinAlignment))
			{
				NearestDistance = DeployableDistance;
				NearestDirection = ToDeployable / DeployableDistance;
				bSnapped = true;
			}
		}

		if (bSnapped)
		{
			Direction = NearestDirection;
			Distance = FMath::Min(NearestDistance, Distance * MaxRangeMultiplier);
		}
	}
}
