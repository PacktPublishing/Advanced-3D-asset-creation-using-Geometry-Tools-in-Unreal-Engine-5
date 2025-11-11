#include "ReferenceViewerStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Framework/Application/SlateApplication.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"

TSharedPtr<FSlateStyleSet> FReferenceViewerStyle::StyleInstance = NULL;

void FReferenceViewerStyle::Initialize()
{
    if (!StyleInstance.IsValid())
    {
        StyleInstance = Create();
        FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
    }
}

void FReferenceViewerStyle::Shutdown()
{
    FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
    ensure(StyleInstance.IsUnique());
    StyleInstance.Reset();
}

FName FReferenceViewerStyle::GetStyleSetName()
{
    static FName StyleSetName(TEXT("ReferenceViewerStyle"));
    return StyleSetName;
}

const ISlateStyle& FReferenceViewerStyle::Get()
{
    return *StyleInstance;
}

TSharedRef<FSlateStyleSet> FReferenceViewerStyle::Create()
{
    TSharedRef<FSlateStyleSet> Style = MakeShareable(new FSlateStyleSet("ReferenceViewerStyle"));
    Style->SetContentRoot(IPluginManager::Get().FindPlugin("ReferenceViewer")->GetBaseDir() / TEXT("Resources"));

    Style->Set("ReferenceViewer.OpenPluginWindow", new FSlateImageBrush(Style->RootToContentDir(TEXT("ButtonIcon_40x"), TEXT(".png")), FVector2D(40.f, 40.f)));

    return Style;
}

void FReferenceViewerStyle::ReloadTextures()
{
    if (FSlateApplication::IsInitialized())
    {
        FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
    }
}