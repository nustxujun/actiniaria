// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "actiniaria.h"
#include "actiniariaStyle.h"
#include "actiniariaCommands.h"
#include "Misc/MessageDialog.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

#include "LevelEditor.h"
#include "IPCFrame.h"
#include "ActiniariaFrame.h"
#include <thread>


static const FName actiniariaTabName("actiniaria");

#define LOCTEXT_NAMESPACE "FactiniariaModule"

void FactiniariaModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FactiniariaStyle::Initialize();
	FactiniariaStyle::ReloadTextures();

	FactiniariaCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FactiniariaCommands::Get().PluginAction,
		FExecuteAction::CreateRaw(this, &FactiniariaModule::PluginButtonClicked),
		FCanExecuteAction());
		
	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	
	{
		TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
		MenuExtender->AddMenuExtension("WindowLayout", EExtensionHook::After, PluginCommands, FMenuExtensionDelegate::CreateRaw(this, &FactiniariaModule::AddMenuExtension));

		LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);
	}
	
	{
		TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
		ToolbarExtender->AddToolBarExtension("Settings", EExtensionHook::After, PluginCommands, FToolBarExtensionDelegate::CreateRaw(this, &FactiniariaModule::AddToolbarExtension));
		
		LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);
	}
}

void FactiniariaModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	FactiniariaStyle::Shutdown();

	FactiniariaCommands::Unregister();
}

void FactiniariaModule::PluginButtonClicked()
{
	ShowWindow(::GetActiveWindow(), SW_MINIMIZE);


	{
		auto thread = std::thread([]() {
			ActiniariaFrame frame;
			frame.init();
			frame.update();
		});

		IPCFrame frame;
		frame.init();

		thread.join();
	}


	//while(true)
	//{
	//	IPCFrame frame;
	//	frame.init();
	//}

}

void FactiniariaModule::AddMenuExtension(FMenuBuilder& Builder)
{
	Builder.AddMenuEntry(FactiniariaCommands::Get().PluginAction);
}

void FactiniariaModule::AddToolbarExtension(FToolBarBuilder& Builder)
{
	Builder.AddToolBarButton(FactiniariaCommands::Get().PluginAction);
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FactiniariaModule, actiniaria)