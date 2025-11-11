#include "SReferenceCanvas.h"
#include "Rendering/DrawElements.h"
#include "Framework/Application/SlateApplication.h"

void SReferenceCanvas::Construct(const FArguments& InArgs)
{
    CanvasSize = FVector2D(2000, 2000);
    ViewOffset = FVector2D::ZeroVector;
    ViewZoom = 1.0f;
    CurrentToolMode = EReferenceToolMode::Select;
    bIsDragging = false;
    bIsPanning = false;
    bShowGrid = true;
    GridSize = 20.0f;
    bNeedsRedraw = true;
}

int32 SReferenceCanvas::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, 
    const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, 
    int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
    // Draw background
    FSlateDrawElement::MakeBox(
        OutDrawElements,
        LayerId++,
        AllottedGeometry.ToPaintGeometry(),
        FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"),
        ESlateDrawEffect::None,
        FLinearColor(0.02f, 0.02f, 0.02f, 0.8f)
    );
    
    // Draw grid if enabled
    if (bShowGrid)
    {
        DrawGrid(AllottedGeometry, OutDrawElements, LayerId++);
    }
    
    // Draw images
    DrawImages(AllottedGeometry, OutDrawElements, LayerId);
    LayerId += Images.Num() + 1;
    
    // Draw measurements if in measure mode
    if (CurrentToolMode == EReferenceToolMode::Measure && MeasurePoints.Num() > 0)
    {
        DrawMeasurements(AllottedGeometry, OutDrawElements, LayerId++);
    }
    
    bNeedsRedraw = false;
    return LayerId;
}

void SReferenceCanvas::DrawGrid(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
    const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
    const float AdjustedGridSize = GridSize * ViewZoom;
    
    TArray<FVector2D> GridLines;
    
    // Vertical lines
    for (float X = fmod(-ViewOffset.X, AdjustedGridSize); X < LocalSize.X; X += AdjustedGridSize)
    {
        GridLines.Add(FVector2D(X, 0));
        GridLines.Add(FVector2D(X, LocalSize.Y));
    }
    
    // Horizontal lines
    for (float Y = fmod(-ViewOffset.Y, AdjustedGridSize); Y < LocalSize.Y; Y += AdjustedGridSize)
    {
        GridLines.Add(FVector2D(0, Y));
        GridLines.Add(FVector2D(LocalSize.X, Y));
    }
    
    FSlateDrawElement::MakeLines(
        OutDrawElements,
        LayerId,
        AllottedGeometry.ToPaintGeometry(),
        GridLines,
        ESlateDrawEffect::None,
        FLinearColor(0.5f, 0.5f, 0.5f, 0.2f),
        false,
        1.0f
    );
}

void SReferenceCanvas::DrawImages(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
    for (const auto& Image : Images)
    {
        if (!Image->bVisible || !Image->Texture)
            continue;
            
        // Transform to screen space
        FVector2D ScreenPos = (Image->Position + ViewOffset) * ViewZoom;
        FVector2D ScreenSize = Image->Size * ViewZoom;
        
        // Culling check
        FBox2D ScreenBounds(ScreenPos, ScreenPos + ScreenSize);
        FBox2D ViewBounds(FVector2D::ZeroVector, AllottedGeometry.GetLocalSize());
        if (!ViewBounds.Intersect(ScreenBounds))
            continue;
            
        // Get or create brush
        TSharedPtr<FSlateBrush> Brush = GetOrCreateBrush(Image->Texture);
        if (!Brush.IsValid())
            continue;
            
        // Draw image - create paint geometry properly
        FPaintGeometry ImageGeometry = AllottedGeometry.ToPaintGeometry(
            FVector2D(ScreenSize.X, ScreenSize.Y),  // Size
            FSlateLayoutTransform(ScreenPos)        // Position
        );
        
        FSlateDrawElement::MakeBox(
            OutDrawElements,
            LayerId,
            ImageGeometry,
            Brush.Get(),
            ESlateDrawEffect::None,
            FLinearColor(1, 1, 1, Image->Opacity)
        );
        
        // Draw selection outline
        if (Image->bSelected)
        {
            TArray<FVector2D> BorderPoints = {
                ScreenPos,
                ScreenPos + FVector2D(ScreenSize.X, 0),
                ScreenPos + ScreenSize,
                ScreenPos + FVector2D(0, ScreenSize.Y),
                ScreenPos
            };
            
            FSlateDrawElement::MakeLines(
                OutDrawElements,
                LayerId + 1,
                AllottedGeometry.ToPaintGeometry(),
                BorderPoints,
                ESlateDrawEffect::None,
                FLinearColor(0, 1, 1, 1),
                false,
                2.0f
            );
        }
    }
}

