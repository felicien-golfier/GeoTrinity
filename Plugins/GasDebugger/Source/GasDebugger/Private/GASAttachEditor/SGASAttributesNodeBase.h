#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Views/SListView.h"



static FName NAME_AttributesName(TEXT("AttributesName"));
static FName NAME_GANumericAttribute(TEXT("GANumericAttribute"));

class UAbilitySystemComponent;

class FGASAttributesNodeBase
{
public:

	virtual ~FGASAttributesNodeBase(){};

	// Current GA name
	virtual FName GetGAName() const = 0;

	// Current attribute value
	virtual float GetNumericAttribute() const = 0;

protected:

	FGASAttributesNodeBase(){};
};


class SGASAttributesTreeItem : public SMultiColumnTableRow<TSharedRef<FGASAttributesNodeBase>>
{
public:

	SLATE_BEGIN_ARGS(SGASAttributesTreeItem)
		: _WidgetInfoToVisualize()
	{}
	SLATE_ARGUMENT(TSharedPtr<FGASAttributesNodeBase>, WidgetInfoToVisualize)
		SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView);

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;

protected:
	/** Information about the widget we are visualizing */
	TSharedPtr<FGASAttributesNodeBase> WidgetInfo;

	FName GAName;
	float NumericAttribute;
};

class FGASAttributesNode : public FGASAttributesNodeBase
{
public:
	virtual ~FGASAttributesNode() override;

	static TSharedRef<FGASAttributesNode> Create(TWeakObjectPtr<UAbilitySystemComponent> InASComponent, const FGameplayAttribute& InAttribute);

public:

	virtual FName GetGAName() const override;


	virtual float GetNumericAttribute() const override;


private:

	explicit FGASAttributesNode(TWeakObjectPtr<UAbilitySystemComponent> InASComponent,const FGameplayAttribute InAttribute);

protected:

	TWeakObjectPtr<UAbilitySystemComponent> ASComponent;

	FGameplayAttribute Attribute;
};
