#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FReferenceViewerModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
    
    void PluginButtonClicked();
    void OpenOverlayWindow();
    
    static FString GetSavedLayoutsPath();
    
private:
    void RegisterMenus();
    TSharedRef<class SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);
    
    TSharedPtr<class FUICommandList> PluginCommands;
    TSharedPtr<class SWindow> OverlayWindow;
};