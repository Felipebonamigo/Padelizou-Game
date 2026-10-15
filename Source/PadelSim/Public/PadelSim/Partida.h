// A partida (Padel.Core/Partida.cs): junta placar, árbitro, bola, jogadores e IA numa máquina de estados
// (saque → rally → fim do ponto → saque… → fim). Não desenha nem lê controle: recebe a entrada dos humanos por
// parâmetro e avisa por evento. É o que o Unreal roda por baixo do que mostra.
#pragma once

#include "PadelSim/Aleatorio.h"
#include "PadelSim/Arbitro.h"
#include "PadelSim/Bola.h"
#include "PadelSim/Golpe.h"
#include "PadelSim/Jogadores.h"
#include "PadelSim/Placar.h"

#include <array>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace padel
{
enum class EstadoDaPartida : int32_t { Saque, Rally, FimDoPonto, Fim };
enum class TipoDeEventoDaPartida : int32_t { SaquePreparado, Balanco, Errou, Golpe, Quique, Parede, Rede, CruzouRede, Saiu, Ponto, Game, Set, Partida, Falta, Let, Fim };
/// Manual: o humano aperta pra balançar e o contato sai no instante do balanço. Automatico: bate sozinho com a bola no
/// ponto mais perto do ideal, como a IA (assistência).
enum class ModoDeGolpe : int32_t { Manual, Automatico };

/// Um acontecimento da partida. Jogador: índice (time*2 + índice) ou -1. Em Golpe: o tipo e o lado do corpo.
struct EventoDaPartida
{
    EventoDaPartida(TipoDeEventoDaPartida tipo, int32_t jogador = -1, int32_t time = -1, std::optional<padel::Motivo> motivo = std::nullopt,
                    std::optional<TipoDeGolpe> golpe = std::nullopt, std::optional<LadoDoGolpe> lado = std::nullopt)
        : Tipo(tipo), Jogador(jogador), Time(time), Motivo(motivo), Golpe(golpe), Lado(lado) {}

    TipoDeEventoDaPartida Tipo;
    int32_t Jogador;
    int32_t Time;
    std::optional<padel::Motivo> Motivo;
    std::optional<TipoDeGolpe> Golpe;
    std::optional<LadoDoGolpe> Lado;
};

struct Mensagem
{
    std::string Texto;
    bool Destaque = false, Suave = false;
};

struct GolpeDado
{
    int32_t Jogador;   // time*2 + índice
    TipoDeGolpe Tipo;
    float Em;
    LadoDoGolpe Lado = LadoDoGolpe::Drive;
};

struct OpcoesDaPartida
{
    padel::Dificuldade Dificuldade = padel::Dificuldade::Medio;
    bool PontoDeOuro = true;
    int32_t SetsParaVencer = 1;
    /// Games pra fechar o set; o empate vai ao tie-break. O formato curto da fatia é 4 (Placar::GamesPorSet).
    int32_t GamesPorSet = 6;
    /// Pontos do tie-break; o super tie-break do formato curto é 10.
    int32_t PontosDoTieBreak = 7;
    /// Por jogador (time*2 + índice): quem é humano. Padrão: só o da casa, metade direita.
    std::array<bool, 4> Humanos{true, false, false, false};
    padel::ModoDeGolpe ModoDeGolpe = padel::ModoDeGolpe::Manual;
    /// O esquema de botões do humano (padel::Controle). Quatro botões só no modo Manual: a Partida recusa o Automático.
    padel::Controle Controle = padel::Controle::DoisBotoes;
    std::optional<uint32_t> Semente;
    /// Pontos corridos (Americano/Mexicano): o total que a soma dos pontos precisa alcançar. Vazio = games e sets.
    std::optional<int32_t> PontosCorridos;

    static constexpr std::array<bool, 4> NinguemHumano{false, false, false, false};
};

struct Estatisticas
{
    int32_t Pontos = 0, Golpes = 0, MaiorRally = 0, Faltas = 0, Lets = 0;
    std::map<padel::Motivo, int32_t> Motivos;
};

class Partida
{
public:
    static constexpr float EsperaDoSaqueHumano = 8.f;
    static constexpr float EsperaDoSaqueDaIA = 1.1f;
    static constexpr float LimiarDoLadoForte = 0.7f;
    static constexpr float ErroMaximoDoRemateForte = 0.6f;

    explicit Partida(OpcoesDaPartida opcoes = {});
    Partida(const Partida&) = delete;
    Partida& operator=(const Partida&) = delete;

    static const char* TextoDoMotivo(padel::Motivo motivo);

    OpcoesDaPartida Opcoes;
    padel::Aleatorio Aleatorio;
    padel::Placar Placar;
    padel::Arbitro Arbitro;
    padel::Bola Bola;
    std::vector<Jogador> Jogadores;   // 4: time*2 + índice
    std::vector<IA> IAs;              // 2: um por time
    EstadoDaPartida Estado = EstadoDaPartida::Saque;
    float Temporizador = 0;
    std::optional<padel::Mensagem> Mensagem;
    std::optional<GolpeDado> UltimoGolpe;
    float TempoDeJogo = 0;
    int32_t RallyAtual = 0;
    padel::Estatisticas Estatisticas;
    padel::Caixa CaixaDoSaque{};
    /// Erro (0 = perfeito, 1,6 = péssimo) do último golpe de um humano — pra HUD e pra teste.
    float UltimoErroDoHumano = 0;
    /// Chamado a cada acontecimento, na ordem em que acontecem.
    std::function<void(const EventoDaPartida&)> AoEvento;

    std::array<Jogador*, 2> JogadoresDoTime(int32_t time);
    Jogador& Sacador();
    bool Acabou() const { return Estado == EstadoDaPartida::Fim; }
    void IniciarPonto();
    void Sacar(Entrada entrada);
    /// Avança a simulação. entradas: uma por jogador (time*2 + índice); nullptr = todas vazias.
    void Avancar(float dt, const Entrada* entradas = nullptr);

private:
    float rolandoHa = 0;
    std::array<Entrada, 4> entradas{};

    void Emitir(const EventoDaPartida& evento);
    std::pair<Jogador*, Jogador*> QuemRecebe(int32_t timeReceptor, bool direita);
    const Entrada& EntradaDe(const Jogador& j) const;
    static Entrada Saneada(Entrada e);
    void Rally(float dt);
    void FixarNoPontoDeSaida();
    void Golpear(Jogador& jogador);
    std::optional<IntencaoDoGolpe> IntencaoDoAperto(const Entrada& e) const;
    Golpe GolpeDoHumano(Jogador& jogador, const Entrada& entrada);
    Golpe EscolherGolpeDoHumano(Jogador& jogador, const Entrada& entrada);
    void Decidir(const Decisao& decisao);
    void EncerrarPonto(int32_t para, padel::Motivo motivo);
    std::string TextoDoPonto(int32_t para, padel::Motivo motivo, const EventoDoPlacar& evento) const;
};
} // namespace padel
