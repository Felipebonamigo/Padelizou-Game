#include "PadelSim/Aleatorio.h"

#include <cmath>

namespace padel
{
Aleatorio::Aleatorio(uint32_t semente) : estado(semente == 0 ? 0x9e3779b9u : semente) {}

float Aleatorio::Proximo()
{
    estado += 0x6d2b79f5u;
    uint32_t t = estado;
    t = (t ^ (t >> 15)) * (t | 1u);
    t ^= t + (t ^ (t >> 7)) * (t | 61u);
    return static_cast<float>((t ^ (t >> 14)) >> 8) / 16777216.f;
}

float Aleatorio::Gaussiana()
{
    constexpr float Pi = 3.14159265358979323846f;
    float u = 0, v = 0;
    while (u == 0) u = Proximo();
    while (v == 0) v = Proximo();
    return std::sqrt(-2 * std::log(u)) * std::cos(2 * Pi * v);
}
} // namespace padel
