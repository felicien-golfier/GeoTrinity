#pragma once

#include "PlayerClassTypes.generated.h"

UENUM(BlueprintType)
enum class EPlayerClass : uint8
{
	None = 0 UMETA(DisplayName = "Not Set"), // Used as first
	Triangle = 1, // DPS
	Circle = 2, // Healer
	Square = 3, // Tank
	All = 4 // Used as last
};
