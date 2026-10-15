// O PadelSim como módulo do Unreal: a física pura (sem UObject), os mesmos arquivos que o CMake compila fora dele.
using UnrealBuildTool;

public class PadelSim : ModuleRules
{
    public PadelSim(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.Add("Core");
        // Determinismo: o resultado tem de bater bit a bit com o gabarito (Testes/PadelSim/Gabarito). O UBT do 5.8
        // compila com /fp:fast por padrão (só o Editor com DefaultBuildSettings V7 vira /fp:precise), então o Game
        // sairia "rápido". Fixamos o módulo em /fp:precise nos dois.
        FPSemantics = FPSemanticsMode.Precise;
    }
}
