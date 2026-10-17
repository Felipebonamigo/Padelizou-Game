#pragma once

#include "CoreMinimal.h"
#include "PadelSim/Golpe.h"
#include "PadelSim/RastroFantasma.h"

#include <optional>

// O que mostrar do rastro-fantasma (padel::rastro_fantasma::Prever) e a cor do pé do jogador. Só escolha: a previsão é
// do PadelSim, com gabarito. A tabela de assistência é a do DESIGN.md:
//   Casual: sempre, a partir da primeira batida no vidro (já quicada) = a saída de parede;
//   Padrão: só quando a bola bate no vidro (o rastro pedido nesse passo, inteiro, já é a saída);
//   Pro:    desligado.
struct FTrechoDoRastro
{
    int32 Inicio = 0;   // índice do primeiro ponto a desenhar
    int32 Fim = 0;      // um depois do último
};

namespace PadelRastro
{
/// Casual: do `PrimeiroDepoisDaParede` até o fim (nada se ele é -1, a saída não vem pela frente). Padrão: o rastro inteiro, mas só
/// quando `bPedidoNaBatidaDoVidro` (o rastro foi tirado no passo do evento Parede). Pro: nada.
inline std::optional<FTrechoDoRastro> Escolher(const padel::Rastro& Rastro, padel::Assistencia Nivel, bool bPedidoNaBatidaDoVidro)
{
    const int32 N = static_cast<int32>(Rastro.Pontos.size());
    if (N == 0) return std::nullopt;
    switch (Nivel)
    {
    case padel::Assistencia::Casual:
        if (Rastro.PrimeiroDepoisDaParede < 0 || Rastro.PrimeiroDepoisDaParede >= N) return std::nullopt;
        return FTrechoDoRastro{Rastro.PrimeiroDepoisDaParede, N};
    case padel::Assistencia::Padrao:
        if (!bPedidoNaBatidaDoVidro) return std::nullopt;
        return FTrechoDoRastro{0, N};
    case padel::Assistencia::Pro:
        break;
    }
    return std::nullopt;
}

/// O resultado do golpe, por cor no pé do jogador: erro 0 (perfeito) é verde, ~0,8 amarelo, 1,6 (péssimo) vermelho
/// (Partida::UltimoErroDoHumano vai de 0 a 1,6). Os pontos de corte são visuais, não regra.
inline FLinearColor CorDoErro(float Erro)
{
    const float T = FMath::Clamp(Erro / 1.6f, 0.f, 1.f);
    const FLinearColor Verde(0.1f, 0.95f, 0.2f), Amarelo(1.f, 0.9f, 0.1f), Vermelho(1.f, 0.15f, 0.1f);
    return T < 0.5f ? FMath::Lerp(Verde, Amarelo, T / 0.5f) : FMath::Lerp(Amarelo, Vermelho, (T - 0.5f) / 0.5f);
}
} // namespace PadelRastro
