#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Views/SListView.h"

static FName NAME_GAGameplayEffectName(TEXT("GameplayEffectName"));
static FName NAME_GAGameplayEffectDuration(TEXT("GameplayEffectDuration"));
static FName NAME_GAGameplayEffectStack(TEXT("GameplayEffectStack"));
static FName NAME_GAGameplayEffectLevel(TEXT("GameplayEffectLevel"));
static FName NAME_GAGameplayEffectPrediction(TEXT("GameplayEffectPrediction"));
static FName NAME_GAGameplayEffectGrantedTags(TEXT("GameplayEffectGrantedTags"));

class UAbilitySystemComponent;

class FGASGameplayEffectNodeBase
{
public:

	virtual ~FGASGameplayEffectNodeBase(){};

	virtual FName GetGAName() const = 0;

	virtual FText GetDurationText() const = 0;

	virtual FText GetStackText() const = 0;

	virtual FName GetLevelStr() const = 0;

	virtual FText GetPredictedText() const = 0;

	virtual FName GetGrantedTagsName() const = 0;

	void AddChildNode(TSharedRef<FGASGameplayEffectNodeBase> InChildNode);

	const TArray<TSharedRef<FGASGameplayEffectNodeBase>>& GetChildNodes() const;

protected:

	FGASGameplayEffectNodeBase(){};

	TArray<TSharedRef<FGASGameplayEffectNodeBase>> ChildNodes;
};


class SGASGameplayEffectTreeItem : public SMultiColumnTableRow<TSharedRef<FGASGameplayEffectNodeBase>>
{
public:

	SLATE_BEGIN_ARGS(SGASGameplayEffectTreeItem)
		: _WidgetInfoToVisualize()
	{}
	SLATE_ARGUMENT(TSharedPtr<FGASGameplayEffectNodeBase>, WidgetInfoToVisualize)
		SLATE_END_ARGS()


	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView);


	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;

protected:
	TSharedPtr<FGASGameplayEffectNodeBase> WidgetInfo;

	FName GAName;
	FText DurationText;
	FText StackText;
	FName LevelStr;
	FName GrantedTagsName;
};

class FGASGameplayEffectNode : public FGASGameplayEffectNodeBase
{
public:
	virtual ~FGASGameplayEffectNode() override;

	static TSharedRef<FGASGameplayEffectNode> Create(const UWorld* World, const FActiveGameplayEffect& InGameplayEffect);

	virtual FName GetGAName() const override;

	virtual FText GetDurationText() const override;

	virtual FText GetStackText() const override;

	virtual FName GetLevelStr() const override;

	virtual FText GetPredictedText() const override;

	virtual FName GetGrantedTagsName() const override;

private:

	explicit FGASGameplayEffectNode(const UWorld* World, const FActiveGameplayEffect InGameplayEffect);

	explicit FGASGameplayEffectNode(const FModifierSpec* ModSpec,const FGameplayModifierInfo* ModInfo);

protected:

	void CreateChild();
	
	const UWorld* World;

	TWeakObjectPtr<UAbilitySystemComponent> ASComponent;

	FActiveGameplayEffect GameplayEffect;

	const FModifierSpec* ModSpec;
	const FGameplayModifierInfo* ModInfo;
};
