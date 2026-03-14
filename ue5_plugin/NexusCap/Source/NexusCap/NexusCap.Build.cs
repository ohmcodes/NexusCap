// Copyright NexusCap Contributors. All Rights Reserved.

using UnrealBuildTool;

public class NexusCap : ModuleRules
{
    public NexusCap(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "Sockets",
            "Networking",
            "LiveLink",
            "LiveLinkInterface",
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Slate",
            "SlateCore",
            "InputCore",
            "UnrealEd",
        });

        // Live Link editor source factory (editor-only)
        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(new string[]
            {
                "WorkspaceMenuStructure",
            });
        }
    }
}
