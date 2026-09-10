// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Characters/PlayerClassTypes.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "HUD/Menu/GeoListPanelWidget.h"

#include "GeoLeaderboardWidget.generated.h"

class UGeoLeaderboardSave;
class UGeoListRowWidget;
class UHorizontalBox;
class UTexture2D;
struct FGeoLeaderboardPlayer;

/**
 * Leaderboard panel: every fight recorded on this machine (AGeoArena hands each finished attempt to
 * UGeoLeaderboardSave), one tab per boss fought and, inside a tab, that boss's attempts best first — least health
 * left leads, so a kill tops the list and a boss never touched sits at the bottom, ties broken by the faster
 * attempt. One row per attempt: rank, how much of the boss was left (KILLED when none was), how long it took, and a
 * column per player — the shape of the class they played, then their name — sized so a full team of three fits the
 * row side by side.
 * Rows and tabs are both list rows, and the panel wears the shared list frame, so it and the server browser are the
 * same list — see UGeoListPanelWidget.
 * Required in the BP hierarchy, on top of the base's: UHorizontalBox "TabsBox".
 */
UCLASS()
class GEOTRINITYUI_API UGeoLeaderboardWidget : public UGeoListPanelWidget
{
	GENERATED_BODY()

protected:
	/** Rasterizes the class icons, then builds the boss tabs and the list under the first one. */
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UHorizontalBox> TabsBox;

private:
	/** Fills TabsBox with one tab per boss the leaderboard holds an attempt at, and shows the first one. */
	void BuildTabs();

	/** Shows Arena's attempts and marks its tab as the one showing. */
	void SelectArena(FGameplayTag Arena);

	/** Fills the list with a header and one row per attempt at the selected boss, or a single line when it holds
	 * none. */
	void PopulateEntries();

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

	/** Boss whose attempts the list is showing. */
	FGameplayTag SelectedArena;

	/** One marker per playable class, shared by every row and kept alive by this map. */
	UPROPERTY()
	TMap<EPlayerClass, TObjectPtr<UTexture2D>> ClassIcons;
};
