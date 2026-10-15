#pragma once

#include "CoreMinimal.h"
#include "PadelSim/Quadra.h"

// Uma caixa alinhada aos eixos do Unreal, em cm: centro e meia extensão.
struct FPadelCaixa
{
    FVector Centro = FVector::ZeroVector;
    FVector MeiaExtensao = FVector::ZeroVector;
};

namespace PadelPaineis
{
/// Espessura do vidro e da grade desenhados, em cm. A física não tem espessura; isto é só visual, e a caixa fica por
/// fora do painel (o plano do painel é a face de dentro).
inline constexpr double EspessuraEmCm = 4.0;

/// O painel de quadra::Paineis() como caixa do Unreal. Nenhuma medida de parede é escrita à mão: tudo vem do painel.
inline FPadelCaixa CaixaDoPainel(const padel::quadra::Painel& P)
{
    constexpr double M = 100.0;
    const double AoLongoCentro = (P.Inicio + P.Fim) * 0.5 * M;
    const double AoLongoMeia = (P.Fim - P.Inicio) * 0.5 * M;
    const double Plano = (P.Parede == padel::QualParede::Lateral ? padel::quadra::MeiaLargura : padel::quadra::MeioComprimento) * M;
    const double Fora = P.Sinal * (Plano + EspessuraEmCm * 0.5);

    FPadelCaixa C;
    C.Centro.Z = (P.ZMin + P.ZMax) * 0.5 * M;
    C.MeiaExtensao.Z = (P.ZMax - P.ZMin) * 0.5 * M;
    if (P.Parede == padel::QualParede::Lateral)
    {
        // Parede lateral: x do PadelSim fixo (Y do Unreal = -x), ao longo dela é o y do PadelSim (= X do Unreal).
        C.Centro.X = AoLongoCentro;
        C.Centro.Y = -Fora;
        C.MeiaExtensao.X = AoLongoMeia;
        C.MeiaExtensao.Y = EspessuraEmCm * 0.5;
    }
    else
    {
        // Parede de fundo: y do PadelSim fixo (= X do Unreal), ao longo dela é o x do PadelSim (Y do Unreal = -x).
        C.Centro.X = Fora;
        C.Centro.Y = -AoLongoCentro;
        C.MeiaExtensao.X = EspessuraEmCm * 0.5;
        C.MeiaExtensao.Y = AoLongoMeia;
    }
    return C;
}
} // namespace PadelPaineis
