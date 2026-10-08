// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Gem/GeoGemProfileSave.h"

#include "Algo/Count.h"
#include "Gem/GeoGemCatalog.h"
#include "Tool/UGeoGameplayLibrary.h"

UGeoGemProfileSave* UGeoGemProfileSave::Load(int32 const LocalPlayerIndex)
{
	return CastChecked<UGeoGemProfileSave>(
		GeoLib::LoadUserSaveFile(GetFileName(LocalPlayerIndex), StaticClass()));
}

void UGeoGemProfileSave::Save(int32 const LocalPlayerIndex)
{
	GeoLib::WriteUserSaveFile(this, GetFileName(LocalPlayerIndex));
}

int32 UGeoGemProfileSave::GetFreeCount(FName const GemId, EPlayerClass const PlayerClass) const
{
	return GetOwnedCount(GemId) - GetSlottedCount(GemId, PlayerClass);
}

int32 UGeoGemProfileSave::GetBreakableCount(FName const GemId) const
{
	int32 MostSlotted = 0;
	for (TPair<EPlayerClass, FGeoGemClassProgress> const& Class : Classes)
	{
		MostSlotted = FMath::Max(MostSlotted, GetSlottedCount(GemId, Class.Key));
	}

	return GetOwnedCount(GemId) - MostSlotted;
}

int32 UGeoGemProfileSave::GetClassLevel(EPlayerClass const PlayerClass) const
{
	FGeoGemClassProgress const* Progress = Classes.Find(PlayerClass);
	return Progress ? Progress->Level : 1;
}

int32 UGeoGemProfileSave::GetClassXp(EPlayerClass const PlayerClass) const
{
	FGeoGemClassProgress const* Progress = Classes.Find(PlayerClass);
	return Progress ? Progress->Xp : 0;
}

FGeoGemLoadout UGeoGemProfileSave::GetLoadout(EPlayerClass const PlayerClass) const
{
	FGeoGemLoadout Loadout;
	if (FGeoGemClassProgress const* Progress = Classes.Find(PlayerClass))
	{
		Loadout = Progress->Loadout;
	}

	Loadout.Sockets.SetNum(GeoGem::GetSockets().Num());
	return Loadout;
}

void UGeoGemProfileSave::AddGems(FName const GemId, int32 const Count)
{
	OwnedGems.FindOrAdd(GemId) += Count;
}

void UGeoGemProfileSave::SetClassLevel(EPlayerClass const PlayerClass, int32 const Level)
{
	FGeoGemClassProgress& Progress = FindOrAddClass(PlayerClass);
	Progress.Level = FMath::Clamp(Level, 1, GeoGem::MaxClassLevel);
	Progress.Xp = 0;
}

int32 UGeoGemProfileSave::AddClassXp(UGeoGemCatalog const& Catalog, EPlayerClass const PlayerClass, int32 const Xp)
{
	if (!ensureMsgf(Catalog.XpPerLevel > 0, TEXT("%hs: %s has no XpPerLevel"), __FUNCTION__, *Catalog.GetName()))
	{
		return 0;
	}

	FGeoGemClassProgress& Progress = FindOrAddClass(PlayerClass);
	int32 const StartLevel = Progress.Level;
	Progress.Xp += Xp;
	while (Progress.Xp >= Catalog.XpPerLevel && Progress.Level < GeoGem::MaxClassLevel)
	{
		Progress.Xp -= Catalog.XpPerLevel;
		++Progress.Level;
	}
	if (Progress.Level == GeoGem::MaxClassLevel)
	{
		Progress.Xp = 0;
	}

	return Progress.Level - StartLevel;
}

bool UGeoGemProfileSave::CanEquip(UGeoGemCatalog const& Catalog, EPlayerClass const PlayerClass,
								  int32 const SocketIndex, FName const GemId) const
{
	TArray<FGeoGemSocket> const& Sockets = GeoGem::GetSockets();
	if (!ensureMsgf(Sockets.IsValidIndex(SocketIndex), TEXT("%hs: no socket %d"), __FUNCTION__, SocketIndex))
	{
		return false;
	}

	TOptional<EGeoGemTier> const Tier = Catalog.FindTier(GemId);
	TArray<FName> const Slotted = GetLoadout(PlayerClass).Sockets;
	FGeoGemSocket const& Socket = Sockets[SocketIndex];
	return Tier == Socket.Tier && GetClassLevel(PlayerClass) >= Socket.UnlockLevel && Slotted[SocketIndex] != GemId
		&& GetFreeCount(GemId, PlayerClass) > 0 && (Tier != EGeoGemTier::Core || !Slotted.Contains(GemId));
}