void SReferenceCanvas::DrawMeasurements(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
    if (MeasurePoints.Num() >= 2)
    {
        TArray<FVector2D> LinePoints;
        for (const auto& Point : MeasurePoints)
        {
            LinePoints.Add((Point + ViewOffset) * ViewZoom);
        }
        
        FSlateDrawElement::MakeLines(
            OutDrawElements,
            LayerId,
            AllottedGeometry.ToPaintGeometry(),
            LinePoints,
            ESlateDrawEffect::None,
            FLinearColor(1, 0, 1, 1),
            false,
            2.0f
        );
        
        // Draw distance text
        if (MeasurePoints.Num() == 2)
        {
            // FIXED: Use vector subtraction and Size() instead of non-existent Dist function
            float Distance = (MeasurePoints[1] - MeasurePoints[0]).Size();
            FString DistText = FString::Printf(TEXT("%.1f px"), Distance);
            
            FVector2D MidPoint = ((MeasurePoints[0] + MeasurePoints[1]) * 0.5f + ViewOffset) * ViewZoom;
            
            // FIXED: Use the correct ToPaintGeometry overload
            FPaintGeometry TextGeometry = AllottedGeometry.ToPaintGeometry(
                FVector2D(100, 20),
                FSlateLayoutTransform(MidPoint)
            );
            
            FSlateDrawElement::MakeText(
                OutDrawElements,
                LayerId + 1,
                TextGeometry,
                DistText,
                FCoreStyle::GetDefaultFontStyle("Bold", 12),
                ESlateDrawEffect::None,
                FLinearColor::White
            );
        }
    }
}

FReply SReferenceCanvas::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    FVector2D LocalMousePos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
    FVector2D CanvasPos = LocalMousePos / ViewZoom - ViewOffset;
    
    if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        switch (CurrentToolMode)
        {
            case EReferenceToolMode::Select:
            case EReferenceToolMode::Move:
            {
                TSharedPtr<FRefImage> HitImage = GetImageAtPosition(CanvasPos);
                if (HitImage.IsValid())
                {
                    SelectImage(HitImage, MouseEvent.IsControlDown());
                    bIsDragging = true;
                    DragStartPos = CanvasPos;
                    return FReply::Handled().CaptureMouse(SharedThis(this));
                }
                break;
            }
            
            case EReferenceToolMode::Measure:
            {
                if (MouseEvent.IsControlDown())
                {
                    MeasurePoints.Empty();
                }
                else
                {
                    MeasurePoints.Add(CanvasPos);
                    if (MeasurePoints.Num() > 2)
                    {
                        MeasurePoints.RemoveAt(0);
                    }
                }
                InvalidateCanvas();
                return FReply::Handled();
            }
            // REMOVED ColorPicker case completely
        }
    }
    else if (MouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton)
    {
        bIsPanning = true;
        DragStartPos = LocalMousePos;
        return FReply::Handled().CaptureMouse(SharedThis(this));
    }
    
    return FReply::Unhandled();
}

FReply SReferenceCanvas::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    if (bIsDragging || bIsPanning)
    {
        bIsDragging = false;
        bIsPanning = false;
        return FReply::Handled().ReleaseMouseCapture();
    }
    return FReply::Unhandled();
}

FReply SReferenceCanvas::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    FVector2D LocalMousePos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
    FVector2D CanvasPos = LocalMousePos / ViewZoom - ViewOffset;
    
    if (bIsDragging)
    {
        if (bIsPanning)
        {
            // Pan view
            FVector2D Delta = LocalMousePos - DragStartPos;
            ViewOffset += Delta / ViewZoom;
            DragStartPos = LocalMousePos;
            InvalidateCanvas();
        }
        else if (CurrentToolMode == EReferenceToolMode::Move || CurrentToolMode == EReferenceToolMode::Select)
        {
            // Move selected images
            FVector2D Delta = CanvasPos - LastMousePos;
            if (MouseEvent.IsShiftDown())
            {
                // Constrain to axis
                if (FMath::Abs(Delta.X) > FMath::Abs(Delta.Y))
                    Delta.Y = 0;
                else
                    Delta.X = 0;
            }
            MoveSelectedImages(Delta);
        }
        return FReply::Handled();
    }
    
    LastMousePos = CanvasPos;
    return FReply::Unhandled();
}

FReply SReferenceCanvas::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    if (MouseEvent.IsControlDown())
    {
        // Zoom
        float ZoomDelta = MouseEvent.GetWheelDelta() * 0.1f;
        float NewZoom = FMath::Clamp(ViewZoom + ZoomDelta, 0.1f, 5.0f);
        
        // Zoom to mouse position
        FVector2D LocalMousePos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
        FVector2D PreZoomCanvasPos = LocalMousePos / ViewZoom - ViewOffset;
        ViewZoom = NewZoom;
        FVector2D PostZoomCanvasPos = LocalMousePos / ViewZoom - ViewOffset;
        ViewOffset += PostZoomCanvasPos - PreZoomCanvasPos;
        
        InvalidateCanvas();
        return FReply::Handled();
    }
    return FReply::Unhandled();
}

