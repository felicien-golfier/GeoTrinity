// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Tool/CatLine/GeoCatLineBank.h"

#include "GeoTrinity/GeoTrinity.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Tool/CatLine/GeoCatLineCategories.h"
#include "Tool/UGeoGameplayLibrary.h"

struct FGeoCatLineFile
{
	TArray<FName> Categories;
	TMap<FName, TArray<FString>> LinesByCategory;
};

/** What one category set has said: the lines left of its shuffled bag, its file-order cursor, its last line. */
struct FGeoCatLineDraw
{
	TArray<FString> Bag;
	int32 Cursor = 0;
	FString Last;
};

static FString FilePathOverride;
static FGeoCatLineFile ParsedFile;
static FDateTime ParsedTimeStamp = FDateTime::MinValue();
static TMap<FString, FGeoCatLineDraw> Draws;

static FGeoCatLineFile const& GetFile()
{
	FString const FilePath =
		FilePathOverride.IsEmpty() ? FPaths::ProjectDir() / TEXT("Data/CatLines.txt") : FilePathOverride;
	FDateTime const TimeStamp = IFileManager::Get().GetTimeStamp(*FilePath);
	if (TimeStamp != ParsedTimeStamp)
	{
		ParsedTimeStamp = TimeStamp;
		ParsedFile = FGeoCatLineFile();
		Draws.Reset();

		FString FileContent;
		TArray<FString> Lines;
		FFileHelper::LoadFileToString(FileContent, *FilePath);
		FileContent.ParseIntoArrayLines(Lines, false);

		FName Category;
		for (FString const& RawLine : Lines)
		{
			FString const Line = RawLine.TrimStartAndEnd();
			if (Line.IsEmpty() || Line.StartsWith(TEXT("#")))
			{
				continue;
			}

			if (Line.StartsWith(TEXT("[")) && Line.EndsWith(TEXT("]")))
			{
				Category = FName(*Line.Mid(1, Line.Len() - 2).TrimStartAndEnd());
				ParsedFile.Categories.AddUnique(Category);
				ParsedFile.LinesByCategory.FindOrAdd(Category);
			}
			else if (Category.IsNone())
			{
				UE_LOG(LogGeoEncounter, Warning, TEXT("%s: line before any [Category] header ignored: %s"), *FilePath,
					   *Line);
			}
			else
			{
				ParsedFile.LinesByCategory.FindOrAdd(Category).Add(Line);
			}
		}
	}
	return ParsedFile;
}

/** The picked categories' lines, in file order; a line in two of them comes once. */
static TArray<FString> GatherPool(FGeoCatLineCategories const& Categories)
{
	FGeoCatLineFile const& File = GetFile();
	TArray<FString> Pool;
	for (FName const Category : File.Categories)
	{
		if (Categories.Names.IsEmpty() || Categories.Names.Contains(Category))
		{
			for (FString const& Line : File.LinesByCategory[Category])
			{
				Pool.AddUnique(Line);
			}
		}
	}
	return Pool;
}

TArray<FName> GeoCatLines::GetCategories()
{
	return GetFile().Categories;
}

int32 GeoCatLines::CountLines(FGeoCatLineCategories const& Categories)
{
	return GatherPool(Categories).Num();
}

bool GeoCatLines::HasLine(FGeoCatLineCategories const& Categories, FString const& Line)
{
	return GatherPool(Categories).Contains(Line);
}

FString GeoCatLines::PickLine(FGeoCatLineCategories const& Categories, bool const bRandom)
{
	TArray<FString> const Pool = GatherPool(Categories);
	FString Line;
	if (Pool.IsEmpty())
	{
		UE_LOG(LogGeoEncounter, Warning, TEXT("GeoCatLines::PickLine: no line in the picked categories"));
	}
	else
	{
		TArray<FString> Names;
		for (FName const Name : Categories.Names)
		{
			Names.Add(Name.ToString());
		}
		Names.Sort();

		FGeoCatLineDraw& Draw =
			Draws.FindOrAdd(FString::Join(Names, TEXT(",")) + (bRandom ? TEXT("|random") : TEXT("|order")));
		if (bRandom)
		{
			Line = GeoLib::DrawFromShuffledBag(Draw.Bag, Pool, Draw.Last);
		}
		else
		{
			Line = Pool[Draw.Cursor % Pool.Num()];
			Draw.Cursor = (Draw.Cursor + 1) % Pool.Num();
		}
	}
	return Line;
}

void GeoCatLines::SetFilePath(FString const& Path)
{
	FilePathOverride = Path;
	ParsedFile = FGeoCatLineFile();
	ParsedTimeStamp = FDateTime::MinValue();
	Draws.Reset();
}
