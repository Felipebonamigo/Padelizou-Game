// Partidas inteiras contra o gabarito do Padel.Core (Gabarito/partidas*.csv, de GabaritoDaPartida.cs): mesma semente,
// mesmas opções, 120 passos por segundo — e o C++ tem de dar o mesmo retrato a cada segundo de jogo e os mesmos
// eventos, com tolerância ZERO. Cobre placar, árbitro, jogadores, IA, golpes especiais e o humano simulado.
#include "PadelSim/HumanoSimulado.h"
#include "PadelSim/Partida.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
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
        linhas.push_back(l);
    }
    return linhas;
}

std::string F(float f)
{
    // O mesmo texto que o C# grava ("R"): o float mais curto que volta igual. Comparar o float lido basta.
    char b[32];
    std::snprintf(b, sizeof b, "%.9g", f);
    return b;
}
bool Igual(const std::string& esperado, float obtido) { return std::strtof(esperado.c_str(), nullptr) == obtido; }
bool Igual(const std::string& esperado, int obtido) { return std::atoi(esperado.c_str()) == obtido; }

const PerfilDeHumano& PerfilPeloNome(const std::string& nome)
{
    for (const PerfilDeHumano* p : {&PerfilDeHumano::Iniciante, &PerfilDeHumano::Intermediario, &PerfilDeHumano::Avancado, &PerfilDeHumano::Profissional})
        if (p->Nome == nome) return *p;
    std::printf("FALHA: perfil %s desconhecido\n", nome.c_str());
    std::exit(1);
}

std::vector<std::string> Retrato(int id, int passo, const Partida& p)
{
    const auto& b = p.Bola;
    std::vector<std::string> s{
        std::to_string(id), std::to_string(passo), std::to_string((int)p.Estado), F(p.Temporizador), F(p.TempoDeJogo), std::to_string(p.RallyAtual), F(p.UltimoErroDoHumano),
        F(b.X), F(b.Y), F(b.Z), F(b.Vx), F(b.Vy), F(b.Vz), F(b.Wx), F(b.Wy), F(b.Wz), b.EmJogo ? "1" : "0", b.Rolando ? "1" : "0", b.Parada ? "1" : "0",
        std::to_string(p.Placar.Pontos[0]), std::to_string(p.Placar.Pontos[1]), std::to_string(p.Placar.Games[0]), std::to_string(p.Placar.Games[1]),
        std::to_string(p.Placar.Sets[0]), std::to_string(p.Placar.Sets[1]), p.Placar.EmTieBreak ? "1" : "0",
        std::to_string(p.Placar.SacadorAtual().Time), std::to_string(p.Placar.SacadorAtual().Jogador),
        std::to_string(p.Arbitro.Faltas), std::to_string(p.Arbitro.Golpeador.value_or(-1)), p.Arbitro.QuicouNoReceptor ? "1" : "0",
        std::to_string((int)p.IAs[0].Posicao), std::to_string((int)p.IAs[1].Posicao),
    };
    for (const auto& j : p.Jogadores)
        for (const auto& v : {F(j.X), F(j.Y), F(j.Vx), F(j.Vy), F(j.Balanco()), F(j.TempoNoBalanco()), F(j.Cooldown), std::to_string(j.Golpes)})
            s.push_back(v);
    return s;
}

/// Compara campo a campo, como número (o texto do float pode variar entre o "R" do C# e o %.9g).
bool MesmoRetrato(const Linha& esperado, const std::vector<std::string>& obtido, std::string& diferenca, const Linha& nomes)
{
    if (esperado.size() != obtido.size()) { diferenca = "número de campos"; return false; }
    for (size_t i = 0; i < esperado.size(); i++)
    {
        bool ok = std::strtof(esperado[i].c_str(), nullptr) == std::strtof(obtido[i].c_str(), nullptr);
        if (!ok)
        {
            diferenca += nomes[i] + ": esperado " + esperado[i] + ", obtido " + obtido[i] + "; ";
        }
    }
    return diferenca.empty();
}

