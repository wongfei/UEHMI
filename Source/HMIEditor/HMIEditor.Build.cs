using UnrealBuildTool;

public class HMIEditor : ModuleRules
{
    public HMIEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "HMI"
        });

        PrivateDependencyModuleNames.AddRange(
            new string[] {
                "CoreUObject",
                "Engine",
                "UnrealEd",
                "Slate",
                "SlateCore",
                //"CurveEditor",
            }
        );
    }
}
