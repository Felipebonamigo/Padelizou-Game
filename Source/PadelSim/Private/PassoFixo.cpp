// Porte de Padel.Core/PassoFixo.cs.
#include "PadelSim/PassoFixo.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace padel
{
namespace
{
/// Um resto a menos disto de um inteiro (em passos; ≈ 8 ns a 120 Hz) é o inteiro: é o que mata a deriva.
constexpr double Folga = 1e-6;
/// atalho: um quadro vale no máximo 1 s de jogo; o tempo além disso é descartado (sem o teto, um delta absurdo
/// travaria o jogo num laço de passos). Saída, se um dia vier tempo de relógio: o chamador divide o delta.
constexpr double MaximoPorQuadro = 1.0;
} // namespace

PassoFixo::PassoFixo(int p) : passosPorSegundo(p)
{
    if (p <= 0) throw std::out_of_range("passosPorSegundo");
}

int PassoFixo::Quadro(double delta, const Entrada& entrada)
{
    acaoPendente |= entrada.AcaoPressionada;
    lobPendente |= entrada.LobPressionada;
    passosDoQuadro = 0;
    if (!(delta > 0) || !std::isfinite(delta)) return 0;

    resto += std::min(delta, MaximoPorQuadro) * passosPorSegundo;
    int passos = static_cast<int>(std::floor(resto + Folga));
    resto -= passos;
    if (std::fabs(resto) < Folga) resto = 0;   // rente ao inteiro: é o inteiro
    if (passos == 0) return 0;

    primeira = entrada;
    primeira.AcaoPressionada = acaoPendente;
    primeira.LobPressionada = lobPendente;
    acaoPendente = lobPendente = false;
    passosDoQuadro = passos;
    return passos;
}

Entrada PassoFixo::EntradaDoPasso(int i) const
{
    if (i < 0 || i >= passosDoQuadro) throw std::out_of_range("i");
    if (i == 0) return primeira;
    Entrada e = primeira;
    e.AcaoPressionada = false;
    e.LobPressionada = false;
    return e;
}
} // namespace padel
