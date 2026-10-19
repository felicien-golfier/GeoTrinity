// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Characters/PlayerClassTypes.h"
#include "CoreMinimal.h"
#include "HUD/Menu/Gems/GeoGemPageWidget.h"

#include "GeoGemLoadoutWidget.generated.h"

class UCanvasPanel;
class UGeoButton;
class UGeoEchoHalo;
class UGeoEditableTextBox;
class UGeoFrameStyle;
class UGeoGemSocketButton;
class UGeoIcon;
class UGeoIconImage;
class UGeoMenuButton;
class UGeoShape;
class UGeoTableWidget;
class UHorizontalBox;
class UUniformGridPanel;
class UVerticalBox;
struct FGeoGemBuild;

/** One ring of a cluster on the socket board: the sockets of one tier around the cluster's centre. */
USTRUCT(BlueprintType)
struct FGeoGemRing
{
	GENERATED_BODY()

	/** Distance of the sockets from the cluster's centre; 0 puts the one socket on it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem", meta = (ClampMin = "0"))
	float Radius = 0.f;

	/** Where the first socket sits, in degrees clockwise from the right; the others share the circle evenly. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem")
	float StartAngle = -90.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem", meta = (ClampMin = "4"))
	float SocketSize = 30.f;
};

/** One cluster of the socket board: where it sits and where its name is written. */
USTRUCT(BlueprintType)
struct FGeoGemCluster
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem")
	FVector2D Center = FVector2D::ZeroVector;

	/** "I", "II", "III". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem")
	FText Name;

	/** Where its label is pinned on the board, and which point of the label is pinned there (0,0 its top-left). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem")
	FVector2D LabelPosition = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem")
	FVector2D LabelAlignment = FVector2D::ZeroVector;
};

/**
 * The Loadout page of the Gems menu: one class's sockets and the gem stacks to fill them. Class tabs with each class's
 * level, in the box the Gems menu's header lends it; the class's builds, each a whole saved loadout: picking a build's
 * tab plays it, so the page always edits the build the class fights with, its tab framed in the class's colour and
 * holding its rename pencil and remove cross (a double click on it renames too, its name field then standing in the
 * tab), then a dashed tab adding an empty build; the socket board, three clusters each a Core ringed by its Prisms,
 * Cuts and Chips, linked by dashed lines, a locked socket showing the level that opens it, the class's level and what
 * the next level opens in its corner; the stacks with their owned and free counts under a tier filter; the selection, a
 * stack (EQUIP: the first empty socket it fits; ALL: every socket it fits, replacing what is slotted; both always
 * shown, off without a picked stack), and UNEQUIP ALL <tier> under the stacks, emptying the filtered tier of the build
 * (the whole build under ALL); and the build's total bonus, a table of stat, bonus and copies. The detail follows the
 * cursor: over a stack or a filled socket it describes that gem without a click, and falls back to the pick when the
 * cursor leaves. Picking a socket also opens its tier's stacks. Picking a stack then a socket it fits, or a socket then
 * a stack, equips it there; the sockets the picked stack fits glow, and the stack stays picked, so a click per socket
 * fills several while copies are free. A click on neither a socket, the stacks nor the detail drops the pick. The board
 * is built once and updated in place, so the socket under the gamepad keeps its focus. Between the clusters, the echo
 * socket (from the max class level) holds one more gem, worth a share of its tier. Required in the BP hierarchy:
 * UCanvasPanel "Board", UUniformGridPanel "StackGrid". Every other part is optional.
 */
UCLASS()
class GEOTRINITYUI_API UGeoGemLoadoutWidget : public UGeoGemPageWidget
{
	GENERATED_BODY()

public:
	/** Refreshes the Loadout page from the current gem profile: socket states, stack counts, selection detail and
	 * totals. */
	virtual void Refresh() override;

	/** The box the class tabs are built in, which lives outside the page, in the Gems menu's header. */
	void SetClassTabBox(UHorizontalBox* InClassTabBox) { ClassTabBox = InClassTabBox; }

protected:
	/** Shows the local player's class first, wires the buttons, and takes the clicks no part of it handles. */
	virtual void NativeConstruct() override;
	/** A click on neither a socket, the stacks nor the detail drops the picked stack or socket. */
	virtual FReply NativeOnMouseButtonDown(FGeometry const& InGeometry, FPointerEvent const& InMouseEvent) override;

	/** Filled with one tab per class; set by SetClassTabBox. */
	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> ClassTabBox;

