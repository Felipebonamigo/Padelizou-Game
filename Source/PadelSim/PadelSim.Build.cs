// O PadelSim como módulo do Unreal: a física pura (sem UObject), os mesmos arquivos que o CMake compila fora dele.
// NÃO COMPILADO AINDA: escrito na nuvem, sem o Unreal. A sessão local confere no pedido 0001.
using UnrealBuildTool;

public class PadelSim : ModuleRules
{
    public PadelSim(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.Add("Core");
        // Determinismo: o resultado tem de bater bit a bit com o gabarito (Testes/PadelSim/Gabarito). Matemática
        // "rápida" ou FMA quebram isso — a sessão local confere a opção de ponto flutuante do UBT desta versão e a
        // Automation Spec "PadelSim.Gabarito" prova (pedido 0001).
    }
}
