// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUD/Menu/GeoMenuPageWidget.h"

#include "GeoLocalConnectWidget.generated.h"

class UEditableTextBox;
class UGeoMenuButton;
class UTextBlock;
class UWorld;

/**
 * "Play Local" panel: direct-IP host/join without Steam, via UGeoSessionSubsystem. Host starts a listen server (the
 * local player is the authority and plays); Join travels to the IP typed in IPInput. LocalIPText shows this machine's
 * IPv4 for the host to read out.
 * Required in the BP hierarchy: UGeoMenuButton "HostButton", "JoinButton",
 * UEditableTextBox "IPInput", UTextBlock "LocalIPText".
 */
UCLASS()
class GEOTRINITYUI_API UGeoLocalConnectWidget : public UGeoMenuPageWidget
{
	GENERATED_BODY()

public:
	/** Gameplay map the listen-server host travels to. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoSession")
	TSoftObjectPtr<UWorld> HostMap;

protected:
	/** Populates LocalIPText with this machine's IPv4 address and wires button delegates. */
	virtual void NativeConstruct() override;
	/** Returns HostButton. */
	virtual UWidget* GetInitialFocusWidget() const override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> HostButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> JoinButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UEditableTextBox> IPInput;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> LocalIPText;

private:
	UFUNCTION()
	void HandleHost();

	UFUNCTION()
	void HandleJoin();
};
