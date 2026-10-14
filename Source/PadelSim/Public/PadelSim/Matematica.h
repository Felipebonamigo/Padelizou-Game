// Funções com a semântica exata do .NET (o gabarito): MathF.Min/Max tratam o zero com sinal de um jeito que std::min/max
// não tratam (Max(-0, +0) = +0), e Util.Limitar é Min(maximo, Max(minimo, valor)).
#pragma once

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
} // namespace padel::mat
