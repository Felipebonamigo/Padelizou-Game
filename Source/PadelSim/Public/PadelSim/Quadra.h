// Quadra de padel, em metros. Origem no centro da rede. x: -5..5 (largura 10 m); y: -10..10 (comprimento 20 m);
// z pra cima. O time da casa (time 0) joga em y > 0; o visitante (time 1) em y < 0.
#pragma once

#include "PadelSim/Tipos.h"
#include <vector>

namespace padel::quadra
{
inline constexpr float Largura = 10.f;
inline constexpr float Comprimento = 20.f;
inline constexpr float MeiaLargura = 5.f;
inline constexpr float MeioComprimento = 10.f;
inline constexpr float LinhaDeSaque = 6.95f;   // distância da rede
inline constexpr float AlturaDaRede = 0.9f;    // 0,88 no centro e 0,92 nos postes — média
// Fundo (|y| = 10) e canto da lateral (8 ≤ |y| ≤ 10): vidro até 3 m, grade de 3 a 4 m.
inline constexpr float AlturaDoVidro = 3.f;
inline constexpr float AlturaDaParede = 4.f;
inline constexpr float InicioDoCanto = 8.f;
// Degrau (6 ≤ |y| < 8): vidro até 2 m, grade de 2 a 3 m.
inline constexpr float InicioDoDegrau = 6.f;
inline constexpr float AlturaDoVidroDoDegrau = 2.f;
inline constexpr float AlturaDoDegrau = 3.f;
// Meio da lateral (|y| < 6): só grade, até 3 m — menos nas portas.
inline constexpr float AlturaDoMeio = 3.f;
// Portas: uma em cada lateral de cada lado da rede, de |y| = 0,45 a 1,25 m, do chão até 2 m.
inline constexpr float InicioDaPorta = 0.45f;
inline constexpr float FimDaPorta = 1.25f;
inline constexpr float AlturaDaPorta = 2.f;

int32_t LadoDe(float y);
int32_t LadoDoTime(int32_t time);
bool Dentro(float x, float y);

/// O que há na parede no ponto dado: Vidro, Grade ou Aberto. aoLongo é a coordenada ao longo da parede (y na lateral,
/// x no fundo). Nas emendas vale o trecho de baixo e o de dentro: z = 3 no fundo é vidro, |y| = 8 já é canto.
Superficie SuperficieDaParede(QualParede parede, float aoLongo, float z);
/// A bola nesse ponto da lateral passa pela porta (as quatro portas são simétricas).
bool NaPorta(float aoLongo, float z);

/// Um retângulo de parede. Sinal: qual das duas paredes do tipo (x = ±5 na lateral; y = ±10 no fundo). Inicio e Fim
/// ao longo dela, ZMin e ZMax a altura.
struct Painel
{
    QualParede Parede;
    int32_t Sinal;
    float Inicio, Fim, ZMin, ZMax;
    Superficie Material;   // Vidro ou Grade
};

/// Todos os painéis de vidro e grade das quatro paredes, sem sobreposição. É o que o Unreal desenha e colide —
/// exatamente o que a física usa.
const std::vector<Painel>& Paineis();
} // namespace padel::quadra
