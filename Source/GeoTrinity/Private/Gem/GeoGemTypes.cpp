// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Gem/GeoGemTypes.h"

FText FGeoGemInfo::GetMagnitudeText(int32 const Count) const
{
	if (MagnitudePerGem == 0.f)
	{
		return FText::GetEmpty();
	}

	FNumberFormattingOptions Options;
	Options.SetAlwaysSign(true).SetMaximumFractionalDigits(2);
	return FText::Format(INVTEXT("{0}%"), FText::AsNumber(MagnitudePerGem * Count * 100.f, &Options));
}

FText FGeoGemInfo::GetEffectText(int32 const Count) const
{
	return MagnitudePerGem == 0.f ? Effect : FText::Format(INVTEXT("{0} {1}"), Effect, GetMagnitudeText(Count));
}

TArray<FGeoGemSocket> const& GeoGem::GetSockets()
{
	static TArray<FGeoGemSocket> const Sockets = []
	{
		struct FStep
		{
			int32 Cores;
			int32 Chips;
			int32 Cuts;
			int32 Prisms;
		};
		constexpr FStep ClusterSteps[] = {
			{0, 2, 0, 0}, {0, 2, 1, 0}, {0, 2, 1, 0}, {0, 0, 1, 1}, {0, 2, 1, 0}, {0, 1, 0, 1}, {1, 1, 1, 1},
		};
		constexpr int32 StepCount = UE_ARRAY_COUNT(ClusterSteps);
		constexpr int32 ClusterCount = 3;

		TArray<FGeoGemSocket> Result;
		for (int32 Cluster = 0; Cluster < ClusterCount; ++Cluster)
		{
			for (int32 Step = 0; Step < StepCount; ++Step)
			{
				int32 const UnlockLevel = FMath::Min(Cluster * StepCount + Step + 1, MaxClassLevel);
				FStep const& Opened = ClusterSteps[Step];
				auto AddSockets = [&Result, UnlockLevel](EGeoGemTier Tier, int32 Count)
				{
					for (int32 i = 0; i < Count; ++i)
					{
						Result.Add({Tier, UnlockLevel});
					}
				};
				AddSockets(EGeoGemTier::Core, Opened.Cores);
				AddSockets(EGeoGemTier::Chip, Opened.Chips);
				AddSockets(EGeoGemTier::Cut, Opened.Cuts);
				AddSockets(EGeoGemTier::Prism, Opened.Prisms);
			}
		}
		return Result;
	}();
	return Sockets;
}
