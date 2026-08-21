// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Characters/PlayerClassDataAsset.h"

FPlayerClassData const* UPlayerClassDataAsset::GetClassData(EPlayerClass const Class) const
{
	FPlayerClassData const* Found = ClassData.Find(Class);
	ensureMsgf(Found, TEXT("%hs: no ClassData entry for %s on %s"), __FUNCTION__, *UEnum::GetValueAsString(Class),
			   *GetName());
	return Found;
}
