// Aleatório com semente (mulberry32): a mesma semente reproduz a mesma partida (Padel.Core/Util.cs).
#pragma once

#include <cstdint>

namespace padel
{
class Aleatorio
{
public:
    explicit Aleatorio(uint32_t semente);
    /// Número em [0, 1).
    float Proximo();
    /// Box-Muller: média 0, desvio 1. Log e cosseno de padel::mat (sem libm).
    float Gaussiana();

private:
    uint32_t estado;
};
} // namespace padel
