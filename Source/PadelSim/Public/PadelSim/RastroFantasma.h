// O rastro-fantasma da saída de parede (Padel.Core/RastroFantasma.cs): por onde a bola vai se ninguém bater nela, até
// deixar de ter jogo do lado de quem recebe — a física rodada adiante numa cópia da bola, no passo da partida.
#pragma once

#include "PadelSim/Exportar.h"

#include "PadelSim/Bola.h"

#include <cstdint>
#include <vector>

namespace padel
{
/// Um ponto do rastro: onde a bola estará e em quantos segundos a partir de agora.
struct PADELSIM_API PontoDoRastro
{
    float X = 0, Y = 0, Z = 0, T = 0;
};

/// Os pontos, um por passo (o i-ésimo é a bola ao fim do passo i + 1), e o índice do primeiro depois da batida no vidro
/// do lado de quem recebe, já quicada — o começo da saída de parede; -1 se ela não vem pela frente.
struct PADELSIM_API Rastro
{
    std::vector<PontoDoRastro> Pontos;
    int32_t PrimeiroDepoisDaParede = -1;
};

namespace rastro_fantasma
{
/// O passo da partida (Partida::Avancar a 120 Hz).
constexpr float Passo = 1.f / 120.f;
/// 3 s: mais que isso, nenhuma bola segue em jogo sem alguém bater.
constexpr int32_t PassosMaximos = 360;

/// O rastro de agora até o segundo quique no lado (contando jaQuicou); o vidro desse lado antes do quique; a bola
/// quicar ou bater na parede do outro lado, cruzar a rede de volta, sair, parar ou rolar; ou PassosMaximos. Quem desenha
/// escolhe o trecho pelo nível de assistência (o porquê está no RastroFantasma.cs).
PADELSIM_API Rastro Prever(const Bola& bola, int32_t lado, bool jaQuicou);
} // namespace rastro_fantasma
} // namespace padel
