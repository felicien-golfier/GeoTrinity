// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "Gem/GeoGemTypes.h"

#include "GeoGemCsvSync.generated.h"

class FObjectPostSaveContext;
class UGeoGemCatalog;
class UPackage;
struct FFileChangeData;
struct FPropertyChangedEvent;

/**
 * Keeps the gems of the catalog UGameDataSettings points at and Data/gems.csv equal, both ways: saving the catalog
 * asset rewrites the CSV, saving the CSV imports it into the asset and saves that. Any edit of a catalog, from either
 * side, re-applies the gem stats of every player in a running PIE session. Only the gems are synced, not the catalog's
 * drop, XP or Forge tables.
 */
UCLASS()
class UGeoGemCsvSync : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	/** Watches the catalog's saves and edits and the CSV's folder, then imports the CSV, or writes it when missing. */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * Replaces the catalog's gems with the CSV's rows and saves the asset; a CSV already holding what the catalog would
	 * write changes nothing. A CSV with any bad row changes nothing either and says why. True when the catalog now
	 * matches the CSV.
	 */
	UFUNCTION(BlueprintCallable, Category = "GeoGem")
	bool ImportCsv() const;

private:
	/** First line of the CSV, naming its columns. */
	static constexpr TCHAR const* CsvHeader =
		TEXT("Tier,Id,Name,Attribute,Operation,MagnitudePerGem,Effect,Color,GrantedTag");

	static FString GetCsvPath();

	/** The CSV file's text with \n line ends; empty when it cannot be read. */
	static FString LoadCsv();

	/** Writes Catalog's gems to the CSV, unless it already holds them. */
	static void ExportCsv(UGeoGemCatalog const& Catalog);

	/** Catalog's gems as CSV text, one row per gem, tier by tier. */
	static FString ToCsv(UGeoGemCatalog const& Catalog);

	/** Value as one CSV cell, quoted when it holds a comma, a quote or a line break. */
	static FString ToCsvCell(FString const& Value);

	/** Csv's rows as GemsByTier, every tier listed; unset with one message per bad row in Errors. */
	static TOptional<TMap<EGeoGemTier, FGeoGemList>> ParseCsv(FString const& Csv, TArray<FString>& Errors);

	/** The value of TEnum named Name, unset for a name it has none of. */
	template <typename TEnum>
	static TOptional<TEnum> ParseEnum(FString const& Name);

	/** The attribute named Name, invalid when no attribute set or more than one declares it. */
	static FGameplayAttribute FindAttribute(FString const& Name);

	/** Logs Message and pops it as an editor toast. */
	static void Notify(FString const& Message, bool bSucceeded);

	void OnPackageSaved(FString const& PackageFilename, UPackage* Package, FObjectPostSaveContext SaveContext) const;
	void OnObjectPropertyChanged(UObject* Object, FPropertyChangedEvent& Event) const;
	void OnCsvFolderChanged(TArray<FFileChangeData> const& Changes) const;

	FDelegateHandle PackageSavedHandle;
	FDelegateHandle ObjectPropertyChangedHandle;
	FDelegateHandle CsvFolderChangedHandle;
};
