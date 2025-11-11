using UnrealBuildTool;

public class ReferenceViewer : ModuleRules
{
    public ReferenceViewer(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        
        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
            }
        );
        
        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "EditorSubsystem",
                "UnrealEd",
                "ToolMenus",
                "EditorWidgets",
                "InputCore",
                "Projects",
                "DesktopPlatform",
                "ImageWrapper",
                "RenderCore",
                "RHI",
                "ApplicationCore",
                "Json",
                "JsonUtilities"
            }
        );
    }
}