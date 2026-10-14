// Testes do PadelSim fora do Unreal (CMake + ctest). O centro é o gabarito: as trajetórias que o Padel.Core (C#)
// gravou em Gabarito/*.csv — o C++ precisa reproduzi-las. Gerador: padelizou-arena/ferramentas/Gabarito, no
// repositório padelizou. Tolerâncias: 1 mm na posição, 1 cm/s na velocidade, 0,1 rad/s no spin.
#include "PadelSim/Bola.h"
#include "PadelSim/Golpes.h"
#include "PadelSim/Quadra.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#ifndef PASTA_DO_GABARITO
#error "defina PASTA_DO_GABARITO (o CMakeLists.txt define)"
#endif

using namespace padel;

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

struct Desvio
{
    float Posicao = 0, Velocidade = 0, Spin = 0;
} pior;

void TesteConstantes()
{
    for (const auto& l : LerCsv("constantes.csv"))
    {
        float esperado = F(l[1]);
        float obtido = l[0] == "KArrasto" ? Bola::KArrasto() : Bola::KMagnus();
        char msg[160];
        std::snprintf(msg, sizeof msg, "%s: esperado %.9g, obtido %.9g", l[0].c_str(), esperado, obtido);
        Conferir(esperado == obtido, msg);
    }
}

void TesteTrajetorias()
{
    const auto casos = LerCsv("casos.csv");
    std::map<int, std::vector<Linha>> amostras, eventos;
    for (auto& l : LerCsv("trajetorias.csv")) amostras[I(l[0])].push_back(l);
    for (auto& l : LerCsv("eventos.csv")) eventos[I(l[0])].push_back(l);

    constexpr float Quadro = 1.f / 60.f;
    std::vector<EventoDaBola> ev;
    for (const auto& c : casos)
    {
        const int id = I(c[0]);
        const std::string nome = c[1];
        Bola bola;
        bola.Posicionar(F(c[2]), F(c[3]), F(c[4]));
        bola.Lancar({F(c[5]), F(c[6]), F(c[7]), 0, F(c[8]), F(c[9]), F(c[10])});

        const auto& esperadas = amostras[id];
        const auto& eventosEsperados = eventos[id];
        size_t proximaAmostra = 0, proximoEvento = 0;
        int ultimoQuadro = I(esperadas.back()[1]);
        for (int q = 0; q <= ultimoQuadro; ++q)
        {
            if (q > 0)
            {
                ev.clear();
                bola.Avancar(Quadro, ev);
                for (const auto& e : ev)
                {
                    char msg[300];
                    if (proximoEvento >= eventosEsperados.size() || I(eventosEsperados[proximoEvento][1]) != q)
                    {
                        std::snprintf(msg, sizeof msg, "caso %d (%s), quadro %d: evento %d a mais", id, nome.c_str(), q, (int)e.Tipo);
                        Falhar(msg);
                        continue;
                    }
                    const auto& x = eventosEsperados[proximoEvento++];
                    float d = std::fabs(e.X - F(x[3])) + std::fabs(e.Y - F(x[4])) + std::fabs(e.Z - F(x[5]));
                    bool igual = (int)e.Tipo == I(x[2]) && e.Lado == I(x[6]) && (int)e.Parede == I(x[7]) &&
                                 (int)e.Material == I(x[8]) && e.PelaPorta == (I(x[9]) != 0) && d < 1e-3f;
                    std::snprintf(msg, sizeof msg, "caso %d (%s), quadro %d: evento (%d em %.4f,%.4f,%.4f lado %d parede %d sup %d porta %d), esperado (%s em %s,%s,%s lado %s parede %s sup %s porta %s)",
                                  id, nome.c_str(), q, (int)e.Tipo, e.X, e.Y, e.Z, e.Lado, (int)e.Parede, (int)e.Material, (int)e.PelaPorta,
                                  x[2].c_str(), x[3].c_str(), x[4].c_str(), x[5].c_str(), x[6].c_str(), x[7].c_str(), x[8].c_str(), x[9].c_str());
                    Conferir(igual, msg);
                }
            }
            if (proximaAmostra < esperadas.size() && I(esperadas[proximaAmostra][1]) == q)
            {
                const auto& a = esperadas[proximaAmostra++];
                float dp = std::fmax(std::fabs(bola.X - F(a[2])), std::fmax(std::fabs(bola.Y - F(a[3])), std::fabs(bola.Z - F(a[4]))));
                float dv = std::fmax(std::fabs(bola.Vx - F(a[5])), std::fmax(std::fabs(bola.Vy - F(a[6])), std::fabs(bola.Vz - F(a[7]))));
                float dw = std::fmax(std::fabs(bola.Wx - F(a[8])), std::fmax(std::fabs(bola.Wy - F(a[9])), std::fabs(bola.Wz - F(a[10]))));
                pior.Posicao = std::fmax(pior.Posicao, dp);
                pior.Velocidade = std::fmax(pior.Velocidade, dv);
                pior.Spin = std::fmax(pior.Spin, dw);
                bool estado = bola.EmJogo == (I(a[11]) != 0) && bola.Rolando == (I(a[12]) != 0) && bola.Parada == (I(a[13]) != 0);
                char msg[300];
                std::snprintf(msg, sizeof msg, "caso %d (%s), quadro %d: posição (%.5f,%.5f,%.5f) esperada (%s,%s,%s), desvio %.2e m; velocidade %.2e; spin %.2e; estado %s",
                              id, nome.c_str(), q, bola.X, bola.Y, bola.Z, a[2].c_str(), a[3].c_str(), a[4].c_str(), dp, dv, dw, estado ? "ok" : "DIFERENTE");
                Conferir(dp < 1e-3f && dv < 1e-2f && dw < 0.1f && estado, msg);
            }
        }
        char msg[200];
        std::snprintf(msg, sizeof msg, "caso %d (%s): %zu de %zu eventos", id, nome.c_str(), proximoEvento, eventosEsperados.size());
        Conferir(proximoEvento == eventosEsperados.size(), msg);
        std::snprintf(msg, sizeof msg, "caso %d (%s): %zu de %zu amostras", id, nome.c_str(), proximaAmostra, esperadas.size());
        Conferir(proximaAmostra == esperadas.size(), msg);
    }
}

