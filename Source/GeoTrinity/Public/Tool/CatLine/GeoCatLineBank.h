// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FGeoCatLineCategories;

/** Data/CatLines.txt parsed: the categories in file order, and each one's lines. */
struct FGeoCatLineFile
{
	TArray<FName> Categories;
	TMap<FName, TArray<FString>> LinesByCategory;
};

/** The Cat's lines of Data/CatLines.txt (Tool/CatLine/CLAUDE.md), re-parsed whenever the file's timestamp changes. */
namespace GeoCatLines
{
	/** The parsed file, read again when its timestamp changed (every draw state dropped then). A missing file is a
	 * packaging or path bug: ensures, and holds no category. */
	GEOTRINITY_API FGeoCatLineFile const& GetFile();

	/** The picked categories' lines, in file order; a line in two of them comes once (an empty set = every category).
	 */
	GEOTRINITY_API TArray<FString> GatherPool(FGeoCatLineCategories const& Categories);

	/** The category names of the file, in file order. */
	GEOTRINITY_API TArray<FName> GetCategories();

	/** How many lines the picked categories hold together (an empty set = every category; a name the file lacks counts
	 * 0). */
	GEOTRINITY_API int32 CountLines(FGeoCatLineCategories const& Categories);

	/** True when Line is one of the lines of the picked categories (a test checks what was said). */
	GEOTRINITY_API bool HasLine(FGeoCatLineCategories const& Categories, FString const& Line);

	/**
	 * One line of the picked categories. bRandom: shuffled, no repeat until every line was said, a refill never starts
	 * with the line just said. Not bRandom: file order, cycling. Each category set (and mode) has its own state, shared
	 * by every caller. Empty when no line matches (logs).
	 */
	GEOTRINITY_API FString PickLine(FGeoCatLineCategories const& Categories, bool bRandom);

	/** Reads Path instead of the default file (tests); empty = the default. Drops the parsed file and every state. */
	GEOTRINITY_API void SetFilePath(FString const& Path);
} // namespace GeoCatLines
