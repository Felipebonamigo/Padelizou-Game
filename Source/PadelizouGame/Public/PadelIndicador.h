#pragma once

#include "CoreMinimal.h"
#include "PadelSim/Partida.h"

#include <optional>

// Quando apertar, para o jogador local no Manual: com a mesma física do PadelSim, numa cópia da bola, acha o passo em
// que o motor deixaria a raquete bater (a bola ao alcance e passando pelo ponto ideal) e desconta os 0,12 s do balanço.
// Só lê a Partida; nenhuma regra nova. O corpo é suposto parado onde está (quem anda muda a previsão, e ela se refaz
// a cada quadro).
struct FPadelPrevisaoDeContato
{
    /// Quanto falta para começar o balanço (aperto); negativo = já passou da hora.
    double SegundosParaApertar = 0.0;
    /// Quanto falta para a raquete tocar a bola.
    double SegundosParaOContato = 0.0;
    /// Onde a bola estará no contato, em metros do PadelSim (o chão embaixo dela).
    float X = 0.f, Y = 0.f;
};

namespace FPadelIndicador
{
/// Vazio quando não há bola vindo para o jogador (fora do rally, o time dele não pode bater, ou a bola não chega ao
/// alcance dele nos próximos 2 s).
PADELIZOUGAME_API std::optional<FPadelPrevisaoDeContato> Prever(const padel::Partida& Partida, int Jogador);
} // namespace FPadelIndicador
