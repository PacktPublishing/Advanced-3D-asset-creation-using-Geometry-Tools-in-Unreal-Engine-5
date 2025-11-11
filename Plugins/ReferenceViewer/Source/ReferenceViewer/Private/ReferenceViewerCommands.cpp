#include "ReferenceViewerCommands.h"

#define LOCTEXT_NAMESPACE "FReferenceViewerModule"

void FReferenceViewerCommands::RegisterCommands()
{
    UI_COMMAND(OpenPluginWindow, "Reference Viewer", "Open the Reference Viewer window", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE