#include "ReferenceViewer.h"
#include "ReferenceViewerStyle.h"
#include "ReferenceViewerCommands.h"
#include "SReferenceCanvas.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SSplitter.h"
#include "ToolMenus.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/Paths.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "Framework/Application/SlateApplication.h"
#include "Types/SlateConstants.h"
#include "RefViewerData.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Engine/Texture2D.h"
#include "Misc/FileHelper.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

static const FName ReferenceViewerTabName("ReferenceViewer");

#define LOCTEXT_NAMESPACE "FReferenceViewerModule"

// Main overlay window - DCC-style floating reference panel
class SReferenceOverlay : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SReferenceOverlay) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs)
    {
        ChildSlot
        [
            SNew(SBorder)
            .BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
            .BorderBackgroundColor(FLinearColor(0.1f, 0.1f, 0.1f, 0.95f))
            [
                SNew(SVerticalBox)
                
                // Compact toolbar
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(SBorder)
                    .BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"))
                    .Padding(5)
                    [
                        SNew(SHorizontalBox)
                        
                        // Tool buttons
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        [
                            SNew(SButton)
                            .Text(FText::FromString("Select"))
                            .ButtonStyle(FCoreStyle::Get(), "ToggleButton")
                            .OnClicked(this, &SReferenceOverlay::OnSelectTool)
                            .IsEnabled(this, &SReferenceOverlay::IsSelectToolEnabled)
                        ]
                        
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .Padding(2, 0)
                        [
                            SNew(SButton)
                            .Text(FText::FromString("Measure"))
                            .ButtonStyle(FCoreStyle::Get(), "ToggleButton")
                            .OnClicked(this, &SReferenceOverlay::OnMeasureTool)
                            .IsEnabled(this, &SReferenceOverlay::IsMeasureToolEnabled)
                        ]
                        
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .Padding(10, 0)
                        [
                            SNew(SSeparator)
                            .Orientation(Orient_Vertical)
                        ]
                        
                        // Import button
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        [
                            SNew(SButton)
                            .Text(FText::FromString("Import"))
                            .OnClicked(this, &SReferenceOverlay::OnImportClicked)
                        ]
                        
                        // Clear button
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .Padding(2, 0)
                        [
                            SNew(SButton)
                            .Text(FText::FromString("Clear"))
                            .OnClicked(this, &SReferenceOverlay::OnClearClicked)
                        ]
                        
                        + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        [
                            SNew(SSpacer)
                        ]
                        
                        // Grid toggle
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        [
                            SNew(SCheckBox)
                            .IsChecked(this, &SReferenceOverlay::GetGridEnabledState)
                            .OnCheckStateChanged(this, &SReferenceOverlay::OnGridEnabledChanged)
                            [
                                SNew(STextBlock)
                                .Text(FText::FromString("Grid"))
                                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
                            ]
                        ]
                        
                        // Grid size
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .Padding(5, 0)
                        [
                            SNew(SBox)
                            .WidthOverride(60)
                            [
                                SNew(SSpinBox<float>)
                                .Value(this, &SReferenceOverlay::GetGridSize)
                                .OnValueChanged(this, &SReferenceOverlay::SetGridSize)
                                .MinValue(5.0f)
                                .MaxValue(100.0f)
                                .Delta(5.0f)
                                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
                            ]
                        ]
                        
                        // Window opacity
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .Padding(10, 0)
                        [
                            SNew(SSeparator)
                            .Orientation(Orient_Vertical)
                        ]
                        
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        [
                            SNew(STextBlock)
                            .Text(FText::FromString("Opacity:"))
                            .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
                        ]
                        
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .Padding(5, 0)
                        [
                            SNew(SBox)
                            .WidthOverride(100)
                            [
                                SNew(SSlider)
                                .Value(this, &SReferenceOverlay::GetWindowOpacity)
                                .OnValueChanged(this, &SReferenceOverlay::SetWindowOpacity)
                                .MinValue(0.3f)
                                .MaxValue(1.0f)
                            ]
                        ]
                    ]
                ]
                
                // Main canvas area
                + SVerticalBox::Slot()
                .FillHeight(1.0f)
                [
                    SAssignNew(Canvas, SReferenceCanvas)
                ]
                
                // Minimal status bar
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(SBorder)
                    .BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"))
                    .Padding(5, 2)
                    [
                        SNew(SHorizontalBox)
                        
                        + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        [
                            SNew(STextBlock)
                            .Text(this, &SReferenceOverlay::GetStatusText)
                            .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
                            .ColorAndOpacity(FSlateColor(FLinearColor(0.8f, 0.8f, 0.8f)))
                        ]
                        
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        [
                            SNew(STextBlock)
                            .Text(FText::FromString("Middle Mouse: Pan | Ctrl+Scroll: Zoom | G: Grid | 1-9: Opacity"))
                            .Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
                            .ColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f)))
                        ]
                    ]
                ]
            ]
        ];
        
        CurrentToolMode = EReferenceToolMode::Select;
        WindowOpacity = 1.0f;
        GridSize = 20.0f;
        bGridEnabled = true;
    }

    void AddImage(TSharedPtr<FRefImage> Image)
    {
        if (Canvas.IsValid())
        {
            Canvas->AddImage(Image);
        }
    }
    
    // Handle key input at the overlay level
    virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override
    {
        // Pass to canvas first
        if (Canvas.IsValid())
        {
            return Canvas->OnKeyDown(MyGeometry, InKeyEvent);
        }
        return FReply::Unhandled();
    }