Linha NomesDasColunas(const std::string& arquivo)
{
    std::ifstream arq(std::string(PASTA_DO_GABARITO) + "/" + arquivo);
    std::string s;
    while (std::getline(arq, s) && (s.empty() || s[0] == '#')) {}
    Linha l;
    std::stringstream ss(s);
    std::string campo;
    while (std::getline(ss, campo, ',')) l.push_back(campo);
    return l;
}
} // namespace

// Revisão de 15/10: no C# as opções da partida não mudam depois de criada; aqui um campo público mutável deixava trocar o
// modo de golpe depois do construtor, furando a recusa de quatro botões no Automático.
static_assert(std::is_const_v<decltype(padel::Partida::Opcoes)>, "Partida::Opcoes tem de ser const, como no C#");

int main()
{
    constexpr float Passo = 1.f / 120.f;
    constexpr int PassosPorRetrato = 120;
    const auto casos = LerCsv("partidas.csv");
    const auto nomes = NomesDasColunas("partidas-retratos.csv");
    std::map<int, std::vector<Linha>> retratos, eventos;
    for (auto& l : LerCsv("partidas-retratos.csv")) retratos[std::atoi(l[0].c_str())].push_back(l);
    for (auto& l : LerCsv("partidas-eventos.csv")) eventos[std::atoi(l[0].c_str())].push_back(l);

    for (const auto& c : casos)
    {
        const int id = std::atoi(c[0].c_str());
        const std::string nome = c[1];
        OpcoesDaPartida o;
        o.Dificuldade = static_cast<Dificuldade>(std::atoi(c[2].c_str()));
        o.PontoDeOuro = c[3] == "1";
        o.SetsParaVencer = std::atoi(c[4].c_str());
        o.GamesPorSet = std::atoi(c[5].c_str());
        o.PontosDoTieBreak = std::atoi(c[6].c_str());
        bool temHumano = c[7] == "1";
        o.Humanos = temHumano ? std::array<bool, 4>{true, false, false, false} : OpcoesDaPartida::NinguemHumano;
        o.ModoDeGolpe = static_cast<ModoDeGolpe>(std::atoi(c[9].c_str()));
        o.Semente = static_cast<uint32_t>(std::strtoul(c[10].c_str(), nullptr, 10));
        if (std::atoi(c[11].c_str()) >= 0) o.PontosCorridos = std::atoi(c[11].c_str());
        o.Controle = static_cast<Controle>(std::atoi(c[21].c_str()));
        o.Assistencia = static_cast<Assistencia>(std::atoi(c[22].c_str()));
        const int passosEsperados = std::atoi(c[12].c_str());

        int passo = 0;
        int falhasDaPartida = 0;
        size_t proximoEvento = 0, proximoRetrato = 0;
        const auto& evEsperados = eventos[id];
        const auto& retEsperados = retratos[id];
        auto Falhar = [&](const std::string& msg) {
            ++falhas;
            if (++falhasDaPartida <= 5) std::printf("FALHA: partida %d (%s), passo %d: %s\n", id, nome.c_str(), passo, msg.c_str());
        };

        Partida p(o);   // o SaquePreparado do construtor sai antes de haver quem ouça — no C# também (o gabarito não o tem)
        p.AoEvento = [&](const EventoDaPartida& e) {
            ++verificacoes;
            if (proximoEvento >= evEsperados.size()) { Falhar("evento a mais, tipo " + std::to_string((int)e.Tipo)); return; }
            const auto& x = evEsperados[proximoEvento++];
            bool ok = Igual(x[1], passo) && Igual(x[2], (int)e.Tipo) && Igual(x[3], e.Jogador) && Igual(x[4], e.Time) &&
                      Igual(x[5], e.Motivo ? (int)*e.Motivo : -1) && Igual(x[6], e.Golpe ? (int)*e.Golpe : -1) && Igual(x[7], e.Lado ? (int)*e.Lado : -1);
            if (!ok)
            {
                char msg[256];
                std::snprintf(msg, sizeof msg, "evento (tipo %d, jogador %d, time %d, motivo %d, golpe %d, lado %d), esperado no passo %s (tipo %s, jogador %s, time %s, motivo %s, golpe %s, lado %s)",
                              (int)e.Tipo, e.Jogador, e.Time, e.Motivo ? (int)*e.Motivo : -1, e.Golpe ? (int)*e.Golpe : -1, e.Lado ? (int)*e.Lado : -1,
                              x[1].c_str(), x[2].c_str(), x[3].c_str(), x[4].c_str(), x[5].c_str(), x[6].c_str(), x[7].c_str());
                Falhar(msg);
            }
        };
        std::optional<padel::Aleatorio> aleatorioDoHumano;
        std::optional<HumanoSimulado> humano;
        if (temHumano)
        {
            aleatorioDoHumano.emplace(*o.Semente + 1000);
            humano.emplace(0, PerfilPeloNome(c[8]), *aleatorioDoHumano, true, o.Controle);
        }
        std::array<Entrada, 4> entradas{};

        auto ConferirRetrato = [&]() {
            ++verificacoes;
            if (proximoRetrato >= retEsperados.size()) { Falhar("retrato a mais"); return; }
            std::string diferenca;
            if (!MesmoRetrato(retEsperados[proximoRetrato++], Retrato(id, passo, p), diferenca, nomes)) Falhar("retrato: " + diferenca);
        };
        ConferirRetrato();
        while (!p.Acabou() && passo < 120 * 60 * 40 && falhasDaPartida < 5)
        {
            passo++;
            if (humano) entradas[0] = humano->Decidir(EstadoVisivel::De(p), Passo);
            p.Avancar(Passo, entradas.data());
            if (passo % PassosPorRetrato == 0 || p.Acabou()) ConferirRetrato();
        }
        ++verificacoes;
        char resumo[300];
        std::snprintf(resumo, sizeof resumo, "fim: %d passos e \"%s\" (esperado %d e \"%s\"); %zu de %zu eventos, %zu de %zu retratos",
                      passo, p.Placar.Resumo().c_str(), passosEsperados, c[13].c_str(), proximoEvento, evEsperados.size(), proximoRetrato, retEsperados.size());
        bool fimOk = passo == passosEsperados && p.Placar.Resumo() == c[13] && proximoEvento == evEsperados.size() && proximoRetrato == retEsperados.size() &&
                     p.Estatisticas.Pontos == std::atoi(c[16].c_str()) && p.Estatisticas.Golpes == std::atoi(c[17].c_str()) &&
                     p.Estatisticas.MaiorRally == std::atoi(c[18].c_str()) && p.Estatisticas.Faltas == std::atoi(c[19].c_str()) && p.Estatisticas.Lets == std::atoi(c[20].c_str());
        if (!fimOk) Falhar(resumo);
        std::printf("partida %d (%s): %s\n", id, nome.c_str(), falhasDaPartida == 0 ? "igual ao Padel.Core" : "DIFERENTE");
    }
    // Como no Core (ControleDeQuatroBotoesTests): quatro botões só no modo Manual, e a Partida recusa o Automático.
    {
        OpcoesDaPartida o;
        o.Controle = Controle::QuatroBotoes;
        o.ModoDeGolpe = ModoDeGolpe::Automatico;
        bool recusou = false;
        try { Partida p(o); } catch (const std::invalid_argument&) { recusou = true; }
        ++verificacoes;
        if (!recusou) { ++falhas; std::printf("FALHA: a Partida aceitou quatro botões no modo Automático\n"); }
    }
    std::printf("Partidas: %d verificações, %d falha(s).\n", verificacoes, falhas);
    return falhas == 0 ? 0 : 1;
}
