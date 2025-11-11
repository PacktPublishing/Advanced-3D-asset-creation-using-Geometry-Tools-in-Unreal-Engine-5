#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "ReferenceViewerStyle.h"

class FReferenceViewerCommands : public TCommands<FReferenceViewerCommands>
{
public:
    FReferenceViewerCommands()
        : TCommands<FReferenceViewerCommands>(
            TEXT("ReferenceViewer"),
            NSLOCTEXT("Contexts", "ReferenceViewer", "Reference Viewer Plugin"),
            NAME_None,
            FReferenceViewerStyle::GetStyleSetName())
    {
    }

    virtual void RegisterCommands() override;

public:
    TSharedPtr<FUICommandInfo> OpenPluginWindow;
};