void TesteGolpes()
{
    for (const auto& l : LerCsv("golpes.csv"))
    {
        Efeito efeito{F(l[10]), F(l[11])};
        Velocidade v = golpes::Calcular(F(l[2]), F(l[3]), F(l[4]), F(l[5]), F(l[6]), F(l[7]), F(l[8]), I(l[9]) != 0, efeito);
        float d = std::fmax(std::fabs(v.Vx - F(l[12])), std::fmax(std::fabs(v.Vy - F(l[13])), std::fabs(v.Vz - F(l[14]))));
        float dw = std::fmax(std::fabs(v.Wx - F(l[16])), std::fmax(std::fabs(v.Wy - F(l[17])), std::fabs(v.Wz - F(l[18]))));
        char msg[300];
        std::snprintf(msg, sizeof msg, "golpe %s (%s): v (%.4f,%.4f,%.4f) esperada (%s,%s,%s); desvio %.2e, spin %.2e",
                      l[0].c_str(), l[1].c_str(), v.Vx, v.Vy, v.Vz, l[12].c_str(), l[13].c_str(), l[14].c_str(), d, dw);
        Conferir(d < 1e-3f && dw < 1e-2f && std::fabs(v.Tempo - F(l[15])) < 1e-4f, msg);
    }
}

// Os painéis desenham exatamente o que a física usa: cada ponto da parede, numa grade de 5 cm, cai num painel do
// material que SuperficieDaParede diz — ou em nenhum, se ela diz Aberto. (Mesmo teste do Padel.Core.)
void TestePaineisBatemComAFisica()
{
    for (QualParede parede : {QualParede::Lateral, QualParede::Fundo})
        for (int sinal : {1, -1})
        {
            float limite = parede == QualParede::Lateral ? quadra::MeioComprimento : quadra::MeiaLargura;
            for (float a = -limite + 0.025f; a < limite; a += 0.05f)
                for (float z = 0.025f; z < quadra::AlturaDaParede + 0.5f; z += 0.05f)
                {
                    Superficie esperada = quadra::SuperficieDaParede(parede, a, z);
                    int cobertos = 0;
                    Superficie achada = Superficie::Aberto;
                    for (const auto& p : quadra::Paineis())
                        if (p.Parede == parede && p.Sinal == sinal && a >= p.Inicio && a <= p.Fim && z >= p.ZMin && z <= p.ZMax)
                        {
                            ++cobertos;
                            achada = p.Material;
                        }
                    char msg[160];
                    std::snprintf(msg, sizeof msg, "painel: parede %d sinal %d em (%.3f, %.3f): física %d, painéis %d (%d cobrem)",
                                  (int)parede, sinal, a, z, (int)esperada, (int)achada, cobertos);
                    Conferir(cobertos <= 1 && achada == esperada, msg);
                }
        }
}
} // namespace

int main()
{
    TesteConstantes();
    TesteTrajetorias();
    TesteGolpes();
    TestePaineisBatemComAFisica();
    std::printf("Pior desvio nas trajetórias: posição %.3g m, velocidade %.3g m/s, spin %.3g rad/s\n",
                pior.Posicao, pior.Velocidade, pior.Spin);
    std::printf("PadelSim: %d verificações, %d falha(s).\n", verificacoes, falhas);
    return falhas == 0 ? 0 : 1;
}