FReply SReferenceCanvas::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
    if (InKeyEvent.GetKey() == EKeys::G)
    {
        bShowGrid = !bShowGrid;
        InvalidateCanvas();
        return FReply::Handled();
    }
    else if (InKeyEvent.GetKey() == EKeys::M)
    {
        SetToolMode(EReferenceToolMode::Measure);
        return FReply::Handled();
    }
    // REMOVED C key handler for ColorPicker
    else if (InKeyEvent.GetKey() == EKeys::Delete)
    {
        // Remove selected images
        Images.RemoveAll([](const TSharedPtr<FRefImage>& Image) { return Image->bSelected; });
        SelectedImages.Empty();
        InvalidateCanvas();
        return FReply::Handled();
    }
    else if (InKeyEvent.GetKey() == EKeys::Escape)
    {
        SetToolMode(EReferenceToolMode::Select);
        return FReply::Handled();
    }
    // Handle number keys for opacity
    else if (InKeyEvent.GetKey() == EKeys::One || InKeyEvent.GetKey() == EKeys::Two || 
             InKeyEvent.GetKey() == EKeys::Three || InKeyEvent.GetKey() == EKeys::Four ||
             InKeyEvent.GetKey() == EKeys::Five || InKeyEvent.GetKey() == EKeys::Six ||
             InKeyEvent.GetKey() == EKeys::Seven || InKeyEvent.GetKey() == EKeys::Eight ||
             InKeyEvent.GetKey() == EKeys::Nine)
    {
        float NewOpacity = 0.1f;
        
        if (InKeyEvent.GetKey() == EKeys::One) NewOpacity = 0.1f;
        else if (InKeyEvent.GetKey() == EKeys::Two) NewOpacity = 0.2f;
        else if (InKeyEvent.GetKey() == EKeys::Three) NewOpacity = 0.3f;
        else if (InKeyEvent.GetKey() == EKeys::Four) NewOpacity = 0.4f;
        else if (InKeyEvent.GetKey() == EKeys::Five) NewOpacity = 0.5f;
        else if (InKeyEvent.GetKey() == EKeys::Six) NewOpacity = 0.6f;
        else if (InKeyEvent.GetKey() == EKeys::Seven) NewOpacity = 0.7f;
        else if (InKeyEvent.GetKey() == EKeys::Eight) NewOpacity = 0.8f;
        else if (InKeyEvent.GetKey() == EKeys::Nine) NewOpacity = 0.9f;
        
        for (auto& Image : SelectedImages)
        {
            Image->Opacity = NewOpacity;
        }
        InvalidateCanvas();
        return FReply::Handled();
    }
    
    return FReply::Unhandled();
}

void SReferenceCanvas::MoveSelectedImages(const FVector2D& Delta)
{
    for (auto& Image : SelectedImages)
    {
        if (!Image->bLocked)
        {
            Image->Position += Delta;
            if (bShowGrid)
            {
                Image->Position = SnapToGrid(Image->Position);
            }
        }
    }
    InvalidateCanvas();
}

FVector2D SReferenceCanvas::SnapToGrid(const FVector2D& Position) const
{
    return FVector2D(
        FMath::RoundToFloat(Position.X / GridSize) * GridSize,
        FMath::RoundToFloat(Position.Y / GridSize) * GridSize
    );
}

TSharedPtr<FRefImage> SReferenceCanvas::GetImageAtPosition(const FVector2D& Position) const
{
    // Search from top to bottom
    for (int32 i = Images.Num() - 1; i >= 0; i--)
    {
        if (Images[i]->HitTest(Position))
        {
            return Images[i];
        }
    }
    return nullptr;
}

void SReferenceCanvas::SelectImage(TSharedPtr<FRefImage> Image, bool bMultiSelect)
{
    if (!bMultiSelect)
    {
        for (auto& Img : Images)
        {
            Img->bSelected = false;
        }
        SelectedImages.Empty();
    }
    
    Image->bSelected = !Image->bSelected;
    if (Image->bSelected)
    {
        SelectedImages.AddUnique(Image);
    }
    else
    {
        SelectedImages.Remove(Image);
    }
    
    InvalidateCanvas();
}

TSharedPtr<FSlateBrush> SReferenceCanvas::GetOrCreateBrush(UTexture2D* Texture) const
{
    if (!Texture)
        return nullptr;
        
    if (TSharedPtr<FSlateBrush>* ExistingBrush = BrushCache.Find(Texture))
    {
        return *ExistingBrush;
    }
    
    TSharedPtr<FSlateBrush> NewBrush = MakeShareable(new FSlateBrush());
    NewBrush->SetResourceObject(Texture);
    NewBrush->ImageSize = FVector2D(Texture->GetSizeX(), Texture->GetSizeY());
    NewBrush->DrawAs = ESlateBrushDrawType::Image;
    
    BrushCache.Add(Texture, NewBrush);
    return NewBrush;
}

FVector2D SReferenceCanvas::ComputeDesiredSize(float) const
{
    return FVector2D(800, 600);
}

void SReferenceCanvas::AddImage(TSharedPtr<FRefImage> Image)
{
    if (Image.IsValid())
    {
        Images.Add(Image);
        InvalidateCanvas();
    }
}

void SReferenceCanvas::RemoveImage(TSharedPtr<FRefImage> Image)
{
    Images.Remove(Image);
    SelectedImages.Remove(Image);
    InvalidateCanvas();
}

void SReferenceCanvas::ClearImages()
{
    Images.Empty();
    SelectedImages.Empty();
    BrushCache.Empty();
    InvalidateCanvas();
}