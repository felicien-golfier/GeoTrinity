// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Tool/Visual/GeoNiagaraParams.h"

#include "NiagaraComponent.h"

void GeoNiagaraParams::SetMeaningColors(UNiagaraComponent* const Component, TArray<FLinearColor> const& Colors)
{
	if (!ensureMsgf(Colors.Num() <= GeoColor::MaxMeaningColorCount,
					TEXT("GeoNiagaraParams: %d colours, a colour pattern takes at most %d"), Colors.Num(),
					GeoColor::MaxMeaningColorCount))
	{
		return;
	}

	for (int32 Index = 0; Index < Colors.Num(); ++Index)
	{
		Component->SetVariableLinearColor(MeaningColors[Index], Colors[Index]);
	}

	Component->SetVariableFloat(ColorCount, Colors.Num());
}
