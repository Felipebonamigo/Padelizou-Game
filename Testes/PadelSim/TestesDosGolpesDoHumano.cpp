// Os golpes do humano em cenas montadas contra o gabarito (Gabarito/golpes-do-humano.csv, de GabaritoDosGolpesDoHumano.cs):
// os caminhos que as partidas não exercitam — A segurado, por 3, smash de recurso, botões juntos, víbora com RB e a janela
// do timing. A cena e o passo do aperto são os do gerador; confere o golpe, o passo, a bola e o erro, bit a bit.
#include "PadelSim/Partida.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#ifndef PASTA_DO_GABARITO
#error "defina PASTA_DO_GABARITO (o CMakeLists.txt define)"
#endif

using namespace padel;

namespace
{
constexpr float Passo = 1.f / 120.f;

float TempoAteOAlcance(const Bola& bola, const Jogador& j)
{
    Bola clone = bola;
    std::vector<EventoDaBola> eventos;
    for (float t = 0; t < 3; t += Passo)
    {
        if (j.AlcancaConfortavelmente(clone)) return t;
        clone.Avancar(Passo, eventos);
    }
    return -1;
}

float F(const std::string& x) { return std::strtof(x.c_str(), nullptr); }
} // namespace

int main()
{
    std::ifstream arq(std::string(PASTA_DO_GABARITO) + "/golpes-do-humano.csv");
    if (!arq) { std::printf("FALHA: não abriu golpes-do-humano.csv\n"); return 1; }
    int verificacoes = 0, falhas = 0;
    std::string s;
    bool cabecalho = true;
    while (std::getline(arq, s))
    {
        if (s.empty() || s[0] == '#') continue;
        if (cabecalho) { cabecalho = false; continue; }
        std::vector<std::string> c;
        std::stringstream ss(s);
        std::string campo;
        while (std::getline(ss, campo, ',')) c.push_back(campo);

        bool alta = c[3] == "1";
        float jy = F(c[4]);
        const std::string& botoes = c[5];
        bool seguraA = c[6] == "1", seguraY = c[7] == "1", rb = c[8] == "1";
        float dx = F(c[9]), dy = F(c[10]);
        OpcoesDaPartida o;
        o.Semente = static_cast<uint32_t>(std::strtoul(c[1].c_str(), nullptr, 10));
        o.Humanos = {true, false, false, false};
        o.Controle = Controle::QuatroBotoes;
        o.Assistencia = static_cast<Assistencia>(std::atoi(c[11].c_str()));
        int adiantamento = std::atoi(c[12].c_str());

        Partida partida(o);
        Entrada saque[4]{};
        saque[0].AcaoPressionada = true;
        saque[0].AcaoSegurada = true;
        partida.Avancar(Passo, saque);
        Jogador& humano = partida.Jogadores[0];
        float jx = 2.0f, bx = 2.6f, by = alta ? jy : 5.5f, bz = alta ? 3.3f : 1.0f;
        Velocidade v;
        if (!alta) { v.Vy = 7; v.Vz = 1.5f; }
        partida.Arbitro.RegistrarGolpe(1);
        if (!alta) partida.Arbitro.Processar({TipoDeEventoDaBola::Quique, bx, by, 0, 1});
        humano.X = jx; humano.Y = jy; humano.Vx = 0; humano.Vy = 0; humano.Cooldown = 0;
        partida.Bola.Posicionar(bx, by, bz);
        partida.Bola.Lancar(v);

        int referencia = -1;
        Bola copia = partida.Bola;
        std::vector<EventoDaBola> eventos;
        for (int i = 0; i < 240 && referencia < 0; i++)
        {
            float falta = TempoAteOAlcance(copia, humano);
            if (falta >= 0 && falta <= Jogador::MomentoIdealDoBalanco) referencia = i;
            copia.Avancar(Passo, eventos);
        }
        int aperto = referencia - adiantamento;

        int tipo = -1, passoDoGolpe = -1, passo = 0;
        Bola bolaNoGolpe;
        float erro = 0;
        partida.AoEvento = [&](const EventoDaPartida& ev) {
            if (ev.Tipo == TipoDeEventoDaPartida::Golpe && ev.Jogador == 0 && tipo < 0)
            {
                tipo = static_cast<int>(*ev.Golpe);
                passoDoGolpe = passo;
                bolaNoGolpe = partida.Bola;
                erro = partida.UltimoErroDoHumano;
            }
        };
        auto Tem = [&](char b) { return botoes.find(b) != std::string::npos; };
        for (passo = 0; passo < 240 && tipo < 0 && partida.Estado == EstadoDaPartida::Rally; passo++)
        {
            bool apertar = passo == aperto, depois = passo >= aperto;
            Entrada es[4]{};
            if (depois)
            {
                Entrada& e = es[0];
                e.Dx = dx; e.Dy = dy;
                e.AcaoPressionada = apertar && Tem('A'); e.AcaoSegurada = Tem('A') && (apertar || seguraA);
                e.LobPressionada = apertar && Tem('B');
                e.CortadoPressionado = apertar && Tem('X');
                e.AtaquePressionado = apertar && Tem('Y'); e.AtaqueSegurado = Tem('Y') && (apertar || seguraY);
                e.ModificadorSegurado = rb;
            }
            partida.Avancar(Passo, es);
        }

        ++verificacoes;
        bool ok = tipo == std::atoi(c[13].c_str()) && passoDoGolpe == std::atoi(c[14].c_str());
        if (tipo >= 0)
            ok = ok && bolaNoGolpe.Vx == F(c[15]) && bolaNoGolpe.Vy == F(c[16]) && bolaNoGolpe.Vz == F(c[17]) && bolaNoGolpe.Wx == F(c[18]) &&
                 bolaNoGolpe.Wy == F(c[19]) && bolaNoGolpe.Wz == F(c[20]) && erro == F(c[21]);
        if (!ok && ++falhas <= 20)
            std::printf("FALHA: cena %s (%s, semente %s): golpe %d no passo %d, erro %.9g; esperado golpe %s no passo %s, erro %s\n", c[0].c_str(),
                        c[2].c_str(), c[1].c_str(), tipo, passoDoGolpe, erro, c[13].c_str(), c[14].c_str(), c[21].c_str());
    }
    std::printf("Golpes do humano: %d cenas, %d falha(s).\n", verificacoes, falhas);
    return falhas == 0 ? 0 : 1;
}
