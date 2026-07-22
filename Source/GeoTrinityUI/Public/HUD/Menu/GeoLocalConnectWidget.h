// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUD/Menu/GeoMenuPanelWidget.h"

#include "GeoLocalConnectWidget.generated.h"

class UEditableTextBox;
class UGeoMenuButton;
class UTextBlock;
class UWorld;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGeoLocalConnectClosedSignature);

/**
 * "Play Local" panel: direct-IP host/join without Steam, via UGeoSessionSubsystem. Host starts a listen server (the
 * local player is the authority and plays); Join travels to the IP typed in IPInput. LocalIPText shows this machine's
 * IPv4 for the host to read out. Communicates back to the main menu exclusively via the OnClosed delegate, which
 * fires from both BackButton and the panel's back input.
 * Required in the BP hierarchy: UGeoMenuButton "HostButton", "JoinButton", "BackButton",
 * UEditableTextBox "IPInput", UTextBlock "LocalIPText".
 */
UCLASS()
class GEOTRINITYUI_API UGeoLocalConnectWidget : public UGeoMenuPanelWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Session")
	FGeoLocalConnectClosedSignature OnClosed;

	/** Gameplay map the listen-server host travels to. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Session")
	TSoftObjectPtr<UWorld> HostMap;

protected:
	/** Populates LocalIPText with this machine's IPv4 address and wires button delegates. */
	virtual void NativeConstruct() override;
	/** Returns HostButton. */
	virtual UWidget* GetInitialFocusWidget() const override;
	/** Fires OnClosed and consumes the back input. */
	virtual bool HandleBackAction() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> HostButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> JoinButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> BackButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UEditableTextBox> IPInput;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> LocalIPText;

private:
	UFUNCTION()
	void HandleHost();

	UFUNCTION()
	void HandleJoin();

	UFUNCTION()
	void HandleBack();
};
