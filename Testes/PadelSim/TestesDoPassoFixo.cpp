// O PassoFixo contra o gabarito (Gabarito/passo-fixo.csv, de GabaritoDoPassoFixo.cs): mesmas sequências de quadros,
// mesmos passos por quadro e a mesma entrada em cada passo.
#include "PadelSim/PassoFixo.h"

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

int main()
{
    std::ifstream arq(std::string(PASTA_DO_GABARITO) + "/passo-fixo.csv");
    if (!arq) { std::printf("FALHA: não abriu passo-fixo.csv\n"); return 1; }
    std::vector<std::vector<std::string>> linhas;
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
    int falhas = 0, verificacoes = 0;
    auto F = [](const std::string& x) { return std::strtof(x.c_str(), nullptr); };
    auto B = [](const std::string& x) { return x == "1"; };
    std::map<int, padel::PassoFixo> porSequencia;
    int quadroAtual = -1, idAtual = -1, passosDoQuadro = 0;
    for (const auto& l : linhas)
    {
        int id = std::atoi(l[0].c_str()), q = std::atoi(l[1].c_str());
        auto& p = porSequencia.try_emplace(id, 120).first->second;
        if (id != idAtual || q != quadroAtual)
        {
            idAtual = id; quadroAtual = q;
            padel::Entrada e{F(l[3]), F(l[4]), B(l[5]), B(l[6]), B(l[7]), B(l[8]), B(l[9]), B(l[10]), B(l[11])};
            passosDoQuadro = p.Quadro(std::strtod(l[2].c_str(), nullptr), e);
            ++verificacoes;
            if (passosDoQuadro != std::atoi(l[12].c_str()) && ++falhas <= 20)
                std::printf("FALHA: sequência %d, quadro %d (delta %s): %d passos, esperado %s\n", id, q, l[2].c_str(), passosDoQuadro, l[12].c_str());
        }
        int i = std::atoi(l[13].c_str());
        if (i < 0 || i >= passosDoQuadro) continue;
        auto x = p.EntradaDoPasso(i);
        ++verificacoes;
        if (!(x.Dx == F(l[14]) && x.Dy == F(l[15]) && x.AcaoPressionada == B(l[16]) && x.AcaoSegurada == B(l[17]) && x.LobPressionada == B(l[18]) &&
              x.CortadoPressionado == B(l[19]) && x.AtaquePressionado == B(l[20]) && x.AtaqueSegurado == B(l[21]) && x.ModificadorSegurado == B(l[22])) &&
            ++falhas <= 20)
            std::printf("FALHA: sequência %d, quadro %d, passo %d: entrada diferente\n", id, q, i);
    }
    std::printf("PassoFixo: %d verificações, %d falha(s).\n", verificacoes, falhas);
    return falhas == 0 ? 0 : 1;
}
