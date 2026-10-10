// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Gem/GeoGemCsvSync.h"

#include "DirectoryWatcherModule.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Notifications/NotificationManager.h"
#include "GameClasses/GeoPlayerState.h"
#include "Gem/GeoGemCatalog.h"
#include "AbilitySystem/Abilities/Gem/GeoCorePassiveAbility.h"
#include "Gem/GeoGemComponent.h"
#include "Gem/GeoGemStatsEffect.h"
#include "IDirectoryWatcher.h"
#include "Misc/DefaultValueHelper.h"
#include "Misc/FileHelper.h"
#include "Serialization/Csv/CsvParser.h"
#include "Settings/GameDataSettings.h"
#include "UObject/ObjectSaveContext.h"
#include "UObject/UObjectIterator.h"
#include "Widgets/Notifications/SNotificationList.h"

void UGeoGemCsvSync::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	PackageSavedHandle = UPackage::PackageSavedWithContextEvent.AddUObject(this, &UGeoGemCsvSync::OnPackageSaved);
	ObjectPropertyChangedHandle =
		FCoreUObjectDelegates::OnObjectPropertyChanged.AddUObject(this, &UGeoGemCsvSync::OnObjectPropertyChanged);

	FString const CsvFolder = FPaths::GetPath(GetCsvPath());
	IFileManager::Get().MakeDirectory(*CsvFolder, true);
	FDirectoryWatcherModule& DirectoryWatcher =
		FModuleManager::LoadModuleChecked<FDirectoryWatcherModule>(TEXT("DirectoryWatcher"));
	DirectoryWatcher.Get()->RegisterDirectoryChangedCallback_Handle(
		CsvFolder, IDirectoryWatcher::FDirectoryChanged::CreateUObject(this, &UGeoGemCsvSync::OnCsvFolderChanged),
		CsvFolderChangedHandle);

	UGeoGemCatalog const* Catalog = UGeoGemCatalog::Get();
	if (Catalog && !FPaths::FileExists(GetCsvPath()))
	{
		ExportCsv(*Catalog);
	}
	else
	{
		ImportCsv();
	}
}

void UGeoGemCsvSync::Deinitialize()
{
	UPackage::PackageSavedWithContextEvent.Remove(PackageSavedHandle);
	FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(ObjectPropertyChangedHandle);
	if (FDirectoryWatcherModule* DirectoryWatcher =
			FModuleManager::GetModulePtr<FDirectoryWatcherModule>(TEXT("DirectoryWatcher")))
	{
		DirectoryWatcher->Get()->UnregisterDirectoryChangedCallback_Handle(FPaths::GetPath(GetCsvPath()),
																		   CsvFolderChangedHandle);
	}

	Super::Deinitialize();
}

bool UGeoGemCsvSync::ImportCsv() const
{
	UGeoGemCatalog* Catalog = UGameDataSettings::GetLoadedDataAsset(GetDefault<UGameDataSettings>()->GemCatalog);
	if (!ensureMsgf(Catalog, TEXT("%hs: Game Data Settings has no GemCatalog"), __FUNCTION__))
	{
		return false;
	}

	FString const Csv = LoadCsv();
	if (Csv == ToCsv(*Catalog))
	{
		return true;
	}

	TArray<FString> Errors;
	TOptional<TMap<EGeoGemTier, FGeoGemList>> GemsByTier = ParseCsv(Csv, Errors);
	if (GemsByTier)
	{
		Catalog->Modify();
		Catalog->GemsByTier = MoveTemp(*GemsByTier);
		Catalog->PostEditChange();
		UEditorLoadingAndSavingUtils::SavePackages({Catalog->GetPackage()}, false);
		Notify(FString::Printf(TEXT("Gem CSV imported into %s"), *Catalog->GetName()), true);
	}
	else
	{
		Notify(FString::Printf(TEXT("Gem CSV not imported, %s unchanged:\n%s"), *Catalog->GetName(),
							   *FString::Join(Errors, TEXT("\n"))),
			   false);
	}

	return GemsByTier.IsSet();
}

FString UGeoGemCsvSync::GetCsvPath()
{
	return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Data/gems.csv"));
}

FString UGeoGemCsvSync::LoadCsv()
{
	FString Csv;
	FFileHelper::LoadFileToString(Csv, *GetCsvPath());
	return Csv.Replace(TEXT("\r\n"), TEXT("\n"));
}