private:
    TSharedPtr<SReferenceCanvas> Canvas;
    EReferenceToolMode CurrentToolMode;
    float WindowOpacity;
    float GridSize;
    bool bGridEnabled;
    
    // Tool selection
    FReply OnSelectTool()
    {
        CurrentToolMode = EReferenceToolMode::Select;
        if (Canvas.IsValid())
            Canvas->SetToolMode(CurrentToolMode);
        return FReply::Handled();
    }
    
    FReply OnMeasureTool()
    {
        CurrentToolMode = EReferenceToolMode::Measure;
        if (Canvas.IsValid())
            Canvas->SetToolMode(CurrentToolMode);
        return FReply::Handled();
    }
    
    bool IsSelectToolEnabled() const
    {
        return CurrentToolMode != EReferenceToolMode::Select;
    }
    
    bool IsMeasureToolEnabled() const
    {
        return CurrentToolMode != EReferenceToolMode::Measure;
    }
    
    // Grid controls
    ECheckBoxState GetGridEnabledState() const
    {
        return bGridEnabled ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
    }
    
    void OnGridEnabledChanged(ECheckBoxState NewState)
    {
        bGridEnabled = (NewState == ECheckBoxState::Checked);
        if (Canvas.IsValid())
        {
            Canvas->SetGridEnabled(bGridEnabled);
        }
    }
    
    float GetGridSize() const { return GridSize; }
    void SetGridSize(float NewSize)
    {
        GridSize = NewSize;
        if (Canvas.IsValid())
        {
            Canvas->SetGridSize(GridSize);
        }
    }
    
    // Window opacity
    float GetWindowOpacity() const { return WindowOpacity; }
    void SetWindowOpacity(float NewOpacity)
    {
        WindowOpacity = NewOpacity;
        if (TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(SharedThis(this)))
        {
            ParentWindow->SetOpacity(WindowOpacity);
        }
    }
    
    // Status text
    FText GetStatusText() const
    {
        if (Canvas.IsValid())
        {
            switch (CurrentToolMode)
            {
                case EReferenceToolMode::Select:
                    return FText::FromString("Select Mode - Click to select, drag to move");
                case EReferenceToolMode::Measure:
                    return FText::FromString("Measure Mode - Click to place measurement points");
                default:
                    return FText::GetEmpty();
            }
        }
        return FText::GetEmpty();
    }
    
    // Import/Clear
    FReply OnImportClicked()
    {
        IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
        if (DesktopPlatform)
        {
            TArray<FString> OpenFilenames;
            const FString DefaultPath = FPaths::ProjectDir();
            
            bool bOpened = DesktopPlatform->OpenFileDialog(
                FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
                TEXT("Select Reference Images"),
                DefaultPath,
                TEXT(""),
                TEXT("Image Files (*.png;*.jpg;*.jpeg;*.bmp)|*.png;*.jpg;*.jpeg;*.bmp"),
                EFileDialogFlags::Multiple,
                OpenFilenames
            );

            if (bOpened)
            {
                for (const FString& Filename : OpenFilenames)
                {
                    LoadImageFile(Filename);
                }
            }
        }
        return FReply::Handled();
    }
    
    FReply OnClearClicked()
    {
        if (Canvas.IsValid())
        {
            Canvas->ClearImages();
        }
        return FReply::Handled();
    }
    
    void LoadImageFile(const FString& FilePath)
    {
        IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
        
        TArray<uint8> RawFileData;
        if (FFileHelper::LoadFileToArray(RawFileData, *FilePath))
        {
            EImageFormat Format = ImageWrapperModule.DetectImageFormat(RawFileData.GetData(), RawFileData.Num());
            if (Format != EImageFormat::Invalid)
            {
                TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(Format);
                if (ImageWrapper.IsValid() && ImageWrapper->SetCompressed(RawFileData.GetData(), RawFileData.Num()))
                {
                    TArray<uint8> UncompressedBGRA;
                    if (ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, UncompressedBGRA))
                    {
                        TSharedPtr<FRefImage> NewImage = MakeShareable(new FRefImage());
                        NewImage->FilePath = FilePath;
                        NewImage->Name = FPaths::GetBaseFilename(FilePath);
                        
                        // Create texture
                        UTexture2D* NewTexture = UTexture2D::CreateTransient(
                            ImageWrapper->GetWidth(),
                            ImageWrapper->GetHeight(),
                            PF_B8G8R8A8
                        );
                        
                        if (NewTexture)
                        {
                            void* TextureData = NewTexture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
                            FMemory::Memcpy(TextureData, UncompressedBGRA.GetData(), UncompressedBGRA.Num());
                            NewTexture->GetPlatformData()->Mips[0].BulkData.Unlock();
                            NewTexture->UpdateResource();
                            
                            NewImage->Texture = NewTexture;
                            NewImage->Size = FVector2D(ImageWrapper->GetWidth(), ImageWrapper->GetHeight());
                            
                            // Position in center of canvas
                            NewImage->Position = FVector2D(
                                400 - NewImage->Size.X / 2,
                                300 - NewImage->Size.Y / 2
                            );
                            
                            AddImage(NewImage);
                        }
                    }
                }
            }
        }
    }
};

