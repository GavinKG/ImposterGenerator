// malosgao presents.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "ImposterGeneratorStyle.h"

class FImposterGeneratorCommands : public TCommands<FImposterGeneratorCommands>
{
public:

	FImposterGeneratorCommands()
		: TCommands<FImposterGeneratorCommands>(TEXT("ImposterGenerator"), NSLOCTEXT("Contexts", "ImposterGenerator", "ImposterGenerator Plugin"), NAME_None, FImposterGeneratorStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenPluginWindow;
};