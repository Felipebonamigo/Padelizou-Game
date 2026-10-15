#include "PadelSimulacao.h"

FPadelSimulacao::FPadelSimulacao(const padel::OpcoesDaPartida& Opcoes)
    : PartidaAtual(std::make_unique<padel::Partida>(Opcoes))
{
}

int FPadelSimulacao::Quadro(double DeltaSeconds, const padel::Entrada& Humano)
{
    const int N = Relogio.Quadro(DeltaSeconds, Humano);
    for (int I = 0; I < N; ++I)
    {
        std::array<padel::Entrada, 4> Entradas{};
        Entradas[0] = Relogio.EntradaDoPasso(I);
        PartidaAtual->Avancar(1.f / static_cast<float>(PassosPorSegundo), Entradas.data());
    }
    Passos += N;
    return N;
}
