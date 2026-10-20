// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/GeoMainMenuWidget.h"

#include "Components/AudioComponent.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "GameClasses/GeoGameInstance.h"
#include "HUD/Menu/Frame/GeoMenuButton.h"
#include "HUD/Menu/GeoTutorialPromptWidget.h"
#include "HUD/Menu/List/GeoLeaderboardWidget.h"
#include "HUD/Menu/Server/GeoBrowseServersWidget.h"
#include "HUD/Menu/Server/GeoCreateServerWidget.h"
#include "HUD/Menu/Server/GeoLocalConnectWidget.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Kismet/GameplayStatics.h"
#include "OnlineSubsystem.h"
#include "Settings/GeoGameUserSettings.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

// ---------------------------------------------------------------------------------------------------------------------
FString UGeoMainMenuWidget::GetLocalPlayerName() const
{
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (!OnlineSub)
	{
		UE_LOG(LogTemp, Warning, TEXT("UGeoMainMenuWidget::GetLocalPlayerName: Online subsystem not available"));
		return FString();
	}

	IOnlineIdentityPtr Identity = OnlineSub->GetIdentityInterface();
	if (!Identity.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("UGeoMainMenuWidget::GetLocalPlayerName: Identity interface not valid"));
		return FString();
	}

	return Identity->GetPlayerNickname(0);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	CreateServerButton->OnClicked.AddUniqueDynamic(this, &UGeoMainMenuWidget::HandleCreateServer);
	JoinServerButton->OnClicked.AddUniqueDynamic(this, &UGeoMainMenuWidget::HandleJoinServer);
	PlayLocalButton->OnClicked.AddUniqueDynamic(this, &UGeoMainMenuWidget::HandlePlayLocal);
	TutorialButton->OnClicked.AddUniqueDynamic(this, &UGeoMainMenuWidget::HandleTutorial);
	LeaderboardButton->OnClicked.AddUniqueDynamic(this, &UGeoMainMenuWidget::HandleLeaderboard);
	QuitButton->OnClicked.AddUniqueDynamic(this, &UGeoMainMenuWidget::HandleQuit);

	if (ensureMsgf(MusicLoop, TEXT("UGeoMainMenuWidget::NativeConstruct: MusicLoop is not set")))
	{
		MusicComponent = UGameplayStatics::CreateSound2D(this, MusicLoop, 1.f, 1.f, 0.f, nullptr, false, false);
		MusicComponent->FadeIn(1.f, 1.f);
	}

	if (!UGeoGameUserSettings::Get()->IsTutorialPromptAnswered())
	{
		OpenPage(UGeoTutorialPromptWidget::StaticClass());
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMainMenuWidget::NativeDestruct()
{
	if (MusicComponent)
	{
		MusicComponent->FadeOut(0.5f, 0.f);
		MusicComponent = nullptr;
	}

	Super::NativeDestruct();
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMainMenuWidget::NativeTick(FGeometry const& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UGeoGameInstance* GameInstance = Cast<UGeoGameInstance>(GetGameInstance());
	if (GameInstance && !ErrorPopup.IsValid())
	{
		FString const SessionError = GameInstance->TakeSessionError();
		if (!SessionError.IsEmpty())
		{
			ShowErrorPopup(SessionError);
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMainMenuWidget::ShowErrorPopup(FString const& Message)
{
	TSharedRef<SButton> const OkButton =
		SNew(SButton)
			.HAlign(HAlign_Center)
			.ContentPadding(FMargin(32.f, 8.f))
			.OnClicked_UObject(this, &UGeoMainMenuWidget::HandleErrorPopupClosed)
				[SNew(STextBlock).Text(INVTEXT("OK")).Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))];

	ErrorPopup =
		SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.7f))
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)[SNew(SBox).MaxDesiredWidth(
				800.f)[SNew(SBorder)
						   .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
						   .BorderBackgroundColor(FLinearColor(0.05f, 0.05f, 0.08f, 1.f))
						   .Padding(24.f)[SNew(SVerticalBox)
										  + SVerticalBox::Slot()
												.AutoHeight()[SNew(STextBlock)
																  .Text(INVTEXT("Connection error"))
																  .Font(FCoreStyle::GetDefaultFontStyle("Bold", 24))
																  .ColorAndOpacity(FLinearColor(1.f, 0.3f, 0.3f, 1.f))]
										  + SVerticalBox::Slot().AutoHeight().Padding(
											  0.f, 16.f)[SNew(STextBlock)
															 .Text(FText::FromString(Message))
															 .Font(FCoreStyle::GetDefaultFontStyle("Regular", 16))
															 .AutoWrapText(true)]
										  + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[OkButton]]]];

	GetGameInstance()->GetGameViewportClient()->AddViewportWidgetContent(ErrorPopup.ToSharedRef(), 100);
	FSlateApplication::Get().SetAllUserFocus(OkButton, EFocusCause::SetDirectly);
}

// ---------------------------------------------------------------------------------------------------------------------
FReply UGeoMainMenuWidget::HandleErrorPopupClosed()
{
	GetGameInstance()->GetGameViewportClient()->RemoveViewportWidgetContent(ErrorPopup.ToSharedRef());
	ErrorPopup.Reset();
	SetFocus();
	return FReply::Handled();
}

// ---------------------------------------------------------------------------------------------------------------------
UWidget* UGeoMainMenuWidget::GetInitialFocusWidget() const
{
	return CreateServerButton;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMainMenuWidget::HandleCreateServer()
{
	OpenPage(UGeoCreateServerWidget::StaticClass());
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMainMenuWidget::HandleJoinServer()
{
	OpenPage(UGeoBrowseServersWidget::StaticClass());
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMainMenuWidget::HandlePlayLocal()
{
	OpenPage(UGeoLocalConnectWidget::StaticClass());
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMainMenuWidget::HandleTutorial()
{
	UGeoGameInstance* GameInstance = Cast<UGeoGameInstance>(GetGameInstance());
	if (ensureMsgf(GameInstance, TEXT("%hs: GameInstance is not a UGeoGameInstance"), __FUNCTION__))
	{
		GameInstance->OpenTutorial();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMainMenuWidget::HandleLeaderboard()
{
	OpenPage(UGeoLeaderboardWidget::StaticClass());
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMainMenuWidget::HandleQuit()
{
	UGeoGameInstance* GameInstance = Cast<UGeoGameInstance>(GetGameInstance());
	if (!ensureMsgf(GameInstance, TEXT("UGeoMainMenuWidget::HandleQuit: GameInstance is not a UGeoGameInstance")))
	{
		return;
	}
	GameInstance->QuitGame();
}
