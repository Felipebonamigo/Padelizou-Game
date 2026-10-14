#include "PadelSim/Quadra.h"

#include <cmath>
#include <utility>

namespace padel::quadra
{
int32_t LadoDe(float y) { return y >= 0 ? 1 : -1; }
int32_t LadoDoTime(int32_t time) { return time == 0 ? 1 : -1; }
bool Dentro(float x, float y) { return std::fabs(x) <= MeiaLargura && std::fabs(y) <= MeioComprimento; }

namespace
{
Superficie VidroGradeOuAberto(float z, float alturaDoVidro, float alturaDaParede)
{
    return z <= alturaDoVidro ? Superficie::Vidro : z <= alturaDaParede ? Superficie::Grade : Superficie::Aberto;
}

std::pair<float, float> Ordenar(float a, float b) { return a < b ? std::pair{a, b} : std::pair{b, a}; }

std::vector<Painel> MontarPaineis()
{
    std::vector<Painel> paineis;
    for (int32_t sinal : {1, -1})
    {
        // Fundo: a largura inteira.
        paineis.push_back({QualParede::Fundo, sinal, -MeiaLargura, MeiaLargura, 0, AlturaDoVidro, Superficie::Vidro});
        paineis.push_back({QualParede::Fundo, sinal, -MeiaLargura, MeiaLargura, AlturaDoVidro, AlturaDaParede, Superficie::Grade});
        // Lateral, de uma ponta à outra: canto, degrau, meio com as duas portas, degrau, canto.
        for (float ponta : {-1.f, 1.f})
        {
            auto [canto0, canto1] = Ordenar(ponta * InicioDoCanto, ponta * MeioComprimento);
            paineis.push_back({QualParede::Lateral, sinal, canto0, canto1, 0, AlturaDoVidro, Superficie::Vidro});
            paineis.push_back({QualParede::Lateral, sinal, canto0, canto1, AlturaDoVidro, AlturaDaParede, Superficie::Grade});
            auto [degrau0, degrau1] = Ordenar(ponta * InicioDoDegrau, ponta * InicioDoCanto);
            paineis.push_back({QualParede::Lateral, sinal, degrau0, degrau1, 0, AlturaDoVidroDoDegrau, Superficie::Vidro});
            paineis.push_back({QualParede::Lateral, sinal, degrau0, degrau1, AlturaDoVidroDoDegrau, AlturaDoDegrau, Superficie::Grade});
            auto [meio0, meio1] = Ordenar(ponta * FimDaPorta, ponta * InicioDoDegrau);
            paineis.push_back({QualParede::Lateral, sinal, meio0, meio1, 0, AlturaDoMeio, Superficie::Grade});
            auto [porta0, porta1] = Ordenar(ponta * InicioDaPorta, ponta * FimDaPorta);
            paineis.push_back({QualParede::Lateral, sinal, porta0, porta1, AlturaDaPorta, AlturaDoMeio, Superficie::Grade});   // acima da porta
        }
        paineis.push_back({QualParede::Lateral, sinal, -InicioDaPorta, InicioDaPorta, 0, AlturaDoMeio, Superficie::Grade});   // entre as portas
    }
    return paineis;
}
} // namespace

Superficie SuperficieDaParede(QualParede parede, float aoLongo, float z)
{
    if (parede == QualParede::Fundo) return VidroGradeOuAberto(z, AlturaDoVidro, AlturaDaParede);
    float a = std::fabs(aoLongo);
    if (a >= InicioDoCanto) return VidroGradeOuAberto(z, AlturaDoVidro, AlturaDaParede);
    if (a >= InicioDoDegrau) return VidroGradeOuAberto(z, AlturaDoVidroDoDegrau, AlturaDoDegrau);
    if (NaPorta(aoLongo, z)) return Superficie::Aberto;
    return z <= AlturaDoMeio ? Superficie::Grade : Superficie::Aberto;
}

// atalho: a porta é um vão sem batente e a bola é um ponto — na quina, ou passa inteira ou bate inteira na grade ao
// lado; se o playtest pedir, modelar o batente (poste redondo) e o raio da bola aqui (e no Padel.Core, o gabarito).
bool NaPorta(float aoLongo, float z)
{
    float a = std::fabs(aoLongo);
    return a >= InicioDaPorta && a <= FimDaPorta && z < AlturaDaPorta;
}

const std::vector<Painel>& Paineis()
{
    static const std::vector<Painel> paineis = MontarPaineis();
    return paineis;
}
} // namespace padel::quadra
