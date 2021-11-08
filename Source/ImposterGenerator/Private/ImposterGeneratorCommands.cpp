// malosgao presents.

#include "ImposterGeneratorCommands.h"

#define LOCTEXT_NAMESPACE "FImposterGeneratorModule"

void FImposterGeneratorCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "ImposterGenerator", "Bring up ImposterGenerator window", EUserInterfaceActionType::Button, FInputGesture());
}

#undef LOCTEXT_NAMESPACE
