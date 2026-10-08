// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "GeoTrinityEditorModule.h"

#include "AbilitySystem/Data/GeoFXMoment.h"
#include "AbilitySystem/Data/GeoSoundRow.h"
#include "Actor/Projectile/ExternalProjectileParams.h"
#include "Detail/ExternalProjectileParamsCustomization.h"
#include "Detail/GeoCurveSourceCustomization.h"
#include "Detail/GeoIconCustomization.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "Thumbnail/GeoIconThumbnailRenderer.h"
#include "ThumbnailRendering/ThumbnailManager.h"
#include "Tool/GeoIcon.h"

IMPLEMENT_MODULE(FGeoTrinityEditorModule, GeoTrinityEditor)

void FGeoTrinityEditorModule::StartupModule()
{
	FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyEditor.RegisterCustomPropertyTypeLayout(
		FExternalProjectileParams::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FExternalProjectileParamsCustomization::MakeInstance));
	PropertyEditor.RegisterCustomPropertyTypeLayout(
		FGeoSoundEntry::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FGeoCurveSourceCustomization::MakeInstance));
	PropertyEditor.RegisterCustomPropertyTypeLayout(
		FGeoVFXParams::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FGeoCurveSourceCustomization::MakeInstance));
	PropertyEditor.RegisterCustomPropertyTypeLayout(
		FGeoBurstVFXParams::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FGeoCurveSourceCustomization::MakeInstance));
	PropertyEditor.RegisterCustomClassLayout(
		UGeoIcon::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(&FGeoIconCustomization::MakeInstance));
	PropertyEditor.NotifyCustomizationModuleChanged();

	UThumbnailManager::Get().RegisterCustomRenderer(UGeoIcon::StaticClass(), UGeoIconThumbnailRenderer::StaticClass());
}

void FGeoTrinityEditorModule::ShutdownModule()
{
	if (UObjectInitialized())
	{
		UThumbnailManager::Get().UnregisterCustomRenderer(UGeoIcon::StaticClass());
	}

	if (!FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		return;
	}

	FPropertyEditorModule& PropertyEditor = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyEditor.UnregisterCustomPropertyTypeLayout(FExternalProjectileParams::StaticStruct()->GetFName());
	PropertyEditor.UnregisterCustomPropertyTypeLayout(FGeoSoundEntry::StaticStruct()->GetFName());
	PropertyEditor.UnregisterCustomPropertyTypeLayout(FGeoVFXParams::StaticStruct()->GetFName());
	PropertyEditor.UnregisterCustomPropertyTypeLayout(FGeoBurstVFXParams::StaticStruct()->GetFName());
	PropertyEditor.UnregisterCustomClassLayout(UGeoIcon::StaticClass()->GetFName());
	PropertyEditor.NotifyCustomizationModuleChanged();
}