void UGeoGemCsvSync::ExportCsv(UGeoGemCatalog const& Catalog)
{
	FString const Csv = ToCsv(Catalog);
	if (Csv != LoadCsv()
		&& !FFileHelper::SaveStringToFile(Csv, *GetCsvPath(), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		Notify(FString::Printf(TEXT("Could not write %s: is it open in Excel? Save %s again once it is closed"),
							   *GetCsvPath(), *Catalog.GetName()),
			   false);
	}
}

FString UGeoGemCsvSync::ToCsv(UGeoGemCatalog const& Catalog)
{
	FString Csv = FString(CsvHeader) + TEXT("\n");
	for (TPair<EGeoGemTier, FGeoGemList> const& Tier : Catalog.GemsByTier)
	{
		for (FGeoGemInfo const& Gem : Tier.Value.Gems)
		{
			TArray<FString> const Cells = {
				StaticEnum<EGeoGemTier>()->GetNameStringByValue(static_cast<int64>(Tier.Key)),
				Gem.Id.ToString(),
				Gem.DisplayName.ToString(),
				Gem.Attribute.IsValid() ? Gem.Attribute.GetName() : FString(),
				StaticEnum<EGeoGemOperation>()->GetNameStringByValue(static_cast<int64>(Gem.Operation)),
				FString::Printf(TEXT("%g"), Gem.MagnitudePerGem),
				Gem.Effect.ToString(),
				StaticEnum<EGeoColor>()->GetNameStringByValue(static_cast<int64>(Gem.Color.Color)),
				Gem.GrantedTag.IsValid() ? Gem.GrantedTag.ToString() : FString(),
				Gem.GrantedAbility ? Gem.GrantedAbility->GetPathName() : FString()};
			Csv += FString::JoinBy(Cells, TEXT(","), &UGeoGemCsvSync::ToCsvCell) + TEXT("\n");
		}
	}

	return Csv;
}

FString UGeoGemCsvSync::ToCsvCell(FString const& Value)
{
	bool const bNeedsQuotes = Value.Contains(TEXT(",")) || Value.Contains(TEXT("\"")) || Value.Contains(TEXT("\n"));
	return bNeedsQuotes ? TEXT("\"") + Value.Replace(TEXT("\""), TEXT("\"\"")) + TEXT("\"") : Value;
}

TOptional<TMap<EGeoGemTier, FGeoGemList>> UGeoGemCsvSync::ParseCsv(FString const& Csv, TArray<FString>& Errors)
{
	FCsvParser const Parser(Csv);
	FCsvParser::FRows const& Rows = Parser.GetRows();
	if (Rows.IsEmpty() || FString::Join(TArray<FString>(Rows[0]), TEXT(",")) != CsvHeader)
	{
		Errors.Add(FString::Printf(TEXT("the first line must be the comma-separated header %s"), CsvHeader));
		return {};
	}

	TMap<EGeoGemTier, FGeoGemList> GemsByTier = GetDefault<UGeoGemCatalog>()->GemsByTier;
	TSet<FName> Ids;
	for (int32 RowIndex = 1; RowIndex < Rows.Num(); ++RowIndex)
	{
		TArray<FString> const Cells(Rows[RowIndex]);
		bool const bBlankLine = !Cells.ContainsByPredicate(
			[](FString const& Cell)
			{
				return !Cell.IsEmpty();
			});
		if (!bBlankLine && Cells.Num() != Rows[0].Num())
		{
			Errors.Add(FString::Printf(TEXT("line %d has %d cells instead of %d"), RowIndex + 1, Cells.Num(),
									   Rows[0].Num()));
		}
		else if (!bBlankLine)
		{
			TOptional<EGeoGemTier> const Tier = ParseEnum<EGeoGemTier>(Cells[0]);
			TOptional<EGeoGemOperation> const Operation = ParseEnum<EGeoGemOperation>(Cells[4]);
			TOptional<EGeoColor> const Color = ParseEnum<EGeoColor>(Cells[7]);
			FGeoGemInfo Gem;
			Gem.Id = FName(Cells[1]);
			Gem.DisplayName = FText::ChangeKey(TEXT("GeoGem"), Cells[1], FText::FromString(Cells[2]));
			Gem.Attribute = FindAttribute(Cells[3]);
			Gem.Effect = FText::ChangeKey(TEXT("GeoGem"), Cells[1] + TEXT("_Effect"), FText::FromString(Cells[6]));
			Gem.GrantedTag = FGameplayTag::RequestGameplayTag(FName(Cells[8]), false);
			Gem.GrantedAbility = Cells[9].IsEmpty() ? nullptr : LoadClass<UGeoCorePassiveAbility>(nullptr, *Cells[9]);
			bool const bMagnitudeParsed = FDefaultValueHelper::ParseFloat(Cells[5], Gem.MagnitudePerGem);

			TArray<FString> RowErrors;
			if (Gem.Id.IsNone() || Ids.Contains(Gem.Id))
			{
				RowErrors.Add(TEXT("its Id is empty or already used"));
			}

			if (!Tier)
			{
				RowErrors.Add(FString::Printf(TEXT("no tier is named \"%s\""), *Cells[0]));
			}

			if (!Cells[3].IsEmpty() && !Gem.Attribute.IsValid())
			{
				RowErrors.Add(FString::Printf(TEXT("no single attribute is named \"%s\""), *Cells[3]));
			}

			if (!Operation)
			{
				RowErrors.Add(FString::Printf(TEXT("no operation is named \"%s\""), *Cells[4]));
			}

			if (!bMagnitudeParsed)
			{
				RowErrors.Add(FString::Printf(TEXT("\"%s\" is not a number"), *Cells[5]));
			}

			if (!Color || *Color == EGeoColor::Override)
			{
				RowErrors.Add(FString::Printf(TEXT("\"%s\" is not a palette colour"), *Cells[7]));
			}

			if (!Cells[8].IsEmpty() && !Gem.GrantedTag.IsValid())
			{
				RowErrors.Add(FString::Printf(TEXT("no gameplay tag is named \"%s\""), *Cells[8]));
			}

			if (!Cells[9].IsEmpty() && !Gem.GrantedAbility)
			{
				RowErrors.Add(FString::Printf(TEXT("no Core passive ability class is named \"%s\""), *Cells[9]));
			}

			if (RowErrors.IsEmpty())
			{
				Gem.Operation = *Operation;
				Gem.Color.Color = *Color;
			}

			if (RowErrors.IsEmpty() && !UGeoGemStatsEffect::Supports(Gem))
			{
				RowErrors.Add(TEXT("the gem stats effect lists no such attribute and operation"));
			}

			if (RowErrors.IsEmpty())
			{
				Ids.Add(Gem.Id);
				GemsByTier.FindOrAdd(*Tier).Gems.Add(MoveTemp(Gem));
			}
			else
			{
				Errors.Add(FString::Printf(TEXT("line %d (%s): %s"), RowIndex + 1, *Cells[1],
										   *FString::Join(RowErrors, TEXT(", "))));
			}
		}
	}

	if (!Errors.IsEmpty())
	{
		return {};
	}

	return GemsByTier;
}

template <typename TEnum>
TOptional<TEnum> UGeoGemCsvSync::ParseEnum(FString const& Name)
{
	int64 const Value = StaticEnum<TEnum>()->GetValueByNameString(Name);
	if (Value == INDEX_NONE)
	{
		return {};
	}

	return static_cast<TEnum>(Value);
}

FGameplayAttribute UGeoGemCsvSync::FindAttribute(FString const& Name)
{
	TArray<FProperty*> Declarations;
	for (TObjectIterator<UClass> Class; Class; ++Class)
	{
		FProperty* Property = Class->IsChildOf<UAttributeSet>() ? FindFProperty<FProperty>(*Class, *Name) : nullptr;
		if (Property && Property->GetOwnerClass() == *Class
			&& FGameplayAttribute::IsGameplayAttributeDataProperty(Property))
		{
			Declarations.Add(Property);
		}
	}

	if (Declarations.Num() != 1)
	{
		return {};
	}

	return FGameplayAttribute(Declarations[0]);
}

void UGeoGemCsvSync::Notify(FString const& Message, bool const bSucceeded)
{
	if (bSucceeded)
	{
		UE_LOG(LogTemp, Log, TEXT("%s"), *Message);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("%s"), *Message);
	}

	if (FSlateApplication::IsInitialized())
	{
		FNotificationInfo Info(FText::FromString(Message));
		Info.ExpireDuration = bSucceeded ? 3.f : 10.f;
		TSharedPtr<SNotificationItem> const Notification = FSlateNotificationManager::Get().AddNotification(Info);
		if (Notification)
		{
			Notification->SetCompletionState(bSucceeded ? SNotificationItem::CS_Success : SNotificationItem::CS_Fail);
		}
	}
}

