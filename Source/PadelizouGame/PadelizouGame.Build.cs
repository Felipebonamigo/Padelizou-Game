using UnrealBuildTool;

public class PadelizouGame : ModuleRules
{
    public PadelizouGame(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });
        PrivateDependencyModuleNames.AddRange(new string[] { "UMG", "Slate", "SlateCore" });
        // O motor da física, do golpe e do placar: o Unreal só desenha e lê o controle.
        PublicDependencyModuleNames.Add("PadelSim");
    }
}