	/** Filled with one tab per build of the shown class, then the tab adding one. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> BuildTabBox;

	/** A mark in the shown class's colour, beside InPlayText. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UGeoShape> InPlayShape;

	/** The build the shown class fights with. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> InPlayText;

	/** The shown class's builds out of the most it may keep. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BuildCountText;

	/** The class's level, beside LevelMaxText. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LevelText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LevelMaxText;

	/** What the next level opens. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NextText;

	/** Holds the clusters and their sockets, laid out by Clusters and Rings. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UCanvasPanel> Board;

	/** Filled with ALL and one tab per tier. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> TierFilterBox;

	/** Filled with one row per gem owned, StackColumns to a line. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UUniformGridPanel> StackGrid;

	/** Shown while a stack or a filled socket is picked. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWidget> DetailBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UGeoGemGlyph> DetailGlyph;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DetailTierText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DetailNameText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DetailEffectText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DetailCountsText;

	/** Why EQUIP is off. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DetailNoteText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UGeoMenuButton> EquipButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UGeoMenuButton> FillButton;

	/** Empties every socket of the tier the filter shows, or the whole build under ALL; under the stacks. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UGeoMenuButton> UnequipTierButton;

	/** Shown in place of DetailBox: what to do, or what the picked empty socket takes. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HintText;

	/** How many open sockets the build fills. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TotalsFilledText;

	/** Filled with one line per gem type slotted: its stat, what the copies add and how many there are, or the Core's
	 * name and what it does across the last two columns. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UGeoTableWidget> TotalsTable;

	/** The clusters, cluster I first; the sockets are shared out evenly between them, in GeoGem::GetSockets() order. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Board")
	TArray<FGeoGemCluster> Clusters = {
		{FVector2D(342.f, 171.f), INVTEXT("I"), FVector2D(684.f, 4.f), FVector2D(1.f, 0.f)},
		{FVector2D(171.f, 468.f), INVTEXT("II"), FVector2D(0.f, 280.f), FVector2D::ZeroVector},
		{FVector2D(513.f, 468.f), INVTEXT("III"), FVector2D(684.f, 280.f), FVector2D(1.f, 0.f)}};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Board")
	TMap<EGeoGemTier, FGeoGemRing> Rings = {{EGeoGemTier::Core, {0.f, 0.f, 46.f}},
											{EGeoGemTier::Prism, {59.f, -90.f, 34.f}},
											{EGeoGemTier::Cut, {108.f, -54.f, 34.f}},
											{EGeoGemTier::Chip, {156.f, -90.f, 30.f}}};

	/** Where the echo socket's centre sits on the board, between the clusters. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Board")
	FVector2D EchoPosition = FVector2D(342.f, 374.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Board", meta = (ClampMin = "4"))
	float EchoSize = 44.f;

	/** Width and height of the halo behind the echo socket, about three times EchoSize. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Board", meta = (ClampMin = "4"))
	float EchoHaloSize = 132.f;

	/** The frame style the echo halo wears for its runners, glow and idle / active blend (DA_Frame_Button). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Board")
	TObjectPtr<UGeoFrameStyle> EchoHaloStyle;

	/** The halo's colour while the echo socket is open and holds no gem. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Board")
	FLinearColor EchoHaloOpenColor = FLinearColor(.45f, .2f, 1.f, 1.f);

	/** The disc behind a cluster reaches this far past its outermost ring's guide circle. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Board", meta = (ClampMin = "0"))
	float ClusterDiscMargin = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Board")
	FLinearColor ClusterDiscColor = FLinearColor(.0024f, .0018f, .0103f, .6f);

	/** The guide circle each ring of sockets sits on. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Board")
	FLinearColor RingColor = FLinearColor(.45f, .2f, 1.f, .25f);

	/** The dashed lines joining the clusters' centres, in order, under the discs. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Board")
	FLinearColor ClusterLinkColor = FLinearColor(.9f, .85f, 1.f, .18f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Board", meta = (ClampMin = "0.5", ClampMax = "8"))
	float ClusterLinkThickness = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Board", meta = (ClampMin = "0", ClampMax = "44"))
	float ClusterLinkDash = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Board", meta = (ClampMin = "0", ClampMax = "44"))
	float ClusterLinkGap = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem", meta = (ClampMin = "1", ClampMax = "6"))
	int32 StackColumns = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem", meta = (ClampMin = "8", ClampMax = "128"))
	float StackGlyphSize = 30.f;

	/** The mark on a stack slotted in the shown build. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem", meta = (ClampMin = "2", ClampMax = "32"))
	float InBuildMarkSize = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem")
	FLinearColor InBuildMarkColor = FLinearColor(.45f, .2f, 1.f, 1.f);

	/** The square in the gem's colour before a stat's name. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Totals", meta = (ClampMin = "2", ClampMax = "32"))
	float TotalsDotSize = 7.f;

	/** Space between that square and the name. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Totals", meta = (ClampMin = "0", ClampMax = "64"))
	float TotalsDotGap = 8.f;

	/** Line thickness of the guide circle each ring of sockets sits on. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Board", meta = (ClampMin = "0.5", ClampMax = "8"))
	float RingThickness = 1.f;

	/** The classes the class tabs list, in their order. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem")
	TArray<EPlayerClass> ClassTabOrder = {EPlayerClass::Square, EPlayerClass::Circle, EPlayerClass::Triangle};

	/** Size of the class shape on each class tab. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem", meta = (ClampMin = "4", ClampMax = "64"))
	float ClassTabShapeSize = 16.f;

	/** Space between two class tabs. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem", meta = (ClampMin = "0", ClampMax = "64"))
	float ClassTabGap = 10.f;

	/** Size of the class shape on the tab of the build in play. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Builds", meta = (ClampMin = "4", ClampMax = "64"))
	float BuildTabShapeSize = 12.f;

	/** Space between two build tabs. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Builds", meta = (ClampMin = "0", ClampMax = "64"))
	float BuildTabGap = 8.f;

	/** Widest a build's name shows on a tab other than the one in play, cut with an ellipsis past it; the build in play
	 * shows its whole name. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Builds", meta = (ClampMin = "20", ClampMax = "600"))
	float BuildTabNameMaxWidth = 112.f;

	/** Width of the field renaming the build in play. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Builds", meta = (ClampMin = "40", ClampMax = "600"))
	float BuildNameFieldWidth = 128.f;

	/** Space between the name field's edge and its text, slimmer than a form's so it fits in the tab. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Builds")
	FMargin BuildNameFieldPadding = FMargin(12.f, 0.f);

	/** Size of the rename and remove buttons on the tab of the build in play. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Builds")
	FVector2D BuildToolSize = FVector2D(28.f, 32.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Builds")
	TObjectPtr<UGeoIcon> RenameBuildIcon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Builds", meta = (ClampMin = "4", ClampMax = "64"))
	float RenameBuildIconSize = 14.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Builds")
	TObjectPtr<UGeoIcon> RemoveBuildIcon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Builds", meta = (ClampMin = "4", ClampMax = "64"))
	float RemoveBuildIconSize = 12.f;

	/** The whole tab adding a build, its dashed outline included. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Builds")
	TObjectPtr<UGeoIcon> AddBuildIcon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Builds", meta = (ClampMin = "8", ClampMax = "128"))
	float AddBuildIconSize = 36.f;

	/** {0} is the class's level, on its tab. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText ClassLevelFormat = INVTEXT("LV {0}");

	/** {0} is the build's place in the list, for a build the player never named. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText BuildNameFormat = INVTEXT("BUILD #{0}");

	/** {0} is the class's builds, {1} the most it may keep. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText BuildCountFormat = INVTEXT("{0} / {1}");

	/** {0} is a number of copies: owned, on a stack, or slotted, in the totals. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText CountFormat = INVTEXT("\u00D7{0}");

	/** {0} is the max level, after the level. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText LevelMaxFormat = INVTEXT(" / {0}");

	/** {0} is the next level, {1} the sockets it opens. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText NextFormat = INVTEXT("NEXT \u00B7 LV {0}\n+{1} SOCKETS");

	/** {0} is the socket count. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText AllOpenFormat = INVTEXT("ALL {0}\nSOCKETS OPEN");

	/** {0} is the cluster's number, {1} the level that opens it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText ClusterFormat = INVTEXT("CLUSTER {0} \u00B7 LV {1}");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText AllFilterText = INVTEXT("ALL");

	/** {0} is the filtered tier's plural name. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText UnequipTierFormat = INVTEXT("UNEQUIP ALL {0}");

	/** UnequipTierButton's label under the ALL filter. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText UnequipAllText = INVTEXT("UNEQUIP ALL");

	/** {0} is the tier, {1} the free copies. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText StackSubFormat = INVTEXT("{0} \u00B7 {1} FREE");

	/** {0} is the tier's number, {1} its name. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText TierFormat = INVTEXT("TIER {0} \u00B7 {1}");

	/** {0} is the tier's number, {1} its name. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText SocketTierFormat = INVTEXT("TIER {0} \u00B7 {1} SOCKET");

	/** A picked stack: {0} owned, {1} slotted in the shown build, {2} free. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText CountsFormat = INVTEXT("OWNED {0} \u00B7 IN BUILD {1} \u00B7 FREE {2}");

	/** A picked socket's gem: {0} owned, {1} free. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText SocketCountsFormat = INVTEXT("OWNED {0} \u00B7 FREE {1}");

	/** EQUIP's label; it slots the picked stack into the first empty socket it fits. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText EquipText = INVTEXT("EQUIP");

	/** FILL's label; it replaces the gems already slotted in the sockets it fills. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText FillText = INVTEXT("ALL");

	/** {0} is the sockets filled, {1} the sockets open. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText FilledFormat = INVTEXT("{0} / {1} FILLED");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText HintNothingPicked = INVTEXT(
		"Pick a stack to equip it, or a socket to fill it. Each cluster holds a Core in the middle, then "
		"Prisms, Cuts and Chips.");

	/** {0} is the socket's tier, {1} the class, {2} the level that opens it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText HintLockedSocket = INVTEXT("This {0} socket opens at {1} level {2}.");

	/** {0} is the socket's tier. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText HintEmptySocket = INVTEXT("Empty {0} socket. Pick any {0} stack to fill it.");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText NoteNoFreeCopy = INVTEXT("No free copy. Craft more in the Forge.");

	/** {0} is the gem's tier. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText NoteNoSocket = INVTEXT("No open {0} socket left to fill.");

	/** Shown as a Core's value in the totals. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText CoreTotalText = INVTEXT("RULE");

	/** Shown in the totals' count column of the echoed gem. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText EchoTotalText = INVTEXT("ECHO");

	/** EchoTotalText when the count column is too narrow for it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText EchoTotalShortText = INVTEXT("ECH");

	/** The echo socket's tier line: {0} is the echoed gem's tier name. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText EchoTierFormat = INVTEXT("ECHO SOCKET · {0}");

	/** After the class level of a class at its max: {0} is the echo level. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText EchoLevelFormat = INVTEXT(" · ECHO {0}");

	/** The echo at its max level: {0} is the echo level. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText EchoMaxCountsFormat = INVTEXT("ECHO LEVEL {0}");

	/** The echo's progress: {0} echo level, {1} XP earned towards the next, {2} XP it costs. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText EchoCountsFormat = INVTEXT("ECHO LEVEL {0} · {1} / {2} XP");

	/** The stack tile of the echoed gem: {0} is the tier. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText StackEchoedFormat = INVTEXT("{0} · ECHOED");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText NoteEchoed = INVTEXT("Already in the echo socket.");

	/** {0} is the class level that opens the echo socket on every class. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText HintLockedEcho = INVTEXT("The echo socket opens once any class reaches level {0}.");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Text")
	FText HintEmptyEcho = INVTEXT("Empty echo socket. Pick a Chip, Cut or Prism stack to echo it.");

	/** {0} is the gem. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Messages")
	FText MsgAlreadyThere = INVTEXT("{0} is already in this socket.");

	/** {0} is the gem, {1} the socket's tier. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Messages")
	FText MsgWrongTier = INVTEXT("{0} does not fit a {1} socket.");

	/** {0} is the class, {1} the level. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Messages")
	FText MsgLocked = INVTEXT("This socket opens at {0} level {1}.");

	/** {0} is the gem. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Messages")
	FText MsgNoFreeCopy = INVTEXT("No free {0}: every copy is slotted.");

	/** {0} is the class level. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Messages")
	FText MsgEchoLocked = INVTEXT("The echo socket opens once any class reaches level {0}.");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Messages")
	FText MsgEchoCore = INVTEXT("A Core can't go in the echo socket.");

	/** {0} is the gem. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Messages")
	FText MsgAlreadyEcho = INVTEXT("{0} is already your echo.");

	/** {0} is the gem. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Messages")
	FText MsgNoCopy = INVTEXT("You own no {0} to echo.");

	/** {0} is the Core. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Messages")
	FText MsgCoreMoved = INVTEXT("{0} moved from its previous socket.");

	/** {0} is the count, {1} the gem. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Messages")
	FText MsgEchoEmptied = INVTEXT("{0} {1} emptied for the echo.");

	/** {0} is the gem. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Messages")
	FText MsgEchoCleared = INVTEXT("Echo cleared: {0} is now in a socket.");

	/** {0} is the count, {1} the gem. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Messages")
	FText MsgFilled = INVTEXT("{0} sockets filled with {1}.");

	/** {0} is the count. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Messages")
	FText MsgUnequipped = INVTEXT("{0} gems unequipped.");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Messages")
	FText MsgBuildAdded = INVTEXT("Build created.");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Messages")
	FText MsgBuildRemoved = INVTEXT("Build deleted.");

	/** {0} is the limit. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Messages")
	FText MsgMaxBuilds = INVTEXT("Maximum of {0} builds.");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoGem|Messages")
	FText MsgLastBuild = INVTEXT("A class keeps at least one build.");

private:
	UFUNCTION()
	void HandleEquip();

	UFUNCTION()
	void HandleFill();

	/** Empties the filtered tier's sockets of the build in play, or every socket under ALL. */
	UFUNCTION()
	void HandleUnequipTier();

