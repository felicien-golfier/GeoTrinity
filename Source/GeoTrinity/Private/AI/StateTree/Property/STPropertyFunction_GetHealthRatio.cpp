// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AI/StateTree/Property/STPropertyFunction_GetHealthRatio.h"

#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeNodeDescriptionHelpers.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(STPropertyFunction_GetHealthRatio)

#define LOCTEXT_NAMESPACE "StateTree"

void FSTGetHealthRatioPropertyFunction::Execute(FStateTreeExecutionContext& Context) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (!InstanceData.Input)
	{
		InstanceData.Output = 0.f;
		return;
	}

	AActor* Actor = InstanceData.Input ? InstanceData.Input->GetPawn() : nullptr;
	UGeoAbilitySystemComponent* ASC = GeoASLib::GetGeoAscFromActor(Actor);
	if (!ASC)
	{
		InstanceData.Output = 0.f;
		return;
	}

	UGeoAttributeSetBase const* AS =
		Cast<UGeoAttributeSetBase>(ASC->GetAttributeSet(UGeoAttributeSetBase::StaticClass()));
	InstanceData.Output = AS ? AS->GetHealthRatio() : 0.f;
}

#if WITH_EDITOR
FText FSTGetHealthRatioPropertyFunction::GetDescription(FGuid const& ID, FStateTreeDataView InstanceDataView,
														IStateTreeBindingLookup const& BindingLookup,
														EStateTreeNodeFormatting Formatting) const
{
	return UE::StateTree::DescHelpers::GetDescriptionForSingleParameterFunc<FInstanceDataType>(
		LOCTEXT("GetHealthRatio", "GetHealthRatio"), ID, InstanceDataView, BindingLookup, Formatting);
}
#endif

#undef LOCTEXT_NAMESPACE
