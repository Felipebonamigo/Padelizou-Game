// Automation Spec "PadelSim.Gabarito": roda dentro do Unreal as mesmas conferências de Testes/PadelSim/Testes*.cpp
// (o código é o mesmo, compilado com as opções de ponto flutuante do UBT). Tolerância zero: qualquer falha aqui é o
// Unreal compilando o motor com outra matemática que o gabarito do Padel.Core.
#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"

#include <cstdio>
#include <io.h> // _dup/_dup2 para capturar o stdout dos testes (o projeto é só Windows por enquanto)

int TestesDaMatematica_Principal();
int TestesDoPadelSim_Principal();
int TestesDosGolpesEspeciais_Principal();
int TestesDaPartida_Principal();
int TestesDoPassoFixo_Principal();
int TestesDoPlacarEDoArbitro_Principal();
int TestesDoRastroFantasma_Principal();
int TestesDosGolpesDoHumano_Principal();

BEGIN_DEFINE_SPEC(FPadelSimGabaritoSpec, "PadelSim.Gabarito",
                  EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
    // Roda um dos main() e devolve o que ele imprimiu (o stdout vai para um arquivo e volta).
    void Rodar(const TCHAR* Nome, int (*Principal)());
END_DEFINE_SPEC(FPadelSimGabaritoSpec)

void FPadelSimGabaritoSpec::Rodar(const TCHAR* Nome, int (*Principal)())
{
    const FString Arquivo = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("PadelSim") / FString::Printf(TEXT("%s.txt"), Nome));
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Arquivo), true);

    fflush(stdout);
    const int Antigo = _dup(_fileno(stdout));
    FILE* Saida = nullptr;
    freopen_s(&Saida, TCHAR_TO_UTF8(*Arquivo), "wb", stdout);
    const int Resultado = Principal();
    fflush(stdout);
    _dup2(Antigo, _fileno(stdout));
    _close(Antigo);

    FString Texto;
    FFileHelper::LoadFileToString(Texto, *Arquivo);
    TArray<FString> Linhas;
    Texto.ParseIntoArrayLines(Linhas);
    for (const FString& Linha : Linhas)
    {
        AddInfo(FString::Printf(TEXT("%s: %s"), Nome, *Linha));
    }
    TestEqual(*FString::Printf(TEXT("%s: código de saída (0 falhas)"), Nome), Resultado, 0);
}

void FPadelSimGabaritoSpec::Define()
{
    Describe("contra o gabarito do Padel.Core", [this]()
    {
        It("Matematica", [this]() { Rodar(TEXT("TestesDaMatematica"), &TestesDaMatematica_Principal); });
        It("PadelSim (bola, quadra, golpes)", [this]() { Rodar(TEXT("TestesDoPadelSim"), &TestesDoPadelSim_Principal); });
        It("GolpesEspeciais", [this]() { Rodar(TEXT("TestesDosGolpesEspeciais"), &TestesDosGolpesEspeciais_Principal); });
        It("Partida", [this]() { Rodar(TEXT("TestesDaPartida"), &TestesDaPartida_Principal); });
        It("PassoFixo", [this]() { Rodar(TEXT("TestesDoPassoFixo"), &TestesDoPassoFixo_Principal); });
        It("PlacarEArbitro", [this]() { Rodar(TEXT("TestesDoPlacarEDoArbitro"), &TestesDoPlacarEDoArbitro_Principal); });
        It("RastroFantasma", [this]() { Rodar(TEXT("TestesDoRastroFantasma"), &TestesDoRastroFantasma_Principal); });
        It("GolpesDoHumano", [this]() { Rodar(TEXT("TestesDosGolpesDoHumano"), &TestesDosGolpesDoHumano_Principal); });
    });
}