	/** Echoes the picked stack; the sockets of the build slotting that gem empty. */
	void HandleEcho();

	UFUNCTION()
	void HandleRenameBuild();

	UFUNCTION()
	void HandleRemoveBuild();

	UFUNCTION()
	void HandleAddBuild();

	/** Names the build in play as it is typed, in capitals and cut to GeoGem::MaxBuildNameLength. */
	UFUNCTION()
	void HandleBuildNameChanged(FText const& Text);

	/** Enter, Escape and leaving the field all keep the name. */
	UFUNCTION()
	void HandleBuildNameCommitted(FText const& Text, ETextCommit::Type CommitMethod);

	/** Shows or collapses an optional part. */
	static void SetShown(UWidget* Widget, bool bShown);

	/** Builds the cluster discs, ring guides and labels, and one socket button per socket, once. */
	void BuildBoard();
	/** The dashed lines joining the clusters' centres, in a square Extent wide from the board's corner. */
	UGeoIconImage* MakeClusterLinks(float Extent);
	void ShowClassTabs(UGeoGemProfileSave const& Profile);
	/** The build the shown class plays and how many it keeps. */
	void ShowInPlay(UGeoGemProfileSave const& Profile);
	/** The build tabs, the field renaming the build in play standing in place of its name. Every one of them clicks on
	 * press: a press first takes focus from the name field, whose commit rebuilds the tabs under the mouse. */
	void ShowBuildTabs(UGeoGemProfileSave const& Profile);
	/** A button showing Icon at IconSize in Style, tinting it with the button's foreground when bTintWithForeground. */
	UGeoButton* MakeIconButton(UGeoIcon const* Icon, float IconSize, FButtonStyle const& Style,
							   bool bTintWithForeground);
	/** Build's name as the tabs show it: what the player named it, else BuildNameFormat of its place. */
	FText GetBuildName(FGeoGemBuild const& Build, int32 BuildIndex) const;
	/** Applies Change to the profile, drops the pick and any rename, then commits. */
	void ChangeBuilds(TFunctionRef<void(UGeoGemProfileSave&)> Change);
	/** Shows the name field in the tab of the build in play, with the keyboard focus. */
	void StartRename();
	/** Saves the name typed and puts the tab back, rebuilding nothing else, so a click on the stacks that ended the
	 * rename still lands. */
	void EndRename();
	void ShowLevel(UGeoGemProfileSave const& Profile);
	void ShowSockets(UGeoGemProfileSave const& Profile, UGeoGemCatalog const& Catalog);
	/** The echo halo's colour, lock and active look from the echo socket's gem, the pick and the hover; flares once per
	 * session when the echo socket is first shown open. */
	void ShowEchoHalo(UGeoGemProfileSave const& Profile, UGeoGemCatalog const& Catalog);
	void ShowFilters();
	/** UnequipTierButton's label from the tier filter; off when the build holds nothing of it. */
	void ShowUnequipTier(UGeoGemProfileSave const& Profile, UGeoGemCatalog const& Catalog);
	void ShowStacks(UGeoGemProfileSave const& Profile, UGeoGemCatalog const& Catalog);
	/** The stack or filled socket under the cursor, else the picked stack or filled socket, else the hint for what is
	 * picked. */
	void ShowDetail(UGeoGemProfileSave const& Profile, UGeoGemCatalog const& Catalog);
	/** ShownSocket, when not INDEX_NONE, words the tier and counts for a socket's gem; the echo socket's shows the
	 * value the echo gives and its level. bPreview is a gem hovered that is not the picked one: its description only,
	 * the buttons and the note belonging to what is picked. */
	void ShowGemDetail(UGeoGemProfileSave const& Profile, UGeoGemCatalog const& Catalog, FGeoGemInfo const& Gem,
					   EGeoGemTier Tier, int32 ShownSocket, bool bPreview);

