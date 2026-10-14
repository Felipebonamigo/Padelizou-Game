// Gabarito dos solucionadores dos golpes especiais (chiquita, contrapared, remate por 3 e por 4) e do VelocidadeDe,
// contra o Padel.Core: Gabarito/golpes-especiais.csv e Gabarito/velocidade-de.csv, gravados pelo
// padelizou-arena/ferramentas/Gabarito/GabaritoDosGolpesEspeciais.cs (repositório padelizou).
// Tolerância ZERO: cada float tem de ser o mesmo bit a bit (NaN só casa com NaN), nada com nada, e os sub-passos de bola
// que a chamada simulou (Bola::SubPassosNestaThread antes e depois) iguais — o orçamento da chiquita conta por eles,
// então uma conta diferente muda o golpe.
#include "PadelSim/Bola.h"
#include "PadelSim/GolpesEspeciais.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <limits>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#ifndef PASTA_DO_GABARITO
#error "defina PASTA_DO_GABARITO (o CMakeLists.txt define)"
#endif

using namespace padel;
namespace ge = padel::golpes_especiais;

namespace
{
int falhas = 0;
int verificacoes = 0;

void Falhar(const std::string& msg)
{
    ++falhas;
    if (falhas <= 40) std::printf("FALHA: %s\n", msg.c_str());
}

void Conferir(bool ok, const std::string& msg)
{
    ++verificacoes;
    if (!ok) Falhar(msg);
}

using Linha = std::vector<std::string>;

std::vector<Linha> LerCsv(const std::string& nome)
{
    std::ifstream arq(std::string(PASTA_DO_GABARITO) + "/" + nome);
    if (!arq) { std::printf("FALHA: não abriu %s\n", nome.c_str()); std::exit(1); }
    std::vector<Linha> linhas;
    std::string s;
    bool cabecalho = true;
    while (std::getline(arq, s))
    {
        if (s.empty() || s[0] == '#') continue;
        if (cabecalho) { cabecalho = false; continue; }
        Linha l;
        std::stringstream ss(s);
        std::string campo;
        while (std::getline(ss, campo, ',')) l.push_back(campo);
        linhas.push_back(l);
    }
    return linhas;
}

float F(const std::string& s) { return std::strtof(s.c_str(), nullptr); }
int I(const std::string& s) { return std::atoi(s.c_str()); }

uint32_t Bits(float f)
{
    uint32_t b;
    std::memcpy(&b, &f, sizeof b);
    return b;
}

/// Bit a bit; NaN casa com NaN (o "R" do C# não grava o payload).
bool Igual(float a, float b) { return (std::isnan(a) && std::isnan(b)) || Bits(a) == Bits(b); }

struct
{
    float Desvio = 0;   // o maior |obtido − esperado| entre os floats (infinito: um lado NaN ou infinito)
    std::string Onde = "nenhum";
    long long SubPassos = 0;   // a maior diferença de sub-passos numa chamada
    std::string OndeSubPassos = "nenhuma";
} pior;

void ConferirFloat(float obtido, const std::string& texto, const std::string& onde)
{
    float esperado = F(texto);
    bool igual = Igual(obtido, esperado);
    float desvio = igual ? 0.f
                   : std::isfinite(obtido) && std::isfinite(esperado) ? std::fabs(obtido - esperado)
                                                                       : std::numeric_limits<float>::infinity();
    if (desvio > pior.Desvio) { pior.Desvio = desvio; pior.Onde = onde; }
    char msg[400];
    std::snprintf(msg, sizeof msg, "%s: obtido %.9g (0x%08x), esperado %s (0x%08x), desvio %.3g", onde.c_str(), obtido,
                  static_cast<unsigned>(Bits(obtido)), texto.c_str(), static_cast<unsigned>(Bits(esperado)), desvio);
    Conferir(igual, msg);
}

void ConferirInteiro(long long obtido, long long esperado, const std::string& onde)
{
    char msg[300];
    std::snprintf(msg, sizeof msg, "%s: obtido %lld, esperado %lld", onde.c_str(), obtido, esperado);
    Conferir(obtido == esperado, msg);
}

/// A partir de linha[primeiro]: temLancamento, vx, vy, vz, tempo, wx, wy, wz.
void ConferirLancamento(const std::optional<Velocidade>& l, const Linha& linha, size_t primeiro, const std::string& onde)
{
    ConferirInteiro(l.has_value(), I(linha[primeiro]), onde + " temLancamento");
    if (!l || linha[primeiro] != "1") return;
    const float obtidos[] = {l->Vx, l->Vy, l->Vz, l->Tempo, l->Wx, l->Wy, l->Wz};
    const char* nomes[] = {"vx", "vy", "vz", "tempo", "wx", "wy", "wz"};
    for (size_t i = 0; i < 7; i++) ConferirFloat(obtidos[i], linha[primeiro + 1 + i], onde + " " + nomes[i]);
}

struct Contagem
{
    int Chamadas = 0, ComSolucao = 0;
};

void TesteSolucionadores()
{
    std::map<std::string, Contagem> contagem;
    for (const auto& l : LerCsv("golpes-especiais.csv"))
    {
        const std::string& qual = l[1];
        float x0 = F(l[2]), y0 = F(l[3]), z0 = F(l[4]);
        int32_t lado = I(l[5]);
        const std::string onde = "chamada " + l[0] + " (" + qual + " de " + l[2] + "," + l[3] + "," + l[4] + " lado " + l[5] + " arg " + l[6] + ")";

        int64_t antes = Bola::SubPassosNestaThread();
        std::optional<Golpe> golpe;
        if (qual == "chiquita") golpe = ge::Chiquita(x0, y0, z0, lado, F(l[6]));
        else if (qual == "contrapared") golpe = ge::Contrapared(x0, y0, z0, lado, F(l[6]));
        else if (qual == "por3") golpe = ge::SmashPor3(x0, y0, z0, lado, I(l[6]));
        else if (qual == "por4") golpe = ge::SmashPor4(x0, y0, z0, lado, F(l[6]));
        else { Falhar(onde + ": solucionador desconhecido"); continue; }
        long long subPassos = static_cast<long long>(Bola::SubPassosNestaThread() - antes);

        long long esperados = std::atoll(l[7].c_str());
        long long diferenca = std::llabs(subPassos - esperados);
        if (diferenca > pior.SubPassos) { pior.SubPassos = diferenca; pior.OndeSubPassos = onde; }
        ConferirInteiro(subPassos, esperados, onde + " sub-passos");

        bool achou = l[8] == "1";
        contagem[qual].Chamadas++;
        if (achou) contagem[qual].ComSolucao++;
        Conferir(golpe.has_value() == achou, onde + (achou ? ": esperado um golpe, veio nada" : ": esperado nada, veio um golpe"));
        if (!golpe || !achou) continue;

        ConferirFloat(golpe->AlvoX, l[9], onde + " alvoX");
        ConferirFloat(golpe->AlvoY, l[10], onde + " alvoY");
        ConferirFloat(golpe->TempoDeVoo, l[11], onde + " tempoDeVoo");
        ConferirInteiro(static_cast<int>(golpe->Tipo), I(l[12]), onde + " tipo");
        ConferirInteiro(golpe->IgnorarRede, I(l[13]), onde + " ignorarRede");
        ConferirFloat(golpe->Efeito.TopspinRpm, l[14], onde + " topspinRpm");
        ConferirFloat(golpe->Efeito.SidespinRpm, l[15], onde + " sidespinRpm");
        ConferirInteiro(static_cast<int>(golpe->Lado), I(l[16]), onde + " lado");
        ConferirLancamento(golpe->Lancamento, l, 17, onde);
    }
    for (const auto& [qual, c] : contagem)
        std::printf("  %-11s %3d chamadas: %3d com solução, %3d null\n", qual.c_str(), c.Chamadas, c.ComSolucao, c.Chamadas - c.ComSolucao);
}

void TesteVelocidadeDe()
{
    int comLancamento = 0, semLancamento = 0;
    for (const auto& l : LerCsv("velocidade-de.csv"))
    {
        Golpe g;
        g.AlvoX = F(l[4]);
        g.AlvoY = F(l[5]);
        g.TempoDeVoo = F(l[6]);
        g.IgnorarRede = l[7] == "1";
        g.Efeito = {F(l[8]), F(l[9])};
        if (l[10] == "1") g.Lancamento = Velocidade{F(l[11]), F(l[12]), F(l[13]), F(l[14]), F(l[15]), F(l[16]), F(l[17])};
        (g.Lancamento ? comLancamento : semLancamento)++;
        Velocidade v = ge::VelocidadeDe(g, F(l[1]), F(l[2]), F(l[3]));
        const std::string onde = "VelocidadeDe " + l[0] + (g.Lancamento ? " (com Lancamento)" : " (sem Lancamento)");
        const float obtidos[] = {v.Vx, v.Vy, v.Vz, v.Tempo, v.Wx, v.Wy, v.Wz};
        const char* nomes[] = {"vx", "vy", "vz", "tempo", "wx", "wy", "wz"};
        for (size_t i = 0; i < 7; i++) ConferirFloat(obtidos[i], l[18 + i], onde + " " + nomes[i]);
    }
    std::printf("  VelocidadeDe: %d com Lancamento, %d sem\n", comLancamento, semLancamento);
}
} // namespace

int main()
{
    TesteSolucionadores();
    TesteVelocidadeDe();
    std::printf("Pior desvio nos floats: %.3g (%s)\n", pior.Desvio, pior.Onde.c_str());
    std::printf("Pior diferença de sub-passos: %lld (%s)\n", pior.SubPassos, pior.OndeSubPassos.c_str());
    std::printf("GolpesEspeciais: %d verificações, %d falha(s).\n", verificacoes, falhas);
    return falhas == 0 ? 0 : 1;
}
