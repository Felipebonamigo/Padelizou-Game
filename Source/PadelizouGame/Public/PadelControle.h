#pragma once

#include "CoreMinimal.h"
#include "PadelSim/Golpe.h"

// O que o controle leu neste quadro, antes de virar padel::Entrada. Mover.X > 0 = direita do jogador (D / analógico
// para a direita); Mover.Y > 0 = rumo à rede (W / analógico para cima).
struct FLeituraDoControle
{
    FVector2D Mover = FVector2D::ZeroVector;
    bool AcaoApertou = false, AcaoSegurada = false;       // A / Espaço
    bool LobApertou = false;                              // B / Shift
    bool CortadoApertou = false;                          // X / Q
    bool AtaqueApertou = false, AtaqueSegurado = false;   // Y / E
    bool ModificadorSegurado = false;                     // RB / Ctrl
};

namespace PadelControle
{
/// No referencial do jogador, como o PadelSim espera: Dy < 0 é rumo à rede, Dx > 0 é a direita dele. A diagonal do
/// teclado não passa de comprimento 1.
inline padel::Entrada MontarEntrada(const FLeituraDoControle& L)
{
    FVector2D Mover = L.Mover;
    if (Mover.SizeSquared() > 1.0) Mover = Mover.GetSafeNormal();

    padel::Entrada E;
    E.Dx = static_cast<float>(Mover.X);
    E.Dy = static_cast<float>(-Mover.Y);
    E.AcaoPressionada = L.AcaoApertou;
    E.AcaoSegurada = L.AcaoSegurada;
    E.LobPressionada = L.LobApertou;
    E.CortadoPressionado = L.CortadoApertou;
    E.AtaquePressionado = L.AtaqueApertou;
    E.AtaqueSegurado = L.AtaqueSegurado;
    E.ModificadorSegurado = L.ModificadorSegurado;
    return E;
}
} // namespace PadelControle
