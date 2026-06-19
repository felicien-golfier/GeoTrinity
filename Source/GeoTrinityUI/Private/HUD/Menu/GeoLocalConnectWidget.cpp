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

	if (!HostButton)
	{
		ensureMsgf(HostButton, TEXT("UGeoLocalConnectWidget: HostButton is not bound"));
		return;
	}
	if (!JoinButton)
	{
		ensureMsgf(JoinButton, TEXT("UGeoLocalConnectWidget: JoinButton is not bound"));
		return;
	}
	if (!BackButton)
	{
		ensureMsgf(BackButton, TEXT("UGeoLocalConnectWidget: BackButton is not bound"));
		return;
	}
	if (!IPInput)
	{
		ensureMsgf(IPInput, TEXT("UGeoLocalConnectWidget: IPInput is not bound"));
		return;
	}
	if (!LocalIPText)
	{
		ensureMsgf(LocalIPText, TEXT("UGeoLocalConnectWidget: LocalIPText is not bound"));
		return;
	}

	HostButton->OnClicked.AddDynamic(this, &UGeoLocalConnectWidget::HandleHost);
	JoinButton->OnClicked.AddDynamic(this, &UGeoLocalConnectWidget::HandleJoin);
	BackButton->OnClicked.AddDynamic(this, &UGeoLocalConnectWidget::HandleBack);

	UGeoSessionSubsystem const* Session = GetGameInstance()->GetSubsystem<UGeoSessionSubsystem>();
	if (ensureMsgf(Session, TEXT("UGeoLocalConnectWidget: GeoSessionSubsystem missing")))
	{
		LocalIPText->SetText(FText::FromString(FString::Printf(TEXT("Your IP: %s"), *Session->GetLocalIPv4())));
	}
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
