using System.IO;
using UnrealBuildTool;

// As conferências do PadelSim contra o gabarito dentro do Unreal. Reaproveita os Testes/PadelSim/Testes*.cpp como
// estão (cada main() vira uma função), por isso fica num módulo só de Editor, sem unity e com exceções ligadas
// (std::regex e std::stringstream dos testes).
public class PadelSimTestes : ModuleRules
{
    public PadelSimTestes(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.NoPCHs;
        bUseUnity = false;
        bEnableExceptions = true;
        bUseRTTI = true;
        // Mesma precisão do motor: os testes montam valores float com as mesmas contas do gabarito.
        FPSemantics = FPSemanticsMode.Precise;

        PrivateDependencyModuleNames.AddRange(new string[] { "Core", "PadelSim" });

        string Raiz = Path.GetFullPath(Path.Combine(ModuleDirectory, "..", ".."));
        PrivateIncludePaths.Add(Path.Combine(Raiz, "Testes", "PadelSim"));
        PrivateDefinitions.Add("PASTA_DO_GABARITO=\"" + Path.Combine(Raiz, "Testes", "PadelSim", "Gabarito").Replace(Path.DirectorySeparatorChar, '/') + "\"");
        PrivateDefinitions.Add("PASTA_DO_MOTOR=\"" + Path.Combine(Raiz, "Source", "PadelSim", "Private").Replace(Path.DirectorySeparatorChar, '/') + "\"");
    }
}
