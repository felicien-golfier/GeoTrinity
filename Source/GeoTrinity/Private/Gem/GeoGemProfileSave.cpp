// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Gem/GeoGemProfileSave.h"

#include "Algo/AnyOf.h"
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

	Profile->SyncEchoWithClassLevels();
	return Profile;
}

void UGeoGemProfileSave::Save(int32 const LocalPlayerIndex)
{
	GeoLib::WriteUserSaveFile(this, GetFileName(LocalPlayerIndex));
}

int32 UGeoGemProfileSave::GetFreeCount(FName const GemId, EPlayerClass const PlayerClass) const
{
	return GetOwnedCount(GemId) - CountSlotted(GetLoadout(PlayerClass), GemId);
}

int32 UGeoGemProfileSave::GetBreakableCount(FName const GemId) const
{
	int32 MostSlotted = 0;
	for (TPair<EPlayerClass, FGeoGemClassProgress> const& Class : Classes)
	{
		for (FGeoGemBuild const& Build : Class.Value.Builds)
		{
			MostSlotted = FMath::Max(MostSlotted, CountSlotted(Build.Loadout, GemId));
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

FName UGeoGemProfileSave::GetEcho(EPlayerClass const PlayerClass) const
{
	return GetLoadout(PlayerClass).Echo;
}

int32 UGeoGemProfileSave::GetEchoLevel() const
{
	return EchoLevel;
}

int32 UGeoGemProfileSave::GetEchoXp() const
{
	return EchoXp;
}

float UGeoGemProfileSave::GetXpProgress(UGeoGemCatalog const& Catalog, EPlayerClass const PlayerClass,
										int32& OutXpTaken, int32& OutXpNeeded) const
{
	bool const bMaxLevel = GetClassLevel(PlayerClass) >= GeoGem::MaxClassLevel;
	OutXpTaken = bMaxLevel ? EchoXp : GetClassXp(PlayerClass);
	OutXpNeeded = bMaxLevel ? Catalog.GetEchoXpForLevel(EchoLevel + 1) : Catalog.XpPerLevel;
	return bMaxLevel && EchoLevel >= Catalog.EchoMaxLevel ? 1.f
														  : static_cast<float>(OutXpTaken) / FMath::Max(OutXpNeeded, 1);
}

EGeoGemRefusal UGeoGemProfileSave::GetEchoRefusal(UGeoGemCatalog const& Catalog, EPlayerClass const PlayerClass,
												  FName const GemId) const
{
	TOptional<EGeoGemTier> const Tier = Catalog.FindTier(GemId);
	return EchoLevel < 1					 ? EGeoGemRefusal::EchoLocked
		: Tier == EGeoGemTier::Core			 ? EGeoGemRefusal::EchoCore
		: GetEcho(PlayerClass) == GemId		 ? EGeoGemRefusal::AlreadyEcho
		: !Tier || GetOwnedCount(GemId) <= 0 ? EGeoGemRefusal::NoCopy
											 : EGeoGemRefusal::None;
}

bool UGeoGemProfileSave::CanEcho(UGeoGemCatalog const& Catalog, EPlayerClass const PlayerClass, FName const GemId) const
{
	return GetEchoRefusal(Catalog, PlayerClass, GemId) == EGeoGemRefusal::None;
}

bool UGeoGemProfileSave::SetEcho(UGeoGemCatalog const& Catalog, EPlayerClass const PlayerClass, FName const GemId)
{
	bool const bCanEcho = CanEcho(Catalog, PlayerClass, GemId);
	if (bCanEcho)
	{
		FGeoGemLoadout& Loadout = GetActiveLoadout(PlayerClass);
		Loadout.Echo = GemId;
		for (FName& Slotted : Loadout.Sockets)
		{
			if (Slotted == GemId)
			{
				Slotted = NAME_None;
			}
		}
	}

	return bCanEcho;
}

void UGeoGemProfileSave::ClearEcho(EPlayerClass const PlayerClass)
{
	GetActiveLoadout(PlayerClass).Echo = NAME_None;
}

FGeoGemLoadout UGeoGemProfileSave::GetLoadout(EPlayerClass const PlayerClass) const
{
	FGeoGemLoadout Loadout;
	if (FGeoGemClassProgress const* Progress = Classes.Find(PlayerClass))
	{
		Loadout = Progress->Builds[Progress->ActiveBuild].Loadout;
	}

	Loadout.Sockets.SetNum(GeoGem::GetSockets().Num());
	Loadout.Level = GetClassLevel(PlayerClass);
	Loadout.EchoLevel = EchoLevel;
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

void UGeoGemProfileSave::AddShards(int32 const Count)
{
	Shards += Count;
}

void UGeoGemProfileSave::SetClassLevel(EPlayerClass const PlayerClass, int32 const Level)
{
	FGeoGemClassProgress& Progress = FindOrAddClass(PlayerClass);
	Progress.Level = FMath::Clamp(Level, 1, GeoGem::MaxClassLevel);
	Progress.Xp = 0;

	TArray<FGeoGemSocket> const& Sockets = GeoGem::GetSockets();
	for (FGeoGemBuild& Build : Progress.Builds)
	{
		for (int32 SocketIndex = 0; SocketIndex < Sockets.Num(); ++SocketIndex)
		{
			if (Sockets[SocketIndex].UnlockLevel > Progress.Level)
			{
				Build.Loadout.Sockets[SocketIndex] = NAME_None;
			}
		}
	}

	SyncEchoWithClassLevels();
}

#if !UE_BUILD_SHIPPING
void UGeoGemProfileSave::SetEchoLevel(UGeoGemCatalog const& Catalog, int32 const Level)
{
	if (Level > 0 && !HasClassAtEchoUnlockLevel())
	{
		SetClassLevel(EPlayerClass::Triangle, GeoGem::EchoUnlockLevel);
	}

	EchoLevel = FMath::Clamp(Level, 0, Catalog.EchoMaxLevel);
	EchoXp = 0;
}
#endif

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
		SyncEchoWithClassLevels();
		EchoXp += Progress.Xp;
		Progress.Xp = 0;
		int32 NextEchoCost = Catalog.GetEchoXpForLevel(EchoLevel + 1);
		while (EchoLevel < Catalog.EchoMaxLevel && EchoXp >= NextEchoCost)
		{
			EchoXp -= NextEchoCost;
			++EchoLevel;
			NextEchoCost = Catalog.GetEchoXpForLevel(EchoLevel + 1);
		}
	}

	return Progress.Level - StartLevel;
}

EGeoGemRefusal UGeoGemProfileSave::GetEquipRefusal(UGeoGemCatalog const& Catalog, EPlayerClass const PlayerClass,
												   int32 const SocketIndex, FName const GemId) const
{
	TArray<FGeoGemSocket> const& Sockets = GeoGem::GetSockets();
	if (!ensureMsgf(Sockets.IsValidIndex(SocketIndex), TEXT("%hs: no socket %d"), __FUNCTION__, SocketIndex))
	{
		return EGeoGemRefusal::WrongTier;
	}

	TOptional<EGeoGemTier> const Tier = Catalog.FindTier(GemId);
	TArray<FName> const Slotted = GetLoadout(PlayerClass).Sockets;
	FGeoGemSocket const& Socket = Sockets[SocketIndex];
	bool const bCopyMoves = GetEcho(PlayerClass) == GemId || (Tier == EGeoGemTier::Core && Slotted.Contains(GemId));
	return Tier != Socket.Tier								   ? EGeoGemRefusal::WrongTier
		: GetClassLevel(PlayerClass) < Socket.UnlockLevel	   ? EGeoGemRefusal::Locked
		: Slotted[SocketIndex] == GemId						   ? EGeoGemRefusal::AlreadyThere
		: !bCopyMoves && GetFreeCount(GemId, PlayerClass) <= 0 ? EGeoGemRefusal::NoFreeCopy
															   : EGeoGemRefusal::None;
}

bool UGeoGemProfileSave::CanEquip(UGeoGemCatalog const& Catalog, EPlayerClass const PlayerClass,
								  int32 const SocketIndex, FName const GemId) const
{
	return GetEquipRefusal(Catalog, PlayerClass, SocketIndex, GemId) == EGeoGemRefusal::None;
}

bool UGeoGemProfileSave::Equip(UGeoGemCatalog const& Catalog, EPlayerClass const PlayerClass, int32 const SocketIndex,
							   FName const GemId)
{
	bool const bCanEquip = CanEquip(Catalog, PlayerClass, SocketIndex, GemId);
	if (bCanEquip)
	{
		FGeoGemLoadout& Loadout = GetActiveLoadout(PlayerClass);
		if (Loadout.Echo == GemId)
		{
			Loadout.Echo = NAME_None;
		}
		else if (Catalog.FindTier(GemId) == EGeoGemTier::Core)
		{
			int32 const HeldIndex = Loadout.Sockets.Find(GemId);
			if (HeldIndex != INDEX_NONE)
			{
				Loadout.Sockets[HeldIndex] = NAME_None;
			}
		}

		Loadout.Sockets[SocketIndex] = GemId;
		OnGemEquipped.Broadcast(PlayerClass, SocketIndex, GemId);
	}

	return bCanEquip;
}

void UGeoGemProfileSave::Unequip(EPlayerClass const PlayerClass, int32 const SocketIndex)
{
	TArray<FName>& Sockets = GetActiveLoadout(PlayerClass).Sockets;
	if (ensureMsgf(Sockets.IsValidIndex(SocketIndex), TEXT("%hs: no socket %d"), __FUNCTION__, SocketIndex))
	{
		Sockets[SocketIndex] = NAME_None;
	}
}

void UGeoGemProfileSave::UnequipTier(UGeoGemCatalog const& Catalog, EPlayerClass const PlayerClass,
									 EGeoGemTier const Tier)
{
	FGeoGemLoadout& Loadout = GetActiveLoadout(PlayerClass);
	TArray<FGeoGemSocket> const& Sockets = GeoGem::GetSockets();
	for (int32 SocketIndex = 0; SocketIndex < Sockets.Num(); ++SocketIndex)
	{
		if (Sockets[SocketIndex].Tier == Tier)
		{
			Loadout.Sockets[SocketIndex] = NAME_None;
		}
	}

	if (Catalog.FindTier(Loadout.Echo) == Tier)
	{
		Loadout.Echo = NAME_None;
	}
}

int32 UGeoGemProfileSave::FillSockets(UGeoGemCatalog const& Catalog, EPlayerClass const PlayerClass, FName const GemId)
{
	bool const bCore = Catalog.FindTier(GemId) == EGeoGemTier::Core;
	bool const bHeld = GetActiveLoadout(PlayerClass).Sockets.Contains(GemId);
	int32 const MaxFilled = bCore ? (bHeld ? 0 : 1) : MAX_int32;
	int32 Filled = 0;
	int32 const SocketCount = GetActiveLoadout(PlayerClass).Sockets.Num();
	for (int32 SocketIndex = 0; SocketIndex < SocketCount && Filled < MaxFilled; ++SocketIndex)
	{
		if (Equip(Catalog, PlayerClass, SocketIndex, GemId))
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
		OnGemCrafted.Broadcast(GemId, Quantity);
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

bool UGeoGemProfileSave::HasClassAtEchoUnlockLevel() const
{
	return Algo::AnyOf(Classes,
					   [](TPair<EPlayerClass, FGeoGemClassProgress> const& Class)
					   {
						   return Class.Value.Level >= GeoGem::EchoUnlockLevel;
					   });
}

void UGeoGemProfileSave::SyncEchoWithClassLevels()
{
	if (HasClassAtEchoUnlockLevel())
	{
		EchoLevel = FMath::Max(EchoLevel, 1);
	}
	else
	{
		EchoLevel = 0;
		EchoXp = 0;
		for (TPair<EPlayerClass, FGeoGemClassProgress>& Class : Classes)
		{
			for (FGeoGemBuild& Build : Class.Value.Builds)
			{
				Build.Loadout.Echo = NAME_None;
			}
		}
	}
}

int32 UGeoGemProfileSave::CountSlotted(FGeoGemLoadout const& Loadout, FName const GemId)
{
	return static_cast<int32>(Algo::Count(Loadout.Sockets, GemId)) + (Loadout.Echo == GemId ? 1 : 0);
}

FGeoGemLoadout& UGeoGemProfileSave::GetActiveLoadout(EPlayerClass const PlayerClass)
{
	FGeoGemClassProgress& Progress = FindOrAddClass(PlayerClass);
	return Progress.Builds[Progress.ActiveBuild].Loadout;
}
