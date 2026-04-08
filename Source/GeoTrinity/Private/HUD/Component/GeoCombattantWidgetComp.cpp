// Copyright 2024 GeoTrinity. All Rights Reserved.


#include "HUD/Component/GeoCombattantWidgetComp.h"

#include "AbilitySystemInterface.h"
#include "GeoTrinity/GeoTrinity.h"
#include "HUD/GenericCombattantWidget.h"


void UGeoCombattantWidgetComp::PostInitProperties()
{
	Super::PostInitProperties();

	if (GetWidgetClass() && !GetWidgetClass()->IsChildOf(UGenericCombattantWidget::StaticClass()))
	{
		UE_LOG(LogGeoASC, Error,
			   TEXT("You may want to use a widget class derived from UGenericCombattantWidget for %s"), *GetName());
	}
}


// Called when the game starts
void UGeoCombattantWidgetComp::BeginPlay()
{
	Super::BeginPlay();

	AActor const* OwnerActor = GetOwner();
	if (OwnerActor->Implements<UAbilitySystemInterface>())
	{
		UAbilitySystemComponent* ASC = Cast<IAbilitySystemInterface>(OwnerActor)->GetAbilitySystemComponent();
		if (UGenericCombattantWidget* CombattantWidget = Cast<UGenericCombattantWidget>(GetUserWidgetObject()))
		{
			CombattantWidget->InitializeWithAbilitySystemComponent(ASC);
		}
	}
	else
	{
		UE_LOG(LogGeoASC, Warning,
			   TEXT("IAbilitySystemInterface was not implemented on owner actor %s for %s. No stat will be displayed"),
			   *OwnerActor->GetName(), *GetName());
	}
}


void UGeoCombattantWidgetComp::EndPlay(EEndPlayReason::Type const EndPlayReason)
{
	if (UGenericCombattantWidget* CombattantWidget = Cast<UGenericCombattantWidget>(GetUserWidgetObject()))
	{
		CombattantWidget->UnbindStatCallbacks();
	}
	Super::EndPlay(EndPlayReason);
}

// Called every frame
void UGeoCombattantWidgetComp::TickComponent(float DeltaTime, ELevelTick TickType,
											 FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}
