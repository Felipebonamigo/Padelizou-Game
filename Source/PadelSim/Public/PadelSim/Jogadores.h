// Jogador (posição, inércia, alcance, balanço) e IA (Padel.Core/Jogadores.cs).
#pragma once

#include "PadelSim/Aleatorio.h"
#include "PadelSim/Bola.h"
#include "PadelSim/Golpe.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace padel
{
class Arbitro;
class Partida;

/// ChanceDeRemateParaFora: com a bola muito alta perto da rede, quanto tenta o remate por 3 / por 4 (se a física deixar).
/// A formação da dupla (Padel.Core/Jogadores.cs, PerfilDeIA, com o porquê de cada número): AtrasoDaFormacao (s) — quanto
/// quem não bateu demora pra acompanhar a dupla e a bola; AcompanhamentoDaBola (0 a 1) — quanto o centro da dupla segue o
/// x da bola; SeparacaoDaDupla (m); FolgaDoBloco (m) — a maior diferença de profundidade aceita do parceiro;
/// ChanceDeDesencontro (0 a 1) — em cada mudança, a chance de quem não bateu nem perceber.
struct PerfilDeIA
{
    float Velocidade, Reacao, ErroDeMira, ErroDePrevisao, ChanceDeErro, ChanceDeLob, ChanceDeRemateParaFora = 0;
    float AtrasoDaFormacao = 0.35f, AcompanhamentoDaBola = 0.35f, SeparacaoDaDupla = 4.0f, FolgaDoBloco = 1.0f, ChanceDeDesencontro = 0;
};

namespace perfis
{
inline constexpr PerfilDeIA Facil{4.6f, 0.45f, 1.5f, 1.1f, 0.14f, 0.2f, 0.05f, 0.7f, 0.15f, 5.0f, 2.5f, 0.3f};
inline constexpr PerfilDeIA Medio{5.6f, 0.25f, 0.9f, 0.7f, 0.07f, 0.3f, 0.15f, 0.35f, 0.35f, 4.0f, 1.0f, 0};
inline constexpr PerfilDeIA Dificil{6.6f, 0.10f, 0.5f, 0.35f, 0.025f, 0.35f, 0.30f, 0.10f, 0.5f, 3.6f, 0.5f, 0};
/// O parceiro do humano: confiável sem ser um muro.
inline constexpr PerfilDeIA Parceiro{5.8f, 0.18f, 0.8f, 0.55f, 0.04f, 0.3f, 0.15f, 0.2f, 0.4f, 3.8f, 0.8f, 0};
const PerfilDeIA& Por(Dificuldade d);
} // namespace perfis

/// Posição, velocidade com inércia (acelera e freia como gente), alcance e o balanço da raquete.
class Jogador
{
public:
    static constexpr float VelocidadeDoHumano = 6.4f;
    static constexpr float DuracaoDoBalanco = 0.3f;
    /// Instante do balanço em que a raquete passa pelo ponto de contato — o único em que ela pode tocar a bola (manual).
    static constexpr float MomentoIdealDoBalanco = 0.12f;
    static constexpr float DistanciaIdealDoContato = 0.6f;
    static constexpr float DistanciaDoCorpo = 0.3f;
    static constexpr float AlturaDaBolaBaixa = 0.3f;
    static constexpr float AlturaDoRevesAlto = 1.6f;
    static constexpr float DistanciaDoVidroDaContrapared = 1.2f;
    static constexpr float PassoPadraoDaPrevisao = 1.f / 120.f;

    Jogador(int32_t time, int32_t indice, std::string nome, bool humano, float velocidade);

    int32_t Time, Indice;
    std::string Nome;
    bool Humano;
    bool Destro = true;
    float Velocidade;
    float Aceleracao = 9.f;
    float Frenagem = 14.f;
    int32_t Lado;
    float X = 0, Y = 0, Vx = 0, Vy = 0;
    float Alcance;
    float AlcanceConfortavel = 0.85f;
    float AlturaMaxima = 2.7f;
    float Cooldown = 0;
    int32_t Golpes = 0;
    int32_t BalancosNoAr = 0;

    float Balanco() const { return balanco; }
    float TempoNoBalanco() const { return tempoNoBalanco; }
    bool BalancoDeLob() const { return balancoDeLob; }
    bool BalancoTerminouNoAr() const { return balancoTerminouNoAr; }
    bool NoInstanteDoContato() const { return noInstanteDoContato; }

    float XDaMetade() const;
    bool Balancando() const { return balanco > 0; }
    float Rapidez() const;
    int32_t LadoDaRaquete() const { return Destro ? 1 : -1; }
    float LateralDe(float x) const;
    float FrontalDe(float y) const;
    LadoDoGolpe LadoDoGolpePara(float x) const;
    LadoDoGolpe LadoDoGolpePara(const Bola& bola) const { return LadoDoGolpePara(bola.X); }
    std::pair<float, float> PontoDeContato(LadoDoGolpe lado) const;
    float DistanciaAoPontoIdeal(float x, float y) const;
    float AtrasoDoContato(const Bola& bola) const;
    float XParaBater(float xDaBola, bool bolaAlta) const;
    float XParaBater(float xDaBola, LadoDoGolpe lado) const;
    LadoDoGolpe LadoParaBater(float xDaBola, bool bolaAlta) const;
    bool BolaAtrasJuntoAoVidro(const Bola& bola) const;
    void Mover(float dx, float dy, float dt);
    bool IrPara(float x, float y, float dt);
    float TempoParaChegar(float d) const;
    float DistanciaAte(float x, float y) const;
    bool Alcanca(const Bola& bola) const;
    bool AlcancaConfortavelmente(const Bola& bola) const;
    bool PodeBaterAgora(const Bola& bola) const;
    bool PodeBaterAgora(const Bola& bola, float dt, bool jaQuicou) const;
    void IniciarBalanco(bool lob);
    void EncerrarBalanco() { balanco = 0; }
    void AvancarTempo(float dt);
    float DificuldadeDoGolpe(const Bola& bola) const;

private:
    float balanco = 0, tempoNoBalanco = 0;
    bool balancoDeLob = false, balancoTerminouNoAr = false, noInstanteDoContato = false;
    float DistanciaAoPontoIdeal(float corpoX, float corpoY, float x, float y) const;
    void Limitar();
};

/// Controla os jogadores de um time que não são humanos: prevê a bola com a mesma física, decide quem busca e onde
/// (com erro de leitura que afina enquanto a bola chega), e escolhe o golpe. Quem não vai na bola joga em formação: os
/// dois andam como um bloco na profundidade (rede ou fundo, subindo e recuando juntos) e na largura (o centro segue a
/// bola; o "cambio" cobre o lado vazio). O parceiro IA de um humano segue a formação do humano. Detalhes no C#.
class IA
{
public:
    static constexpr float ProfundidadeDaRede = 3.2f;
    static constexpr float ProfundidadeDoFundo = 6.5f;
    static constexpr float ProfundidadeMaximaDoBloco = 8.0f;

    IA(int32_t time, const PerfilDeIA& perfil, Aleatorio& aleatorio);

    int32_t Time;
    PerfilDeIA Perfil;
    Formacao Posicao = Formacao::Fundo;

    /// Índice (time*2 + índice) de quem está indo buscar a bola, se houver.
    std::optional<int32_t> Responsavel() const;
    void AoSacar(Partida& partida, int32_t timeSacador);
    void AoGolpear(Partida& partida, const Golpe& golpe, const Jogador& jogador);
    void AoRebater(Partida& partida);
    void Reagir(float dt, Partida& partida);
    Golpe EscolherGolpe(Jogador& jogador, const Bola& bola, Partida& partida);

private:
    struct Plano
    {
        Jogador* Responsavel = nullptr;
        float AlvoY = 0, AlvoT = 0, CriadoEm = 0, PlanejadoEm = 0, XDaBola = 0, RuidoX = 0;
        LadoDoGolpe Lado = LadoDoGolpe::Drive;
    };
    struct Candidato
    {
        float X, Y, Z, T;
        bool Voleio;
    };

    Aleatorio* aleatorio;
    std::optional<Plano> plano;
    float relogio = 0;
    // Formação de cada jogador (pelo Indice): a que ele segue agora, a que vai seguir e quando.
    Formacao formacao[2] = {Formacao::Fundo, Formacao::Fundo};
    std::optional<Formacao> pendente[2];
    float mudaEm[2] = {0, 0};
    bool esperaOParceiro[2] = {false, false};
    bool ladosTrocados = false;
    float xDaBolaLido = 0;
    float profundidadeDoHumanoLida = 0;
    bool humanoNaRede = false;

    void MudarFormacao(Partida& partida, Formacao nova, const Jogador& quem);
    std::pair<float, float> LugarNaFormacao(Jogador& j, const Jogador& parceiro, const std::optional<Plano>& p);
    int32_t VagaDe(const Jogador& j) const;
    void CobrirOOutroLado(const Jogador& j, float xDele, float centro);

    void Planejar(Partida& partida, bool novaTrajetoria);
    float ErroDeLeitura(float segundosAteABola) const;
    float XDoCorpo(const Plano& p, const Jogador& j) const;
    std::vector<Candidato> Prever(const Bola& bola, const Arbitro& arbitro) const;
    Golpe Escolher(Jogador& jogador, const Bola& bola, Partida& partida);
    bool VoltaBativel(const Bola& bola, const Arbitro& arbitro) const;
};
} // namespace padel
