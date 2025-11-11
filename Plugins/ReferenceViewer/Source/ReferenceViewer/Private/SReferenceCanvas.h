#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"
#include "RefViewerData.h"

// High-performance custom canvas widget
class SReferenceCanvas : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SReferenceCanvas) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    
    // Rendering
    virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, 
        const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, 
        int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
    
    virtual FVector2D ComputeDesiredSize(float) const override;
    
    // Make widget interactive
    virtual bool SupportsKeyboardFocus() const override { return true; }
    
    // Input handling
    virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
    virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
    virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
    virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
    virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
    
    // Image management
    void AddImage(TSharedPtr<FRefImage> Image);
    void RemoveImage(TSharedPtr<FRefImage> Image);
    void ClearImages();
    
    // Tool modes
    void SetToolMode(EReferenceToolMode Mode) { CurrentToolMode = Mode; }
    EReferenceToolMode GetToolMode() const { return CurrentToolMode; }
    
    // Grid
    void SetGridEnabled(bool bEnabled) { bShowGrid = bEnabled; }
    void SetGridSize(float Size) { GridSize = Size; }
    
    // Performance
    void InvalidateCanvas() { bNeedsRedraw = true; }
    
private:
    // Images
    TArray<TSharedPtr<FRefImage>> Images;
    TArray<TSharedPtr<FRefImage>> SelectedImages;
    
    // Canvas state
    FVector2D CanvasSize;
    FVector2D ViewOffset;
    float ViewZoom;
    
    // Interaction state
    EReferenceToolMode CurrentToolMode;
    bool bIsDragging;
    bool bIsPanning;
    FVector2D DragStartPos;
    FVector2D LastMousePos;
    
    // Grid
    bool bShowGrid;
    float GridSize;
    
    // Measurement
    TArray<FVector2D> MeasurePoints;
    
    // Performance
    mutable bool bNeedsRedraw;
    mutable TMap<UTexture2D*, TSharedPtr<FSlateBrush>> BrushCache;
    
    // Helper functions
    void DrawGrid(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const;
    void DrawImages(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const;
    void DrawMeasurements(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const;
    
    FVector2D SnapToGrid(const FVector2D& Position) const;
    TSharedPtr<FRefImage> GetImageAtPosition(const FVector2D& Position) const;
    void SelectImage(TSharedPtr<FRefImage> Image, bool bMultiSelect);
    void MoveSelectedImages(const FVector2D& Delta);
    
    TSharedPtr<FSlateBrush> GetOrCreateBrush(UTexture2D* Texture) const;
};