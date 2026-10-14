#include "PadelSim/Placar.h"

#include <algorithm>
#include <stdexcept>

namespace padel
{
Placar::Placar(bool pontoDeOuro, int32_t setsParaVencer, int32_t timeQueSaca, int32_t gamesPorSet, int32_t pontosDoTieBreak)
    : PontoDeOuro(pontoDeOuro), SetsParaVencer(setsParaVencer), GamesPorSet(gamesPorSet), PontosDoTieBreak(pontosDoTieBreak), SacadorDoGame(timeQueSaca)
{
    if (gamesPorSet < 1) throw std::out_of_range("gamesPorSet");
    if (pontosDoTieBreak < 1) throw std::out_of_range("pontosDoTieBreak");
    AbrirTurnoDeSaque(timeQueSaca);
}

Placar Placar::DePontosCorridos(int32_t total, int32_t timeQueSaca)
{
    if (total < 1) throw std::out_of_range("A partida de pontos corridos vale pelo menos 1 ponto.");
    if (timeQueSaca != 0 && timeQueSaca != 1) throw std::out_of_range("O time é 0 (casa) ou 1 (visitante).");
    Placar p(false, 1, timeQueSaca);
    p.PontosCorridos = total;
    return p;
}

void Placar::AbrirTurnoDeSaque(int32_t time)
{
    turnoDeSaque = time;
    jogadorSacador[time] = proximoJogador[time];
    proximoJogador[time] ^= 1;
}

Sacador Placar::SacadorAtual() const { return {turnoDeSaque, jogadorSacador[turnoDeSaque]}; }
LadoDoSaque Placar::LadoDoSaqueAtual() const { return TotalDePontosNoGame() % 2 == 0 ? LadoDoSaque::Direita : LadoDoSaque::Esquerda; }
bool Placar::EmPontoDecisivo() const { return !EmTieBreak && PontoDeOuro && Pontos[0] == 3 && Pontos[1] == 3; }

std::string Placar::TextoDosPontos(int32_t time) const
{
    static const char* const Nomes[] = {"0", "15", "30", "40"};
    if (EmTieBreak || PontosCorridos) return std::to_string(Pontos[time]);
    int32_t a = Pontos[time], b = Pontos[1 - time];
    if (a >= 3 && b >= 3) return a > b ? "AD" : "40";
    return Nomes[std::min(a, 3)];
}

EventoDoPlacar Placar::PontoPara(int32_t time)
{
    if (Acabou()) return {TipoDeEventoDoPlacar::Encerrada, Vencedor.value_or(-1)};
    if (time != 0 && time != 1) throw std::out_of_range("time");
    Pontos[time] += 1;
    if (PontosCorridos) return PontoCorrido(time, *PontosCorridos);
    int32_t a = Pontos[time], b = Pontos[1 - time];
    if (EmTieBreak)
    {
        if (a >= PontosDoTieBreak && a - b >= 2) return FecharGame(time);
        RodarSaqueNoTieBreak();
        return {TipoDeEventoDoPlacar::Ponto, time};
    }
    bool fechou = PontoDeOuro ? (a >= 4 && a > b) : (a >= 4 && a - b >= 2);
    return fechou ? FecharGame(time) : EventoDoPlacar{TipoDeEventoDoPlacar::Ponto, time};
}

EventoDoPlacar Placar::PontoCorrido(int32_t time, int32_t total)
{
    int32_t soma = TotalDePontosNoGame();
    if (soma >= total)
    {
        if (Pontos[0] == Pontos[1]) Empate = true;
        else Vencedor = Pontos[0] > Pontos[1] ? 0 : 1;
        return {TipoDeEventoDoPlacar::Partida, Vencedor.value_or(-1)};
    }
    if (soma % PontosPorTurnoDeSaque == 0)
    {
        SacadorDoGame = 1 - SacadorDoGame;
        AbrirTurnoDeSaque(SacadorDoGame);
    }
    return {TipoDeEventoDoPlacar::Ponto, time};
}

void Placar::RodarSaqueNoTieBreak()
{
    int32_t n = TotalDePontosNoGame();
    int32_t turno = ((n + 1) / 2) % 2 == 0 ? SacadorDoGame : 1 - SacadorDoGame;
    if (turno != turnoDeSaque) AbrirTurnoDeSaque(turno);
}

EventoDoPlacar Placar::FecharGame(int32_t time)
{
    bool eraTieBreak = EmTieBreak;
    std::optional<std::array<int32_t, 2>> pontosDoTieBreak;
    if (eraTieBreak) pontosDoTieBreak = Pontos;
    Games[time] += 1;
    Pontos = {0, 0};
    EmTieBreak = false;
    int32_t g = Games[time], h = Games[1 - time];
    EventoDoPlacar evento;
    if (eraTieBreak || (g >= GamesPorSet && g - h >= 2))
    {
        evento = FecharSet(time, pontosDoTieBreak);
    }
    else
    {
        if (g == GamesPorSet && h == GamesPorSet) EmTieBreak = true;
        evento = {TipoDeEventoDoPlacar::Game, time};
    }
    // Próximo game: o outro time saca. Depois do tie-break, saca quem NÃO abriu o tie-break.
    SacadorDoGame = 1 - SacadorDoGame;
    if (!Acabou()) AbrirTurnoDeSaque(SacadorDoGame);
    return evento;
}

EventoDoPlacar Placar::FecharSet(int32_t time, std::optional<std::array<int32_t, 2>> pontosDoTieBreak)
{
    Sets[time] += 1;
    SetsAnteriores.push_back({Games, pontosDoTieBreak});
    Games = {0, 0};
    if (Sets[time] >= SetsParaVencer)
    {
        Vencedor = time;
        return {TipoDeEventoDoPlacar::Partida, time};
    }
    return {TipoDeEventoDoPlacar::Set, time};
}

std::string Placar::Resumo() const
{
    if (PontosCorridos) return std::to_string(Pontos[0]) + "-" + std::to_string(Pontos[1]);
    std::string texto;
    auto Juntar = [&](const std::string& parte) { texto += (texto.empty() ? "" : " ") + parte; };
    for (const auto& s : SetsAnteriores)
    {
        std::string parte = std::to_string(s.Games[0]) + "-" + std::to_string(s.Games[1]);
        if (s.TieBreak) parte += "(" + std::to_string(std::min((*s.TieBreak)[0], (*s.TieBreak)[1])) + ")";
        Juntar(parte);
    }
    if (!Acabou()) Juntar(std::to_string(Games[0]) + "-" + std::to_string(Games[1]));
    return texto;
}
} // namespace padel
