// Transforma o tempo de cada quadro (o DeltaSeconds do Unreal) em passos fixos de simulação, a 120 por segundo
// (Padel.Core/PassoFixo.cs). Duas garantias, com gabarito:
// - sem deriva: o acumulado é contado EM PASSOS (delta × passos por segundo, em double), e um resto rente a um inteiro
//   vira o inteiro;
// - sem aperto perdido: ação e lob valem um quadro só; se o quadro não dá passo, o aperto espera o próximo passo. Num
//   quadro com vários passos, só o primeiro leva o aperto. Direção e botão segurado são os do quadro mais novo.
#pragma once

#include "PadelSim/Golpe.h"

namespace padel
{
class PassoFixo
{
public:
    explicit PassoFixo(int passosPorSegundo);
    /// Um quadro de delta segundos com a entrada lida nele: quantos passos rodar agora (0, 1 ou mais). Delta zero,
    /// negativo ou não finito não dá passo, e o aperto dele fica guardado.
    int Quadro(double delta, const Entrada& entrada);
    /// A entrada do passo i (0 = o primeiro) do último Quadro: só o primeiro leva os apertos.
    Entrada EntradaDoPasso(int i) const;

private:
    int passosPorSegundo;
    double resto = 0;   // fração de passo que sobrou dos quadros anteriores, em [0, 1)
    bool acaoPendente = false, lobPendente = false, cortadoPendente = false, ataquePendente = false;
    Entrada primeira{};
    int passosDoQuadro = 0;
};
} // namespace padel
