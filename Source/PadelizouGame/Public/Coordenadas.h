#pragma once

#include "CoreMinimal.h"

// A conversão PadelSim <-> Unreal, escrita num lugar só. Todo ator lê e escreve posição por aqui.
//
// PadelSim: metros; x = largura (direita de quem olha a rede do lado y > 0), y = comprimento da quadra, z = altura.
// Unreal:   centímetros; X = comprimento (o y do PadelSim), Y = largura (o x do PadelSim), Z = altura.
// Trocar x e y inverte a mão do sistema, que é o que leva o PadelSim (destro) ao Unreal (canhoto) sem espelhar a quadra.
namespace PadelCoordenadas
{
inline constexpr double MetroEmCm = 100.0;

inline FVector ParaUnreal(float x, float y, float z)
{
    return FVector(y * MetroEmCm, x * MetroEmCm, z * MetroEmCm);
}

inline void ParaPadel(const FVector& P, float& x, float& y, float& z)
{
    x = static_cast<float>(P.Y / MetroEmCm);
    y = static_cast<float>(P.X / MetroEmCm);
    z = static_cast<float>(P.Z / MetroEmCm);
}
} // namespace PadelCoordenadas
