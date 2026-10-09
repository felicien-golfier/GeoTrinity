// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

/** Details of a UGeoIcon: a Preview row on top drawing the icon as the game does, live while its strokes are edited. */
class FGeoIconCustomization : public IDetailCustomization
{
public:
	/** Returns a new instance of this customization; required by FPropertyEditorModule::RegisterCustomClassLayout. */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** Adds a live Preview row to the top of the Details panel, drawing the icon as the game renders it. */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
	/** Side of the preview, in slate units. */
	static constexpr float PreviewSize = 128.f;
};
