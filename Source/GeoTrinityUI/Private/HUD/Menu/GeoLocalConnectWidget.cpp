// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "HUD/Menu/GeoLocalConnectWidget.h"

#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "HUD/Menu/GeoMenuButton.h"
#include "System/GeoSessionSubsystem.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoLocalConnectWidget::NativeConstruct()
{
	Super::NativeConstruct();

	HostButton->OnClicked.AddUniqueDynamic(this, &UGeoLocalConnectWidget::HandleHost);
	JoinButton->OnClicked.AddUniqueDynamic(this, &UGeoLocalConnectWidget::HandleJoin);
	BackButton->OnClicked.AddUniqueDynamic(this, &UGeoLocalConnectWidget::HandleBack);

	UGeoSessionSubsystem const* Session = GetGameInstance()->GetSubsystem<UGeoSessionSubsystem>();
	if (ensureMsgf(Session, TEXT("%hs: GeoSessionSubsystem missing"), __FUNCTION__))
	{
		LocalIPText->SetText(FText::FromString(FString::Printf(TEXT("Your IP: %s"), *Session->GetLocalIPv4())));
	}
}

// ---------------------------------------------------------------------------------------------------------------------
UWidget* UGeoLocalConnectWidget::GetInitialFocusWidget() const
{
	return HostButton;
}

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoLocalConnectWidget::HandleBackAction()
{
	HandleBack();
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoLocalConnectWidget::HandleHost()
{
	UGeoSessionSubsystem* Session = GetGameInstance()->GetSubsystem<UGeoSessionSubsystem>();
	if (!ensureMsgf(Session, TEXT("UGeoLocalConnectWidget::HandleHost: GeoSessionSubsystem missing"))
		|| !ensureMsgf(!HostMap.IsNull(), TEXT("UGeoLocalConnectWidget::HandleHost: HostMap is not set")))
	{
		return;
	}
	Session->HostListen(HostMap.ToSoftObjectPath().GetLongPackageName());
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoLocalConnectWidget::HandleJoin()
{
	UGeoSessionSubsystem* Session = GetGameInstance()->GetSubsystem<UGeoSessionSubsystem>();
	if (!ensureMsgf(Session, TEXT("UGeoLocalConnectWidget::HandleJoin: GeoSessionSubsystem missing")))
	{
		return;
	}
	Session->JoinByAddress(IPInput->GetText().ToString());
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoLocalConnectWidget::HandleBack()
{
	OnClosed.Broadcast();
}