	/** The echo socket's index in the board, the picks and the hovers: one past GeoGem::GetSockets(). */
	static int32 GetEchoIndex() { return GeoGem::GetSockets().Num(); }

	/** Socket Index of the board: one of GeoGem::GetSockets(), or the echo socket, a Core socket at the echo unlock. */
	static FGeoGemSocket GetSocket(int32 Index);

	/** Whether socket Index is locked: the echo socket until the shared echo level is 1, the others by the shown
	 * class's level. */
	bool IsSocketLocked(UGeoGemProfileSave const& Profile, int32 Index) const;

	/** What socket Index of Loadout holds, the echo socket's gem included. */
	static FName GetSlottedGem(FGeoGemLoadout const& Loadout, int32 Index);

	void ShowTotals(UGeoGemProfileSave const& Profile, UGeoGemCatalog const& Catalog);

	void PickSocket(int32 SocketIndex);
	void PickStack(FName GemId);

	/** Drops the picked stack or socket, and an echo waiting for its confirm. */
	void ClearPick();

	/** Why a click on socket SocketIndex does nothing: the picked gem's refusal there, else, with nothing picked, the
	 * lock of the socket. */
	EGeoGemRefusal GetClickRefusal(UGeoGemProfileSave const& Profile, UGeoGemCatalog const& Catalog,
								   int32 SocketIndex) const;