bool UGeoGemProfileSave::Equip(UGeoGemCatalog const& Catalog, EPlayerClass const PlayerClass, int32 const SocketIndex,
							   FName const GemId)
{
	bool const bCanEquip = CanEquip(Catalog, PlayerClass, SocketIndex, GemId);
	if (bCanEquip)
	{
		FindOrAddClass(PlayerClass).Loadout.Sockets[SocketIndex] = GemId;
	}

	return bCanEquip;
}

void UGeoGemProfileSave::Unequip(EPlayerClass const PlayerClass, int32 const SocketIndex)
{
	TArray<FName>& Sockets = FindOrAddClass(PlayerClass).Loadout.Sockets;
	if (ensureMsgf(Sockets.IsValidIndex(SocketIndex), TEXT("%hs: no socket %d"), __FUNCTION__, SocketIndex))
	{
		Sockets[SocketIndex] = NAME_None;
	}
}

int32 UGeoGemProfileSave::FillEmptySockets(UGeoGemCatalog const& Catalog, EPlayerClass const PlayerClass,
										   FName const GemId)
{
	int32 Filled = 0;
	TArray<FName> const& Sockets = FindOrAddClass(PlayerClass).Loadout.Sockets;
	for (int32 SocketIndex = 0; SocketIndex < Sockets.Num(); ++SocketIndex)
	{
		if (Sockets[SocketIndex].IsNone() && Equip(Catalog, PlayerClass, SocketIndex, GemId))
		{
			++Filled;
		}
	}

	return Filled;
}

int32 UGeoGemProfileSave::BreakDown(UGeoGemCatalog const& Catalog, FName const GemId, int32 const Quantity)
{
	TOptional<EGeoGemTier> const Tier = Catalog.FindTier(GemId);
	int32 const Count = FMath::Clamp(Quantity, 0, GetBreakableCount(GemId));
	if (!Tier || Count == 0)
	{
		return 0;
	}

	int32 const Gained = Count * Catalog.GetBreakDownShards(*Tier);
	Shards += Gained;
	int32& Owned = OwnedGems.FindChecked(GemId);
	Owned -= Count;
	if (Owned == 0)
	{
		OwnedGems.Remove(GemId);
	}

	return Gained;
}

int32 UGeoGemProfileSave::BreakDownTier(UGeoGemCatalog const& Catalog, EGeoGemTier const Tier)
{
	TArray<FName> GemIds;
	OwnedGems.GetKeys(GemIds);

	int32 Gained = 0;
	for (FName const GemId : GemIds)
	{
		if (Catalog.FindTier(GemId) == Tier)
		{
			Gained += BreakDown(Catalog, GemId, GetBreakableCount(GemId));
		}
	}

	return Gained;
}

bool UGeoGemProfileSave::Craft(UGeoGemCatalog const& Catalog, FName const GemId, int32 const Quantity)
{
	TOptional<EGeoGemTier> const Tier = Catalog.FindTier(GemId);
	if (!ensureMsgf(Tier, TEXT("%hs: no gem %s in %s"), __FUNCTION__, *GemId.ToString(), *Catalog.GetName()))
	{
		return false;
	}

	int32 const Cost = Catalog.GetCraftCost(*Tier) * Quantity;
	bool const bCanCraft = Quantity > 0 && Cost <= Shards;
	if (bCanCraft)
	{
		Shards -= Cost;
		AddGems(GemId, Quantity);
	}

	return bCanCraft;
}

FString UGeoGemProfileSave::GetFileName(int32 const LocalPlayerIndex)
{
	return LocalPlayerIndex == 0 ? TEXT("GeoGems.sav") : FString::Printf(TEXT("GeoGems_P%d.sav"), LocalPlayerIndex + 1);
}

FGeoGemClassProgress& UGeoGemProfileSave::FindOrAddClass(EPlayerClass const PlayerClass)
{
	FGeoGemClassProgress& Progress = Classes.FindOrAdd(PlayerClass);
	Progress.Loadout.Sockets.SetNum(GeoGem::GetSockets().Num());
	return Progress;
}

int32 UGeoGemProfileSave::GetSlottedCount(FName const GemId, EPlayerClass const PlayerClass) const
{
	FGeoGemClassProgress const* Progress = Classes.Find(PlayerClass);
	return Progress ? static_cast<int32>(Algo::Count(Progress->Loadout.Sockets, GemId)) : 0;
}
