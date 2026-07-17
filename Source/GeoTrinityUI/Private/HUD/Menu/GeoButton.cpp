// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/GeoButton.h"

#include "Components/ButtonSlot.h"
#include "Settings/GameDataSettings.h"
#include "Sound/SoundBase.h"
#include "Widgets/Input/SButton.h"

// Maps focus onto SButton's protected hover state so focus renders and sounds exactly like mouse-over.
class SGeoButton : public SButton
{
public:
	virtual FReply OnFocusReceived(FGeometry const& MyGeometry, FFocusEvent const& InFocusEvent) override
	{
		FReply const Reply = SButton::OnFocusReceived(MyGeometry, InFocusEvent);
		if (!IsHovered())
		{
			SetHover(true);
			ExecuteHoverStateChanged(true);
		}
		return Reply;
	}

	virtual void OnFocusLost(FFocusEvent const& InFocusEvent) override
	{
		SButton::OnFocusLost(InFocusEvent);
		if (IsHovered())
		{
			SetHover(false);
			ExecuteHoverStateChanged(true);
		}
		// Empty attribute returns hover ownership to the regular mouse enter/leave handling.
		SetHover(TAttribute<bool>());
	}
};

TSharedRef<SWidget> UGeoButton::RebuildWidget()
{
	UGameDataSettings const* GameDataSettings = GetDefault<UGameDataSettings>();
	USoundBase* DefaultClickSound = GameDataSettings->GetLoadedDataAsset(GameDataSettings->DefaultButtonClickSound);
	USoundBase* DefaultHoverSound = GameDataSettings->GetLoadedDataAsset(GameDataSettings->DefaultButtonHoverSound);
	if ((!GetStyle().PressedSlateSound.GetResourceObject() && DefaultClickSound) ||
		(!GetStyle().HoveredSlateSound.GetResourceObject() && DefaultHoverSound))
	{
		FButtonStyle NewStyle = GetStyle();
		if (!NewStyle.PressedSlateSound.GetResourceObject() && DefaultClickSound)
		{
			NewStyle.PressedSlateSound.SetResourceObject(DefaultClickSound);
		}
		if (!NewStyle.HoveredSlateSound.GetResourceObject() && DefaultHoverSound)
		{
			NewStyle.HoveredSlateSound.SetResourceObject(DefaultHoverSound);
		}
		SetStyle(NewStyle);
	}

	MyButton = SNew(SGeoButton)
				   .OnClicked(BIND_UOBJECT_DELEGATE(FOnClicked, SlateHandleClicked))
				   .OnPressed(BIND_UOBJECT_DELEGATE(FSimpleDelegate, SlateHandlePressed))
				   .OnReleased(BIND_UOBJECT_DELEGATE(FSimpleDelegate, SlateHandleReleased))
				   .OnHovered_UObject(this, &ThisClass::SlateHandleHovered)
				   .OnUnhovered_UObject(this, &ThisClass::SlateHandleUnhovered)
				   .OnReceivedFocus_UObject(this, &ThisClass::SlateHandleOnReceivedFocus)
				   .OnLostFocus_UObject(this, &ThisClass::SlateHandleOnLostFocus)
				   .OnSlateButtonDragDetected(BIND_UOBJECT_DELEGATE(FOnDragDetected, SlateHandleDragDetected))
				   .OnSlateButtonDragEnter(BIND_UOBJECT_DELEGATE(FOnDragEnter, SlateHandleDragEnter))
				   .OnSlateButtonDragLeave(BIND_UOBJECT_DELEGATE(FOnDragLeave, SlateHandleDragLeave))
				   .OnSlateButtonDragOver(BIND_UOBJECT_DELEGATE(FOnDragOver, SlateHandleDragOver))
				   .OnSlateButtonDrop(BIND_UOBJECT_DELEGATE(FOnDrop, SlateHandleDrop))
				   .ButtonStyle(&GetStyle())
				   .ClickMethod(GetClickMethod())
				   .TouchMethod(GetTouchMethod())
				   .PressMethod(GetPressMethod())
				   .IsFocusable(GetIsFocusable())
				   .AllowDragDrop(bAllowDragDrop);

	if (GetChildrenCount() > 0)
	{
		Cast<UButtonSlot>(GetContentSlot())->BuildSlot(MyButton.ToSharedRef());
	}

	return MyButton.ToSharedRef();
}
