#pragma once
#include "CoreMinimal.h"

struct FGeoBox
{
	FVector2D Position;
	FVector2D Size;

	FGeoBox() {}
	FGeoBox( FVector2D InPos, FVector2D InSize ) : Position( InPos ), Size( InSize ) {}

	bool Overlaps( const FGeoBox& Other ) const;
};
