// Funções com a semântica exata do .NET (o gabarito): MathF.Min/Max tratam o zero com sinal de um jeito que std::min/max
// não tratam (Max(-0, +0) = +0), e Util.Limitar é Min(maximo, Max(minimo, valor)).
#pragma once

#include "PadelSim/Exportar.h"

#include <cmath>

namespace padel::mat
{
inline float Max(float a, float b)
{
    if (a != b) { if (!std::isnan(a)) return b < a ? a : b; return a; }
    return std::signbit(b) ? a : b;
}

inline float Min(float a, float b)
{
    if (a != b) { if (!std::isnan(a)) return a < b ? a : b; return a; }
    return std::signbit(a) ? a : b;
}

inline float Limitar(float valor, float minimo, float maximo) { return Min(maximo, Max(minimo, valor)); }
inline float Distancia(float x1, float y1, float x2, float y2) { return std::sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1)); }
/// = MathF.Sign (sem a exceção do NaN: aqui NaN dá 0).
inline int Sinal(float v) { return v > 0 ? 1 : v < 0 ? -1 : 0; }

// Sem libm (Padel.Core/MatematicaDeterministica.cs, Cephes): só soma, multiplicação, divisão, raiz, floor e bits, na
// MESMA ordem do C# — o resultado sai igual em gcc, clang e MSVC e igual ao gabarito (TestesDaMatematica). O motor
// nunca chama std::sin & cia.; o TestesDaMatematica barra a volta.
PADELSIM_API float Seno(float x);
PADELSIM_API float Cosseno(float x);
/// Arco-tangente de y/x no quadrante certo, em (−π, π]. ArcoTangente2(0, 0) = 0.
PADELSIM_API float ArcoTangente2(float y, float x);
/// e^x; abaixo de −87,34 devolve 0 (atalho: o resultado seria subnormal; o motor nunca chega perto).
PADELSIM_API float Exp(float x);
/// ln x; Log(0) = −∞, negativo = NaN.
PADELSIM_API float Log(float x);
/// b^e = e^(e·ln b) para b > 0.
PADELSIM_API float Potencia(float b, float e);
/// b^n por multiplicações seguidas (exato enquanto couber em 24 bits).
PADELSIM_API float PotenciaInteira(float b, int n);
} // namespace padel::mat
