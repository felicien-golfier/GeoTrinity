
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SUserWidget.h"
#include "Framework/Application/IInputProcessor.h"

class SWidget;
class FTabManager;

enum EDebugAbilitieCategories
{
	Tags,
	Attributes,
	GameplayEffects,
	Ability,
};


class SGASAttachEditor : public SUserWidget
{
public:

	SLATE_USER_ARGS(SGASAttachEditor) {}
	SLATE_END_ARGS()

		

public:
	virtual void Construct(const FArguments& InArgs) = 0;

	// Set the status change
	virtual void SetPickingMode(bool bTick) = 0;

	// The tab control name
	static FName GetTabName();

	static void RegisterTabSpawner(FTabManager& TabManager);

};

class FAttachInputProcessor : public IInputProcessor
{
public:
	FAttachInputProcessor(SGASAttachEditor* InWidgetPtr);
	~FAttachInputProcessor() { GASAttachEditorWidgetPtr = nullptr; };

private:

	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override;
	virtual const TCHAR* GetDebugName() const override { return TEXT("AttachInputProcessor"); }
	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override {}

private:
	SGASAttachEditor* GASAttachEditorWidgetPtr;
};