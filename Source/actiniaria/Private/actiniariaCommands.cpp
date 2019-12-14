// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "actiniariaCommands.h"

#define LOCTEXT_NAMESPACE "FactiniariaModule"

void FactiniariaCommands::RegisterCommands()
{
	UI_COMMAND(PluginAction, "actiniaria", "Execute actiniaria action", EUserInterfaceActionType::Button, FInputGesture());
}

#undef LOCTEXT_NAMESPACE
