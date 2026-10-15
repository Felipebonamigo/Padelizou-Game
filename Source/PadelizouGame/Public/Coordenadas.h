#pragma once

#include "CoreMinimal.h"

// A conversÃ£o PadelSim <-> Unreal, escrita num lugar sÃ³. Todo ator lÃª e escreve posiÃ§Ã£o por aqui.
//
// PadelSim: metros; x = largura (+x Ã© a DIREITA de quem joga do lado de y > 0, o time da casa, olhando a rede), y =
// comprimento da quadra, z = altura.
// Unreal:   centÃ­metros; X = comprimento (o y do PadelSim), Y = -largura (o -x do PadelSim), Z = altura.
// Com a cÃ¢mera atrÃ¡s da casa olhando para -X, a direita da tela Ã© -Y: por isso Y = -x, e o D (Dx > 0) anda para a direita.
namespace PadelCoordenadas
{
inline constexpr double MetroEmCm = 100.0;

inline FVector ParaUnreal(float x, float y, float z)
{
    return FVector(y * MetroEmCm, -x * MetroEmCm, z * MetroEmCm);
}

inline void ParaPadel(const FVector& P, float& x, float& y, float& z)
{
    x = static_cast<float>(-P.Y / MetroEmCm);
    y = static_cast<float>(P.X / MetroEmCm);
    z = static_cast<float>(P.Z / MetroEmCm);
}
} // namespace PadelCoordenadas
