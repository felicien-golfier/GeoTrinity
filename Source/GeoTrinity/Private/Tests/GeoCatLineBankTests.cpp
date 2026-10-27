// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Tool/CatLine/GeoCatLineBank.h"
#include "Tool/CatLine/GeoCatLineCategories.h"

#if WITH_DEV_AUTOMATION_TESTS

constexpr EAutomationTestFlags GeoCatLineBankTestFlags =
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

static FGeoCatLineCategories Picked(TArray<FName> const& Names)
{
	FGeoCatLineCategories Categories;
	Categories.Names = Names;
	return Categories;
}

// ---------------------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeoCatLineBankTest, "GeoTrinity.CatLine.Bank", GeoCatLineBankTestFlags)

bool FGeoCatLineBankTest::RunTest(FString const& /*Parameters*/)
{
	FString const Path = FPaths::CreateTempFilename(*FPaths::ProjectIntermediateDir(), TEXT("CatLines"), TEXT(".txt"));
	FFileHelper::SaveStringToFile(TEXT("before any header\n[A]\n# a comment\na1\n\n  a2  \n[B]\nb1\nb2\nb3\n[Empty]\n"),
								  *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	GeoCatLines::SetFilePath(Path);

	TestTrue(TEXT("Categories come in file order, a header with no line included"),
			 GeoCatLines::GetCategories() == TArray<FName>{"A", "B", "Empty"});
	TestEqual(TEXT("An empty set counts every line"), GeoCatLines::CountLines(Picked({})), 5);
	TestEqual(TEXT("One category counts its lines, comments and blanks left out"),
			  GeoCatLines::CountLines(Picked({"A"})), 2);
	TestEqual(TEXT("A category the file lacks counts nothing"), GeoCatLines::CountLines(Picked({"A", "Nope"})), 2);
	TestEqual(TEXT("Only missing or empty categories hold no line"), GeoCatLines::CountLines(Picked({"Nope", "Empty"})),
			  0);
	TestTrue(TEXT("HasLine knows a line of the picked categories, not another"),
			 GeoCatLines::HasLine(Picked({"A"}), TEXT("a1")) && !GeoCatLines::HasLine(Picked({"A"}), TEXT("b1")));
	TestTrue(TEXT("No match picks an empty line"), GeoCatLines::PickLine(Picked({"Nope"}), true).IsEmpty());

	TSet<FString> FirstBag;
	for (int32 Pick = 0; Pick < 3; ++Pick)
	{
		FirstBag.Add(GeoCatLines::PickLine(Picked({"B"}), true));
	}
	TestTrue(TEXT("No repeat until the bag is spent"), FirstBag.Num() == 3);

	FString Previous;
	bool bRepeatedAcrossRefill = false;
	for (int32 Pick = 0; Pick < 40; ++Pick)
	{
		FString const Line = GeoCatLines::PickLine(Picked({"A"}), true);
		TestTrue(TEXT("A pick belongs to the picked category, trimmed"), Line == TEXT("a1") || Line == TEXT("a2"));
		bRepeatedAcrossRefill |= Line == Previous;
		Previous = Line;
	}
	TestFalse(TEXT("A refill never starts with the line just said"), bRepeatedAcrossRefill);

	TestEqual(TEXT("File order: first"), GeoCatLines::PickLine(Picked({"B"}), false), FString("b1"));
	TestEqual(TEXT("File order: second"), GeoCatLines::PickLine(Picked({"B"}), false), FString("b2"));
	TestEqual(TEXT("File order: third"), GeoCatLines::PickLine(Picked({"B"}), false), FString("b3"));
	TestEqual(TEXT("File order cycles"), GeoCatLines::PickLine(Picked({"B"}), false), FString("b1"));

	GeoCatLines::PickLine(Picked({"B"}), true);
	FFileHelper::SaveStringToFile(TEXT("[B]\nb9\n[C]\nc1\n"), *Path,
								  FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	IFileManager::Get().SetTimeStamp(*Path, FDateTime::UtcNow() + FTimespan::FromSeconds(60));
	TestTrue(TEXT("An edited file is read again"), GeoCatLines::GetCategories() == TArray<FName>{"B", "C"});
	TestEqual(TEXT("A bag started before the edit says the edited lines at once"),
			  GeoCatLines::PickLine(Picked({"B"}), true), FString("b9"));
	TestEqual(TEXT("A new line of the edited file is picked"), GeoCatLines::PickLine(Picked({"C"}), true),
			  FString("c1"));

	GeoCatLines::SetFilePath(FString());
	IFileManager::Get().Delete(*Path);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
