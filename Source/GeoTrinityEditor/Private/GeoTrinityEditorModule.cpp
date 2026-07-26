// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "GeoTrinityEditorModule.h"

#include "Actor/Projectile/ExternalProjectileParams.h"
#include "Detail/ExternalProjectileParamsCustomization.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"

IMPLEMENT_MODULE(FGeoTrinityEditorModule, GeoTrinityEditor)

void FGeoTrinityEditorModule::StartupModule()
{
	FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyEditor.RegisterCustomPropertyTypeLayout(
		FExternalProjectileParams::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FExternalProjectileParamsCustomization::MakeInstance));
	PropertyEditor.NotifyCustomizationModuleChanged();
}

void FGeoTrinityEditorModule::ShutdownModule()
{
	if (!FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		return;
	}

	FPropertyEditorModule& PropertyEditor = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyEditor.UnregisterCustomPropertyTypeLayout(FExternalProjectileParams::StaticStruct()->GetFName());
	PropertyEditor.NotifyCustomizationModuleChanged();
}