void UGeoGemCsvSync::OnPackageSaved(FString const& /*PackageFilename*/, UPackage* Package,
									FObjectPostSaveContext SaveContext) const
{
	UGeoGemCatalog const* Catalog = Cast<UGeoGemCatalog>(Package->FindAssetInPackage());
	if (Catalog && SaveContext.SaveSucceeded() && !SaveContext.IsCooking() && !SaveContext.IsFromAutoSave()
		&& !SaveContext.IsProceduralSave() && Catalog == UGeoGemCatalog::Get())
	{
		ExportCsv(*Catalog);
	}
}

void UGeoGemCsvSync::OnObjectPropertyChanged(UObject* Object, FPropertyChangedEvent& /*Event*/) const
{
	if (Cast<UGeoGemCatalog>(Object))
	{
		for (FWorldContext const& Context : GEngine->GetWorldContexts())
		{
			UWorld* World = Context.World();
			if (Context.WorldType == EWorldType::PIE && World)
			{
				for (TActorIterator<AGeoPlayerState> PlayerState(World); PlayerState; ++PlayerState)
				{
					if (PlayerState->HasAuthority())
					{
						PlayerState->GetGemComponent()->ApplyGems();
					}
				}
			}
		}
	}
}

void UGeoGemCsvSync::OnCsvFolderChanged(TArray<FFileChangeData> const& Changes) const
{
	FString const CsvName = FPaths::GetCleanFilename(GetCsvPath());
	bool const bCsvChanged = Changes.ContainsByPredicate(
		[&CsvName](FFileChangeData const& Change)
		{
			return Change.Action != FFileChangeData::FCA_Removed && FPaths::GetCleanFilename(Change.Filename) == CsvName;
		});
	if (bCsvChanged)
	{
		ImportCsv();
	}
}
