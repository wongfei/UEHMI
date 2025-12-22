using UnrealBuildTool;

public class HMIUncooked : ModuleRules
{
    public HMIUncooked(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "HMI"
        });

        PrivateDependencyModuleNames.AddRange(
            new string[] {
                "CoreUObject",
                "Engine",
                "AnimGraph",
                "BlueprintGraph",
            }
        );
    }
}
