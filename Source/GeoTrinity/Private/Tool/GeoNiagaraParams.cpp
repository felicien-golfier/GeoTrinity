// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Tool/GeoNiagaraParams.h"

#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

void GeoNiagaraParams::ApplySwappableAsset(UNiagaraComponent* const Component, FBeamVfxAssetSet const& Assets,
										   bool const bWantIndicator)
{
	UNiagaraSystem* const DesiredAsset = Assets.GetDesiredAsset(bWantIndicator);
	if (Component && DesiredAsset && Component->GetAsset() != DesiredAsset)
	{
		Component->SetAsset(DesiredAsset);
	}
}