// Main tab widget
class SReferenceViewerTab : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SReferenceViewerTab) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs)
    {
        ChildSlot
        [
            SNew(SBorder)
            .Padding(10)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(5)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString("Reference Viewer - DCC Style"))
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
                ]
                
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(5)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    [
                        SNew(SButton)
                        .Text(FText::FromString("Open Floating Reference Panel"))
                        .OnClicked(this, &SReferenceViewerTab::OnOpenOverlayClicked)
                    ]
                    
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .Padding(10, 0)
                    [
                        SNew(SButton)
                        .Text(FText::FromString("Open Docked Panel"))
                        .OnClicked(this, &SReferenceViewerTab::OnOpenDockedClicked)
                    ]
                ]
                
                + SVerticalBox::Slot()
                .FillHeight(1.0f)
                .Padding(5)
                [
                    SNew(SBorder)
                    .BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"))
                    [
                        SNew(SBox)
                        .VAlign(VAlign_Center)
                        .HAlign(HAlign_Center)
                        [
                            SNew(SVerticalBox)
                            + SVerticalBox::Slot()
                            .AutoHeight()
                            [
                                SNew(STextBlock)
                                .Text(FText::FromString("Professional Reference Overlay System"))
                                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
                                .Justification(ETextJustify::Center)
                            ]
                            
                            + SVerticalBox::Slot()
                            .AutoHeight()
                            .Padding(0, 20)
                            [
                                SNew(STextBlock)
                                .Text(FText::FromString("Features: High-performance custom canvas | Drag & drop image arrangement | Grid snapping system | Measurement tools | Opacity controls (1-9 keys) | Pan & zoom navigation"))
                                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
                                .Justification(ETextJustify::Left)
                            ]
                        ]
                    ]
                ]
                
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(5)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString("Reference Viewer by Barros Creations"))
                    .Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
                    .ColorAndOpacity(FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f)))
                    .Justification(ETextJustify::Center)
                ]
            ]
        ];
    }