	/** Posts Refusal's message to the strip; GemId and SocketIndex fill in the gem and the socket's tier or lock. */
	void PostRefusal(UGeoGemCatalog const& Catalog, EGeoGemRefusal Refusal, FName GemId, int32 SocketIndex) const;

	/** Posts what slotting GemId moved, from Before, the loadout as it was: a Core leaving its socket, the echo
	 * cleared. */
	void PostEquipEffects(UGeoGemCatalog const& Catalog, FGeoGemLoadout const& Before, FName GemId) const;

	/** Sockets and echo of Loadout holding a gem. */
	static int32 CountFilled(FGeoGemLoadout const& Loadout);

	/** Sets what the cursor is over, a stack (GemId) or a socket (SocketIndex), and shows its detail. */
	void Hover(FName GemId, int32 SocketIndex);

	/** The open sockets (empty or not) the picked stack's gem may go into now, for the board's glow, EQUIP and FILL. */
	TArray<int32> GetTargetSockets(UGeoGemProfileSave const& Profile, UGeoGemCatalog const& Catalog) const;

	/** Copies of the picked stack FILL would slot: one per target socket while copies are free, a Core once. */
	int32 GetFillCount(UGeoGemProfileSave const& Profile, UGeoGemCatalog const& Catalog) const;

	EPlayerClass ShownClass = EPlayerClass::None;

	/** Unset shows every tier. */
	TOptional<EGeoGemTier> TierFilter;

	/** The picked stack, kept while it equips socket after socket; NAME_None while a socket or nothing is picked. */
	FName PickedGem;

	/** The picked socket, the echo socket (GetEchoIndex) included; INDEX_NONE while a stack or nothing is picked. */
	int32 PickedSocket = INDEX_NONE;

	/** The stack under the cursor; NAME_None when none. */
	FName HoveredGem;

	/** The socket under the cursor; INDEX_NONE when none. */
	int32 HoveredSocket = INDEX_NONE;

	/** Whether the build in play shows its name field rather than its name. */
	bool bRenamingBuild = false;

	/** The field renaming the build in play, while bRenamingBuild. */
	UPROPERTY(Transient)
	TObjectPtr<UGeoEditableTextBox> BuildNameField;

	UPROPERTY()
	TArray<TObjectPtr<UGeoGemSocketButton>> SocketButtons;

	/** Behind the echo socket's button; built with the board. */
	UPROPERTY(Transient)
	TObjectPtr<UGeoEchoHalo> EchoHalo;

	/** Whether the echo halo has flared yet this session. */
	static bool bEchoFlared;
};
