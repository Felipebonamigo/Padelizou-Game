#pragma once

#include "PadelSim/Exportar.h"
#include "PadelSim/Tipos.h"

namespace padel::golpes
{
/// Velocidade e spin pra levar a bola da origem ao alvo (no chão) em cerca de tempoDeVoo segundos, com a física de
/// verdade: chute analítico no vácuo, depois simula e corrige até cair a menos de 12 cm do alvo; se bate na rede, sobe
/// o arco. ignorarRede pula a correção e deixa a bola ir baixa de propósito (é como se erra na rede ou no vidro).
PADELSIM_API Velocidade Calcular(float x0, float y0, float z0, float alvoX, float alvoY, float tempoDeVoo = 0.9f,
                    float folgaNaRede = 0.3f, bool ignorarRede = false, Efeito efeito = {});
} // namespace padel::golpes
