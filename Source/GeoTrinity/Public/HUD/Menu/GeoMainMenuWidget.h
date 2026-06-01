// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"

#include "GeoMainMenuWidget.generated.h"

class UGeoMenuButton;

/**
 * Main lobby menu widget. Composes three UGeoMenuButton instances and handles all action logic in C++.
 * Blueprint subclasses configure appearance through the button UPROPERTYs and set GameMapURL + MaxPublicConnections.
 * Required in the BP hierarchy: UGeoMenuButton widgets named "CreateServerButton", "JoinServerButton", "QuitButton".
 */
UCLASS()
class GEOTRINITY_API UGeoMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Session")
	FString GameMapURL;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Session")
	int32 MaxPublicConnections = 4;

	UFUNCTION(BlueprintCallable, Category = "Session")
	FString GetLocalPlayerName() const;

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> CreateServerButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> JoinServerButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UGeoMenuButton> QuitButton;

private:
	UFUNCTION()
	void HandleCreateServer();

	UFUNCTION()
	void HandleJoinServer();

	UFUNCTION()
	void HandleQuit();

	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);

	FDelegateHandle CreateSessionDelegateHandle;
};
