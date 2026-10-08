// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Gem/GeoGemProfileSave.h"

#include "Algo/Count.h"
#include "Gem/GeoGemCatalog.h"
#include "Tool/UGeoGameplayLibrary.h"

UGeoGemProfileSave* UGeoGemProfileSave::Load(int32 const LocalPlayerIndex)
{
	UGeoGemProfileSave* Profile =
		CastChecked<UGeoGemProfileSave>(GeoLib::LoadUserSaveFile(GetFileName(LocalPlayerIndex), StaticClass()));
	TArray<EPlayerClass> PlayerClasses;
	Profile->Classes.GetKeys(PlayerClasses);
	for (EPlayerClass const PlayerClass : PlayerClasses)
	{
		Profile->FindOrAddClass(PlayerClass);
	}

	return Profile;
}

void UGeoGemProfileSave::Save(int32 const LocalPlayerIndex)
{
	GeoLib::WriteUserSaveFile(this, GetFileName(LocalPlayerIndex));
}

int32 UGeoGemProfileSave::GetFreeCount(FName const GemId, EPlayerClass const PlayerClass) const
{
	return GetOwnedCount(GemId) - static_cast<int32>(Algo::Count(GetLoadout(PlayerClass).Sockets, GemId));
}

int32 UGeoGemProfileSave::GetBreakableCount(FName const GemId) const
{
	int32 MostSlotted = 0;
	for (TPair<EPlayerClass, FGeoGemClassProgress> const& Class : Classes)
	{
		for (FGeoGemBuild const& Build : Class.Value.Builds)
		{
			MostSlotted = FMath::Max(MostSlotted, static_cast<int32>(Algo::Count(Build.Loadout.Sockets, GemId)));
		}
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
		Loadout = Progress->Builds[Progress->ActiveBuild].Loadout;
	}

	Loadout.Sockets.SetNum(GeoGem::GetSockets().Num());
	return Loadout;
}

TArray<FGeoGemBuild> UGeoGemProfileSave::GetBuilds(EPlayerClass const PlayerClass) const
{
	FGeoGemClassProgress const* Progress = Classes.Find(PlayerClass);
	return Progress ? Progress->Builds : TArray<FGeoGemBuild>{FGeoGemBuild()};
}

int32 UGeoGemProfileSave::GetActiveBuild(EPlayerClass const PlayerClass) const
{
	FGeoGemClassProgress const* Progress = Classes.Find(PlayerClass);
	return Progress ? Progress->ActiveBuild : 0;
}

void UGeoGemProfileSave::SetActiveBuild(EPlayerClass const PlayerClass, int32 const BuildIndex)
{
	FGeoGemClassProgress& Progress = FindOrAddClass(PlayerClass);
	if (ensureMsgf(Progress.Builds.IsValidIndex(BuildIndex), TEXT("%hs: no build %d"), __FUNCTION__, BuildIndex))
	{
		Progress.ActiveBuild = BuildIndex;
	}
}

bool UGeoGemProfileSave::AddBuild(EPlayerClass const PlayerClass)
{
	FGeoGemClassProgress& Progress = FindOrAddClass(PlayerClass);
	bool const bCanAdd = Progress.Builds.Num() < GeoGem::MaxBuilds;
	if (bCanAdd)
	{
		Progress.Builds.AddDefaulted_GetRef().Loadout.Sockets.SetNum(GeoGem::GetSockets().Num());
		Progress.ActiveBuild = Progress.Builds.Num() - 1;
	}

	return bCanAdd;
}

void UGeoGemProfileSave::RemoveActiveBuild(EPlayerClass const PlayerClass)
{
	FGeoGemClassProgress& Progress = FindOrAddClass(PlayerClass);
	if (Progress.Builds.Num() > 1)
	{
		Progress.Builds.RemoveAt(Progress.ActiveBuild);
		Progress.ActiveBuild = FMath::Max(0, Progress.ActiveBuild - 1);
	}
}

void UGeoGemProfileSave::RenameActiveBuild(EPlayerClass const PlayerClass, FString const& Name)
{
	FGeoGemClassProgress& Progress = FindOrAddClass(PlayerClass);
	Progress.Builds[Progress.ActiveBuild].Name = Name.TrimStartAndEnd().Left(GeoGem::MaxBuildNameLength);
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
		GetActiveSockets(PlayerClass)[SocketIndex] = GemId;
	}

	return bCanEquip;
}

void UGeoGemProfileSave::Unequip(EPlayerClass const PlayerClass, int32 const SocketIndex)
{
	TArray<FName>& Sockets = GetActiveSockets(PlayerClass);
	if (ensureMsgf(Sockets.IsValidIndex(SocketIndex), TEXT("%hs: no socket %d"), __FUNCTION__, SocketIndex))
	{
		Sockets[SocketIndex] = NAME_None;
	}
}

int32 UGeoGemProfileSave::FillEmptySockets(UGeoGemCatalog const& Catalog, EPlayerClass const PlayerClass,
										   FName const GemId)
{
	int32 Filled = 0;
	TArray<FName> const& Sockets = GetActiveSockets(PlayerClass);
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
	if (Progress.Builds.IsEmpty())
	{
		Progress.Builds.Add({FString(), MoveTemp(Progress.Loadout)});
	}

	for (FGeoGemBuild& Build : Progress.Builds)
	{
		Build.Loadout.Sockets.SetNum(GeoGem::GetSockets().Num());
	}

	return Progress;
}

TArray<FName>& UGeoGemProfileSave::GetActiveSockets(EPlayerClass const PlayerClass)
{
	FGeoGemClassProgress& Progress = FindOrAddClass(PlayerClass);
	return Progress.Builds[Progress.ActiveBuild].Loadout.Sockets;
}
