// Porte linha a linha de Padel.Core/MatematicaDeterministica.cs (algoritmos da Cephes, de Stephen L. Moshier, domínio
// público). Mudou uma linha lá, muda aqui — o TestesDaMatematica confere bit a bit.
#include "PadelSim/Matematica.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>

namespace padel::mat
{
namespace
{
constexpr float Pi = 3.14159265358979323846f;
constexpr float PiSobre2 = 1.57079632679489661923f;
constexpr float PiSobre4 = 0.78539816339744830962f;
constexpr float QuatroSobrePi = 1.27323954473516f;
constexpr float DP1 = 0.78515625f, DP2 = 2.4187564849853515625e-4f, DP3 = 3.77489497744594108e-8f;

int32_t BitsDe(float f)
{
    int32_t b;
    std::memcpy(&b, &f, sizeof b);
    return b;
}

float DeBits(int32_t b)
{
    float f;
    std::memcpy(&f, &b, sizeof f);
    return f;
}

float PolinomioDoSeno(float r, float z) { return ((-1.9515295891E-4f * z + 8.3321608736E-3f) * z - 1.6666654611E-1f) * z * r + r; }

float PolinomioDoCosseno(float z)
{
    return ((2.443315711809948E-5f * z - 1.388731625493765E-3f) * z + 4.166664568298827E-2f) * z * z - 0.5f * z + 1.0f;
}

float ArcoTangente(float x)
{
    int sinal = 1;
    if (x < 0) { sinal = -1; x = -x; }
    float y;
    if (x > 2.414213562373095f) { y = PiSobre2; x = -(1 / x); }
    else if (x > 0.4142135623730950f) { y = PiSobre4; x = (x - 1) / (x + 1); }
    else y = 0;
    float z = x * x;
    y += (((8.05374449538e-2f * z - 1.38776856032E-1f) * z + 1.99777106478E-1f) * z - 3.33329491539E-1f) * z * x + x;
    return sinal < 0 ? -y : y;
}

/// 2^n exato, montado nos bits do expoente (n em [−126, 127]).
float DoisElevadoA(int n) { return DeBits(static_cast<int32_t>(static_cast<uint32_t>(n + 127) << 23)); }
} // namespace

float Seno(float x)
{
    if (!std::isfinite(x)) return std::numeric_limits<float>::quiet_NaN();
    int sinal = 1;
    if (x < 0) { x = -x; sinal = -1; }
    int j = static_cast<int>(QuatroSobrePi * x);
    float y = static_cast<float>(j);
    if ((j & 1) != 0) { j += 1; y += 1; }
    j &= 7;
    if (j > 3) { sinal = -sinal; j -= 4; }
    float r = ((x - y * DP1) - y * DP2) - y * DP3;
    float z = r * r;
    float s = j == 1 || j == 2 ? PolinomioDoCosseno(z) : PolinomioDoSeno(r, z);
    return sinal < 0 ? -s : s;
}

float Cosseno(float x)
{
    if (!std::isfinite(x)) return std::numeric_limits<float>::quiet_NaN();
    if (x < 0) x = -x;
    int j = static_cast<int>(QuatroSobrePi * x);
    float y = static_cast<float>(j);
    if ((j & 1) != 0) { j += 1; y += 1; }
    j &= 7;
    int sinal = 1;
    if (j > 3) { j -= 4; sinal = -sinal; }
    if (j > 1) sinal = -sinal;
    float r = ((x - y * DP1) - y * DP2) - y * DP3;
    float z = r * r;
    float c = j == 1 || j == 2 ? PolinomioDoSeno(r, z) : PolinomioDoCosseno(z);
    return sinal < 0 ? -c : c;
}

float ArcoTangente2(float y, float x)
{
    if (std::isnan(x) || std::isnan(y)) return std::numeric_limits<float>::quiet_NaN();
    if (x == 0)
    {
        if (y > 0) return PiSobre2;
        if (y < 0) return -PiSobre2;
        return 0;
    }
    if (y == 0) return x > 0 ? 0.f : Pi;
    float w = ArcoTangente(y / x);
    if (x < 0) w = y < 0 ? w - Pi : w + Pi;
    return w;
}

float Exp(float x)
{
    if (std::isnan(x)) return x;
    if (x > 88.72283905206835f) return std::numeric_limits<float>::infinity();
    if (x < -87.33654475055310898657f) return 0;
    float z = std::floor(1.44269504088896341f * x + 0.5f);
    x -= z * 0.693359375f;
    x -= z * -2.12194440e-4f;
    int n = static_cast<int>(z);
    float x2 = x * x;
    float p = (((((1.9875691500E-4f * x + 1.3981999507E-3f) * x + 8.3334519073E-3f) * x + 4.1665795894E-2f) * x + 1.6666665459E-1f) * x
               + 5.0000001201E-1f) * x2 + x + 1.0f;
    return n > 127 ? p * DoisElevadoA(127) * 2 : p * DoisElevadoA(n);
}

float Log(float x)
{
    if (std::isnan(x) || x < 0) return std::numeric_limits<float>::quiet_NaN();
    if (x == 0) return -std::numeric_limits<float>::infinity();
    if (std::isinf(x)) return x;
    int e = 0;
    int32_t bits = BitsDe(x);
    if ((bits >> 23) == 0)
    {
        x *= 33554432.f;   // 2^25: tira do subnormal
        e = -25;
        bits = BitsDe(x);
    }
    e += ((bits >> 23) & 0xff) - 126;
    float m = DeBits((bits & 0x007fffff) | 0x3f000000);   // em [0,5, 1)
    if (m < 0.707106781186547524f) { e -= 1; m = m + m - 1; }
    else m = m - 1;
    float z = m * m;
    float y = ((((((((7.0376836292E-2f * m - 1.1514610310E-1f) * m + 1.1676998740E-1f) * m - 1.2420140846E-1f) * m + 1.4249322787E-1f) * m
                  - 1.6668057665E-1f) * m + 2.0000714765E-1f) * m - 2.4999993993E-1f) * m + 3.3333331174E-1f) * m * z;
    float fe = static_cast<float>(e);
    y += -2.12194440e-4f * fe;
    y += -0.5f * z;
    float r = m + y;
    r += 0.693359375f * fe;
    return r;
}

float Potencia(float b, float e)
{
    if (e == 0) return 1;
    if (b > 0) return Exp(e * Log(b));
    if (b == 0) return e > 0 ? 0.f : std::numeric_limits<float>::infinity();
    return std::numeric_limits<float>::quiet_NaN();
}

float PotenciaInteira(float b, int n)
{
    if (n < 0) return 1 / PotenciaInteira(b, -n);
    float r = 1;
    for (int i = 0; i < n; i++) r *= b;
    return r;
}
} // namespace padel::mat
