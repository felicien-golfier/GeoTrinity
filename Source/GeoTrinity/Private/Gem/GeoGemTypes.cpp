// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Gem/GeoGemTypes.h"

#include "AbilitySystem/Abilities/Gem/GeoCorePassiveAbility.h"
#include "AbilitySystem/Data/AbilityInfo.h"

FText FGeoGemInfo::GetMagnitudeText(float const Copies) const
{
	if (MagnitudePerGem == 0.f)
	{
		return FText::GetEmpty();
	}

	FNumberFormattingOptions Options;
	Options.SetAlwaysSign(true).SetMaximumFractionalDigits(2);
	if (Operation == EGeoGemOperation::Flat)
	{
		return FText::AsNumber(MagnitudePerGem * Copies, &Options);
	}

	return FText::Format(INVTEXT("{0}%"), FText::AsNumber(MagnitudePerGem * Copies * 100.f, &Options));
}

FText FGeoGemInfo::GetEffectText(float const Copies) const
{
	// {Token}s read the Core's passive ability, else UGameDataSettings (Leverage has no ability).
	UGeoGameplayAbility const* const Source = GrantedAbility
		? GrantedAbility->GetDefaultObject<UGeoGameplayAbility>()
		: GetDefault<UGeoGameplayAbility>();
	FText const ResolvedEffect =
		FText::FromString(GeoDescription::ResolveTokens(Effect.ToString(), *Source, 1, false));
	return MagnitudePerGem == 0.f ? ResolvedEffect
								  : FText::Format(INVTEXT("{0} {1}"), ResolvedEffect, GetMagnitudeText(Copies));
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
