#pragma once

#include "PlayerClassTypes.generated.h"

UENUM(BlueprintType)
enum class EPlayerClass : uint8
{
	None = 0 UMETA(DisplayName = "Not Set"),
	Triangle, // DPS
	Circle, // Healer
	Square // Tank
};
