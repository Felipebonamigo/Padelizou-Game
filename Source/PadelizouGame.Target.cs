using UnrealBuildTool;

public class PadelizouGameTarget : TargetRules
{
    public PadelizouGameTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
        ExtraModuleNames.AddRange(new string[] { "PadelizouGame", "PadelSim" });
    }
}
