// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Framework/Docking/TabManager.h"

class FGasDebuggerModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	/** This function will be bound to Command (by default it will bring up plugin window) */
	void PluginButtonClicked();
	
private:

	void RegisterMenus();

	TSharedRef<class SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);

	TSharedPtr<SWidget> CreateGASCheckTool();

	TSharedPtr<class FUICommandList> PluginCommands;

	TWeakPtr<SDockTab> GameplayCheckEditorTab;

	// Manage all tab controls
	TSharedPtr<FTabManager> GASEditorTabManager;

	// All tab level management
	TSharedPtr<FTabManager::FLayout> GASEditorTabLayout;
};
