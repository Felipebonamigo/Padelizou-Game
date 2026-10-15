// Placar de padel, puro (Padel.Core/Placar.cs): pontos 0/15/30/40, ponto de ouro ou vantagem, games, tie-break no
// empate em GamesPorSet (6; o formato curto da fatia é 4, com super tie-break a 10), sets, partida em melhor de N; ou
// pontos corridos (Americano/Mexicano). Também diz quem saca e de que lado.
#pragma once

#include "PadelSim/Exportar.h"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace padel
{
enum class TipoDeEventoDoPlacar : int32_t { Ponto, Game, Set, Partida, Encerrada };
enum class LadoDoSaque : int32_t { Direita, Esquerda };

struct PADELSIM_API EventoDoPlacar
{
    TipoDeEventoDoPlacar Tipo;
    int32_t Time;
};
struct PADELSIM_API Sacador
{
    int32_t Time, Jogador;
};
struct PADELSIM_API SetEncerrado
{
    std::array<int32_t, 2> Games;
    std::optional<std::array<int32_t, 2>> TieBreak;
};

class PADELSIM_API Placar
{
public:
    static constexpr int32_t PontosPorTurnoDeSaque = 4;

    explicit Placar(bool pontoDeOuro = true, int32_t setsParaVencer = 1, int32_t timeQueSaca = 0, int32_t gamesPorSet = 6,
                    int32_t pontosDoTieBreak = 7);
    static Placar DePontosCorridos(int32_t total, int32_t timeQueSaca = 0);

    bool PontoDeOuro;
    int32_t SetsParaVencer;
    /// Games pra fechar o set (com 2 de vantagem); o empate nele vai ao tie-break. Padrão 6; o formato curto, 4.
    int32_t GamesPorSet;
    /// Pontos pra fechar o tie-break (com 2 de vantagem). Padrão 7; o super tie-break do formato curto, 10.
    int32_t PontosDoTieBreak;
    std::optional<int32_t> PontosCorridos;
    std::array<int32_t, 2> Pontos{0, 0};
    std::array<int32_t, 2> Games{0, 0};
    std::array<int32_t, 2> Sets{0, 0};
    std::vector<SetEncerrado> SetsAnteriores;
    bool EmTieBreak = false;
    std::optional<int32_t> Vencedor;
    bool Empate = false;
    int32_t SacadorDoGame = 0;

    bool Acabou() const { return Vencedor.has_value() || Empate; }
    Sacador SacadorAtual() const;
    int32_t TotalDePontosNoGame() const { return Pontos[0] + Pontos[1]; }
    LadoDoSaque LadoDoSaqueAtual() const;
    bool EmPontoDecisivo() const;
    std::string TextoDosPontos(int32_t time) const;
    EventoDoPlacar PontoPara(int32_t time);
    std::string Resumo() const;

private:
    std::array<int32_t, 2> proximoJogador{0, 0};
    std::array<int32_t, 2> jogadorSacador{0, 0};
    int32_t turnoDeSaque = 0;

    void AbrirTurnoDeSaque(int32_t time);
    EventoDoPlacar PontoCorrido(int32_t time, int32_t total);
    void RodarSaqueNoTieBreak();
    EventoDoPlacar FecharGame(int32_t time);
    EventoDoPlacar FecharSet(int32_t time, std::optional<std::array<int32_t, 2>> pontosDoTieBreak);
};
} // namespace padel
