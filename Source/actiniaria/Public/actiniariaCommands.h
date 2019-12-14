// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "actiniariaStyle.h"

class FactiniariaCommands : public TCommands<FactiniariaCommands>
{
public:

	FactiniariaCommands()
		: TCommands<FactiniariaCommands>(TEXT("actiniaria"), NSLOCTEXT("Contexts", "actiniaria", "actiniaria Plugin"), NAME_None, FactiniariaStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > PluginAction;
};
