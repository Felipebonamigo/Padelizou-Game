// O rastro-fantasma contra o gabarito (Gabarito/rastros.csv e rastros-pontos.csv, de GabaritoDoRastro.cs): as mesmas
// bolas, o mesmo número de pontos, o mesmo começo da saída de parede e cada ponto bit a bit.
#include "PadelSim/RastroFantasma.h"

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

namespace
{
std::vector<std::vector<std::string>> LerCsv(const std::string& nome)
{
    std::vector<std::vector<std::string>> linhas;
    std::ifstream arq(std::string(PASTA_DO_GABARITO) + "/" + nome);
    if (!arq) { std::printf("FALHA: não abriu %s\n", nome.c_str()); std::exit(1); }
    std::string s;
    bool cabecalho = true;
    while (std::getline(arq, s))
    {
        if (s.empty() || s[0] == '#') continue;
        if (cabecalho) { cabecalho = false; continue; }
        std::vector<std::string> l;
        std::stringstream ss(s);
        std::string campo;
        while (std::getline(ss, campo, ',')) l.push_back(campo);
        linhas.push_back(l);
    }
    return linhas;
}

float F(const std::string& x) { return std::strtof(x.c_str(), nullptr); }
} // namespace

int main()
{
    int verificacoes = 0, falhas = 0;
    std::map<int, std::vector<std::vector<std::string>>> pontos;
    for (auto& l : LerCsv("rastros-pontos.csv")) pontos[std::atoi(l[0].c_str())].push_back(l);
    for (const auto& c : LerCsv("rastros.csv"))
    {
        int id = std::atoi(c[0].c_str());
        padel::Bola bola;
        bola.Posicionar(F(c[2]), F(c[3]), F(c[4]));
        if (c[12] == "1")
        {
            padel::Velocidade v;
            v.Vx = F(c[5]); v.Vy = F(c[6]); v.Vz = F(c[7]);
            v.Wx = F(c[8]); v.Wy = F(c[9]); v.Wz = F(c[10]);
            bola.Lancar(v);
        }
        std::vector<padel::EventoDaBola> eventos;
        for (int i = 0, n = std::atoi(c[13].c_str()); i < n; i++) bola.Avancar(padel::rastro_fantasma::Passo, eventos);
        auto rastro = padel::rastro_fantasma::Prever(bola, std::atoi(c[11].c_str()), c[14] == "1");
        int falhasDoCaso = 0;
        auto Falhar = [&](const std::string& msg) {
            ++falhas;
            if (++falhasDoCaso <= 5) std::printf("FALHA: rastro %d (%s): %s\n", id, c[1].c_str(), msg.c_str());
        };
        ++verificacoes;
        if ((int)rastro.Pontos.size() != std::atoi(c[15].c_str()))
            Falhar(std::to_string(rastro.Pontos.size()) + " pontos, esperado " + c[15]);
        ++verificacoes;
        if (rastro.PrimeiroDepoisDaParede != std::atoi(c[16].c_str()))
            Falhar("saída de parede no " + std::to_string(rastro.PrimeiroDepoisDaParede) + ", esperado " + c[16]);
        for (const auto& p : pontos[id])
        {
            size_t i = std::strtoul(p[1].c_str(), nullptr, 10);
            ++verificacoes;
            if (i >= rastro.Pontos.size()) { Falhar("falta o ponto " + p[1]); continue; }
            const auto& r = rastro.Pontos[i];
            if (!(r.X == F(p[2]) && r.Y == F(p[3]) && r.Z == F(p[4]) && r.T == F(p[5])))
            {
                char msg[200];
                std::snprintf(msg, sizeof msg, "ponto %zu: (%.9g, %.9g, %.9g, %.9g), esperado (%s, %s, %s, %s)", i, r.X, r.Y, r.Z, r.T,
                              p[2].c_str(), p[3].c_str(), p[4].c_str(), p[5].c_str());
                Falhar(msg);
            }
        }
        std::printf("rastro %d (%s): %s\n", id, c[1].c_str(), falhasDoCaso == 0 ? "igual ao Padel.Core" : "DIFERENTE");
    }
    std::printf("Rastros: %d verificações, %d falha(s).\n", verificacoes, falhas);
    return falhas == 0 ? 0 : 1;
}
