// Placar e árbitro contra o gabarito (Gabarito/placar.csv e arbitro.csv, de GabaritoDoPlacarEDoArbitro.cs): o que as
// partidas não alcançam — tie-break, vários sets, vantagem longa, empate nos pontos corridos, formato curto (set de 4,
// super tie-break a 10), falta, let, dupla falta.
#include "PadelSim/Arbitro.h"
#include "PadelSim/Placar.h"
#include "PadelSim/Quadra.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#ifndef PASTA_DO_GABARITO
#error "defina PASTA_DO_GABARITO (o CMakeLists.txt define)"
#endif

using namespace padel;

namespace
{
int falhas = 0, verificacoes = 0;
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
        if (!s.empty() && s.back() == ',') l.push_back("");
        linhas.push_back(l);
    }
    return linhas;
}

int I(const std::string& s) { return std::atoi(s.c_str()); }
std::string B(bool b) { return b ? "1" : "0"; }

void Conferir(bool ok, const std::string& msg)
{
    ++verificacoes;
    if (!ok && ++falhas <= 20) std::printf("FALHA: %s\n", msg.c_str());
}

void TestePlacar()
{
    std::map<int, std::optional<Placar>> placares;
    for (const auto& l : LerCsv("placar.csv"))
    {
        int id = I(l[0]);
        auto& p = placares[id];
        if (!p)
        {
            int corridos = I(l[3]);
            p = corridos >= 0 ? Placar::DePontosCorridos(corridos, I(l[4])) : Placar(l[1] == "1", I(l[2]), I(l[4]), I(l[5]), I(l[6]));
        }
        EventoDoPlacar ev = p->PontoPara(I(l[8]));
        auto s = p->SacadorAtual();
        std::vector<std::string> obtido{
            std::to_string((int)ev.Tipo), std::to_string(ev.Time), std::to_string(p->Pontos[0]), std::to_string(p->Pontos[1]),
            std::to_string(p->Games[0]), std::to_string(p->Games[1]), std::to_string(p->Sets[0]), std::to_string(p->Sets[1]),
            B(p->EmTieBreak), std::to_string(p->Vencedor.value_or(-1)), B(p->Empate), std::to_string(s.Time), std::to_string(s.Jogador),
            std::to_string((int)p->LadoDoSaqueAtual()), B(p->EmPontoDecisivo()), p->TextoDosPontos(0), p->TextoDosPontos(1), p->Resumo()};
        std::string diferenca;
        for (size_t i = 0; i < obtido.size(); i++)
            if (obtido[i] != l[9 + i]) diferenca += "campo " + std::to_string(9 + i) + ": esperado '" + l[9 + i] + "', obtido '" + obtido[i] + "'; ";
        Conferir(diferenca.empty(), "placar " + l[0] + ", ponto " + l[7] + ": " + diferenca);
    }
}

void TesteArbitro()
{
    std::map<int, Arbitro> arbitros;
    for (const auto& l : LerCsv("arbitro.csv"))
    {
        Arbitro& a = arbitros[I(l[0])];
        std::optional<Decisao> d;
        const std::string& acao = l[2];
        if (acao == "saque") a.IniciarSaque(I(l[3]), CaixaDeSaque(I(l[4]), l[5] == "1"));
        else if (acao == "golpe") a.RegistrarGolpe(I(l[3]));
        else if (acao == "novo") a.NovoPonto();
        else
        {
            EventoDaBola e{static_cast<TipoDeEventoDaBola>(I(l[6])), std::strtof(l[7].c_str(), nullptr), std::strtof(l[8].c_str(), nullptr),
                           std::strtof(l[9].c_str(), nullptr), I(l[10]), static_cast<QualParede>(I(l[11])), static_cast<Superficie>(I(l[12])), l[13] == "1"};
            d = a.Processar(e);
        }
        std::vector<std::string> obtido{
            std::to_string(d ? (int)d->Tipo : -1), std::to_string(d ? d->Para : -2), std::to_string(d ? (int)d->Motivo : -1),
            std::to_string(a.Faltas), B(a.EmSaque), std::to_string(a.Golpeador.value_or(-1)), B(a.QuicouNoReceptor), B(a.TocouARede),
            B(a.Cruzou), B(a.PodeGolpear(0)), B(a.PodeGolpear(1))};
        std::string diferenca;
        for (size_t i = 0; i < obtido.size(); i++)
            if (obtido[i] != l[14 + i]) diferenca += "campo " + std::to_string(14 + i) + ": esperado " + l[14 + i] + ", obtido " + obtido[i] + "; ";
        Conferir(diferenca.empty(), "árbitro " + l[0] + ", passo " + l[1] + " (" + acao + "): " + diferenca);
    }
}
} // namespace

int main()
{
    TestePlacar();
    TesteArbitro();
    std::printf("Placar e árbitro: %d verificações, %d falha(s).\n", verificacoes, falhas);
    return falhas == 0 ? 0 : 1;
}
