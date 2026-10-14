// Os golpes que dependem de parede, de rede baixa ou de a bola sair da quadra: chiquita, contrapared, remate por 3 e
// por 4 (Padel.Core/GolpesEspeciais.cs). Cada solucionador lança candidatos, SIMULA a Bola e confere os eventos; sem
// solução daquela posição, devolve nullopt e quem chamou escolhe outro golpe.
#pragma once

#include "PadelSim/Golpe.h"

#include <cstdint>
#include <optional>

namespace padel::golpes_especiais
{
inline constexpr float AlcanceDaChiquita = 3.5f;
inline constexpr float FolgaDaChiquitaSobreARede = 0.5f;
inline constexpr float DistanciaMinimaDaChiquitaDaRede = 0.10f;
inline constexpr float RapidezMaximaDoRemate = 40.f;
inline constexpr float RapidezMaximaDaContrapared = 22.f;

/// A velocidade de lançamento do golpe: a do solucionador (Lancamento), ou golpes::Calcular pelo alvo.
Velocidade VelocidadeDe(const Golpe& golpe, float x0, float y0, float z0);

std::optional<Golpe> Chiquita(float x0, float y0, float z0, int32_t ladoDoAlvo, float alvoX);
std::optional<Golpe> Contrapared(float x0, float y0, float z0, int32_t ladoDoAlvo, float alvoX);
std::optional<Golpe> SmashPor4(float x0, float y0, float z0, int32_t ladoDoAlvo, float alvoX);
std::optional<Golpe> SmashPor3(float x0, float y0, float z0, int32_t ladoDoAlvo, int32_t ladoDaSaida);
} // namespace padel::golpes_especiais
