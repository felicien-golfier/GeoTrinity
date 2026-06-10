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


// The engine creates the UserWidget here as part of the component's normal lifecycle (and re-creates it whenever the
// screen-space layer rebuilds). Binding from inside this override means the widget is always bound right after it
// exists, on whatever schedule the engine chooses — no need to force creation early.
void UGeoCombattantWidgetComp::InitWidget()
{
	Super::InitWidget();
	BindWidgetToOwnerASC();
}

void UGeoCombattantWidgetComp::InitializeForOwner()
{
	// Re-bind once the owner's ASC is ready (the ASC may be null when the widget was first created — players get theirs
	// later via InitGAS/OnRep_PlayerState). Does nothing if the widget doesn't exist yet; InitWidget binds it then.
	BindWidgetToOwnerASC();
}

void UGeoCombattantWidgetComp::BindWidgetToOwnerASC()
{
	UGenericCombattantWidget* CombattantWidget = Cast<UGenericCombattantWidget>(GetUserWidgetObject());
	if (!CombattantWidget)
	{
		return;
	}

	AActor const* OwnerActor = GetOwner();
	if (!OwnerActor->Implements<UAbilitySystemInterface>())
	{
		UE_LOG(LogGeoASC, Warning,
			   TEXT("IAbilitySystemInterface was not implemented on owner actor %s for %s. No stat will be displayed"),
			   *OwnerActor->GetName(), *GetName());
		return;
	}

	// A null ASC is expected before GAS init (re-bound later via InitializeForOwner), so it is a no-op, not an error.
	if (UAbilitySystemComponent* ASC = Cast<IAbilitySystemInterface>(OwnerActor)->GetAbilitySystemComponent())
	{
		CombattantWidget->InitializeWithAbilitySystemComponent(ASC);
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
