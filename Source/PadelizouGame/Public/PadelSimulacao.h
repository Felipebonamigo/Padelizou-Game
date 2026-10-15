#pragma once

#include "CoreMinimal.h"
#include "PadelSim/Partida.h"
#include "PadelSim/PassoFixo.h"

#include <array>
#include <memory>

// Uma partida do PadelSim e o relógio dela. O Unreal chama Quadro() com o DeltaSeconds de cada quadro; o PassoFixo
// transforma isso em passos de 120 Hz, e cada passo é um Partida::Avancar(1/120). Nunca Avancar(DeltaSeconds) direto:
// o passo é fixo, ou o jogo deixa de ser o do gabarito.
class PADELIZOUGAME_API FPadelSimulacao
{
public:
    static constexpr int PassosPorSegundo = 120;

    explicit FPadelSimulacao(const padel::OpcoesDaPartida& Opcoes);

    /// Um quadro: Humano é a entrada do jogador 0 lida neste quadro (o aperto vale no primeiro passo). Devolve quantos
    /// passos rodou.
    int Quadro(double DeltaSeconds, const padel::Entrada& Humano);

    padel::Partida& Partida() { return *PartidaAtual; }
    const padel::Partida& Partida() const { return *PartidaAtual; }
    int PassosTotais() const { return Passos; }

private:
    std::unique_ptr<padel::Partida> PartidaAtual;
    padel::PassoFixo Relogio{PassosPorSegundo};
    int Passos = 0;
};
