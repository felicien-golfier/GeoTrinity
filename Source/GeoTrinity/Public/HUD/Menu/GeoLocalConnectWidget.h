// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"

#include "GeoLocalConnectWidget.generated.h"

class UEditableTextBox;
class UGeoMenuButton;
class UTextBlock;
class UWorld;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGeoLocalConnectClosedSignature);

/**
 * "Play Local" panel: direct-IP host/join without Steam, via UGeoSessionSubsystem. Host launches a dedicated server
 * behind and joins it as a pure client; Join travels to the IP typed in IPInput. LocalIPText shows this machine's IPv4
 * for the host to read out. Communicates back to the main menu exclusively via the OnClosed delegate.
 * Required in the BP hierarchy: UGeoMenuButton "HostButton", "JoinButton", "BackButton",
 * UEditableTextBox "IPInput", UTextBlock "LocalIPText".
 */
UCLASS()
class GEOTRINITY_API UGeoLocalConnectWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Session")
	FGeoLocalConnectClosedSignature OnClosed;

	/** Gameplay map the launched dedicated server loads. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Session")
	TSoftObjectPtr<UWorld> HostMap;

protected:
	virtual void NativeConstruct() override;

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
