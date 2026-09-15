// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Characters/PlayerClassTypes.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "HUD/Menu/GeoListPanelWidget.h"
#include "Tool/GeoDifficulty.h"

#include "GeoLeaderboardWidget.generated.h"

class UGeoLeaderboardSave;
class UGeoListRowWidget;
class UHorizontalBox;
class UTexture2D;
struct FGeoLeaderboardEntry;
struct FGeoLeaderboardPlayer;

/**
 * Leaderboard panel: every fight recorded on this machine (AGeoArena hands each finished attempt to
 * UGeoLeaderboardSave), one tab per boss fought, a strip of difficulty tabs under them — a boss always opens on
 * Original — and, inside a boss and a difficulty, those attempts best first. Least health left leads, so a kill tops
 * the list and a boss never touched sits at the bottom, ties broken by the faster attempt; attempts at different
 * difficulties are never ranked against each other. One row per attempt: rank, how much of the boss was left (KILLED
 * when none was), how long it took, and a column per player — the shape of the class they played, then their name —
 * sized so a full team of three fits the row side by side.
 * Clicking an attempt unfolds what each of its players did with their class — damage, healing and damage taken, each
 * with a bar reading as its share of the best in the column, plus the rates and the biggest single hit and heal —
 * under the row itself, so the run it belongs to stays in sight. One attempt is unfolded at a time.
 * Rows and tabs are both list rows, and the panel wears the shared list frame, so it and the server browser are the
 * same list — see UGeoListPanelWidget.
 * Required in the BP hierarchy, on top of the base's: UHorizontalBox "TabsBox" and "DifficultyTabsBox".
 */
UCLASS()
class GEOTRINITYUI_API UGeoLeaderboardWidget : public UGeoListPanelWidget
{
	GENERATED_BODY()

protected:
	/** Rasterizes the class icons, then builds the tabs and the list under the first boss. */
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UHorizontalBox> TabsBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UHorizontalBox> DifficultyTabsBox;

private:
	/** Fills TabsBox with one tab per boss the leaderboard holds an attempt at and DifficultyTabsBox with one per
	 * difficulty, hardest first, then shows the first boss. A difficulty tab wears the header tint and upper case, so
	 * the strip reads as a filter on the boss tab above it rather than as more bosses. */
	void BuildTabs();

	/** Shows Arena's attempts at Original and marks its tab as the one showing. */
	void SelectArena(FGameplayTag Arena);

	/** Shows the selected boss's attempts at Difficulty and marks its tab as the one showing. */
	void SelectDifficulty(EGeoDifficulty Difficulty);

	/** Fills the list with a header and one row per attempt at the selected boss and difficulty, or a single line when
	 * there is none. */
	void PopulateEntries();

	/** Unfolds the figures of Leaderboard's entry EntryIndex under its row, or folds them away when it is the entry
	 * already showing them. */
	void ToggleEntry(int32 EntryIndex);

	/** Appends the unfolded attempt's stat table: a line naming the columns, then one line per player. */
	void AddStatRows(FGeoLeaderboardEntry const& Entry);

	/** Appends one figure to a stat row. ColumnMax draws a bar under it reading as the figure's share of the best in
	 * its column; 0 is a figure nothing compares it to, which is the number on its own. */
	void AddStatColumn(UGeoListRowWidget* Row, float Value, float ColumnMax, FLinearColor BarColor);

	/** Appends one player to Row: the shape of their class, then their name shortened to what the column holds. */
	void AddPlayerColumn(UGeoListRowWidget* Row, FGeoLeaderboardPlayer const& Player);

	/** Rasterizes ClassIcons, one marker per playable class, before any row asks for one. */
	void BuildClassIcons();

	/** The class marker Class is drawn as: its shape in its colour, with no asset to author. */
	static UTexture2D* CreateClassIcon(EPlayerClass Class);

	/** True when Point, in the icon's 0-1 square with Y downwards, is inside the shape Class is drawn as. */
	static bool IsInsideClassShape(EPlayerClass Class, FVector2f Point);

	/** The colour Class is drawn in: yellow triangle, blue square, green circle. */
	static FLinearColor GetClassColor(EPlayerClass Class);

	/** Read once on construction, so switching tabs never goes back to disk. */
	UPROPERTY()
	TObjectPtr<UGeoLeaderboardSave> Leaderboard;

	/** The tab row showing each boss's attempts, by the arena tag it stands for. */
	UPROPERTY()
	TMap<FGameplayTag, TObjectPtr<UGeoListRowWidget>> TabRows;

	/** The tab row showing the selected boss's attempts at each difficulty. */
	UPROPERTY()
	TMap<EGeoDifficulty, TObjectPtr<UGeoListRowWidget>> DifficultyTabRows;

	/** Boss whose attempts the list is showing. */
	FGameplayTag SelectedArena;

	/** Difficulty the list is showing the selected boss's attempts at. */
	EGeoDifficulty SelectedDifficulty = EGeoDifficulty::Original;

	/** Index in Leaderboard's entries of the attempt unfolded under its row; INDEX_NONE while the list is folded.
	 * Keyed by position rather than AttemptId, since an attempt saved before ids existed carries an empty one — every
	 * such row would match, and the empty id is also what "nothing unfolded" would be. */
	int32 ExpandedEntryIndex = INDEX_NONE;

	/** One marker per playable class, shared by every row and kept alive by this map. */
	UPROPERTY()
	TMap<EPlayerClass, TObjectPtr<UTexture2D>> ClassIcons;
};