private:
    FReply OnOpenOverlayClicked()
    {
        // Create floating window with specific DCC-style properties
        TSharedRef<SWindow> OverlayWindow = SNew(SWindow)
            .Title(FText::FromString("Reference Panel"))
            .ClientSize(FVector2D(1000, 700))
            .SupportsMaximize(false)
            .SupportsMinimize(true)
            .SizingRule(ESizingRule::UserSized)
            .IsTopmostWindow(true)
            .FocusWhenFirstShown(true)
            .HasCloseButton(true)
            .SupportsTransparency(EWindowTransparency::PerWindow)
            .InitialOpacity(1.0f)
            .CreateTitleBar(true)
            .AutoCenter(EAutoCenter::PreferredWorkArea)
            [
                SNew(SReferenceOverlay)
            ];
            
        FSlateApplication::Get().AddWindow(OverlayWindow);
        
        return FReply::Handled();
    }
    
    FReply OnOpenDockedClicked()
    {
        // Create a docked version that can be tabbed with other panels
        TSharedRef<SDockTab> NewTab = SNew(SDockTab)
            .TabRole(ETabRole::NomadTab)
            .Label(FText::FromString("Reference Panel"))
            [
                SNew(SReferenceOverlay)
            ];
            
        // Add as a new tab - use the correct API
        FGlobalTabmanager::Get()->InsertNewDocumentTab(
            "LevelEditorTab",
            FTabManager::ESearchPreference::RequireClosedTab,
            NewTab
        );
        
        return FReply::Handled();
    }
};

void FReferenceViewerModule::StartupModule()
{
    FReferenceViewerStyle::Initialize();
    FReferenceViewerStyle::ReloadTextures();
    
    FReferenceViewerCommands::Register();
    
    PluginCommands = MakeShareable(new FUICommandList);

    PluginCommands->MapAction(
        FReferenceViewerCommands::Get().OpenPluginWindow,
        FExecuteAction::CreateRaw(this, &FReferenceViewerModule::PluginButtonClicked),
        FCanExecuteAction());

    UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FReferenceViewerModule::RegisterMenus));
    
    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(ReferenceViewerTabName, FOnSpawnTab::CreateRaw(this, &FReferenceViewerModule::OnSpawnPluginTab))
        .SetDisplayName(LOCTEXT("FReferenceViewerTabTitle", "Reference Viewer"))
        .SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FReferenceViewerModule::ShutdownModule()
{
    UToolMenus::UnRegisterStartupCallback(this);
    UToolMenus::UnregisterOwner(this);
    FReferenceViewerStyle::Shutdown();
    FReferenceViewerCommands::Unregister();
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ReferenceViewerTabName);
}

FString FReferenceViewerModule::GetSavedLayoutsPath()
{
    return FPaths::ProjectSavedDir() / TEXT("ReferenceViewer") / TEXT("Layouts");
}

TSharedRef<SDockTab> FReferenceViewerModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        [
            SNew(SReferenceViewerTab)
        ];
}

void FReferenceViewerModule::PluginButtonClicked()
{
    FGlobalTabmanager::Get()->TryInvokeTab(ReferenceViewerTabName);
}

void FReferenceViewerModule::RegisterMenus()
{
    FToolMenuOwnerScoped OwnerScoped(this);

    UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
    {
        FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
        Section.AddMenuEntryWithCommandList(FReferenceViewerCommands::Get().OpenPluginWindow, PluginCommands);
    }
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FReferenceViewerModule, ReferenceViewer)