#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"

// Optimized image data structure
struct FRefImage
{
    // Basic data
    FString Name;
    FString FilePath;
    UTexture2D* Texture;
    
    // Transform
    FVector2D Position;
    FVector2D Size;
    float Rotation;
    float Opacity;
    
    // State
    bool bSelected;
    bool bLocked;
    bool bVisible;
    
    // Cached render data
    TSharedPtr<FSlateBrush> CachedBrush;
    FBox2D CachedBounds;
    
    FRefImage() 
        : Texture(nullptr)
        , Position(FVector2D::ZeroVector)
        , Size(FVector2D(200, 200))
        , Rotation(0.0f)
        , Opacity(1.0f)
        , bSelected(false)
        , bLocked(false)
        , bVisible(true)
    {}
    
    FBox2D GetBounds() const
    {
        return FBox2D(Position, Position + Size);
    }
    
    bool HitTest(const FVector2D& Point) const
    {
        return GetBounds().IsInside(Point);
    }
};

// Layout save data
struct FReferenceLayout
{
    FString Name;
    TArray<FRefImage> Images;
    FVector2D CanvasSize;
    float GridSize;
    bool bGridEnabled;
};

// Tool modes
enum class EReferenceToolMode : uint8
{
    Select,
    Move,
    Measure
};