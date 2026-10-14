// Um jogador automático que joga PELA ENTRADA, como uma pessoa (Padel.Core/HumanoSimulado.cs): lê a bola com erro,
// reage com atraso, aperta o botão com erro de timing. Serve pro modo demonstração, pra testes e pra calibrar.
#pragma once

#include "PadelSim/Aleatorio.h"
#include "PadelSim/Arbitro.h"
#include "PadelSim/Bola.h"
#include "PadelSim/Golpe.h"
#include "PadelSim/Jogadores.h"

#include <array>
#include <optional>
#include <string>

namespace padel
{
class Partida;
enum class EstadoDaPartida : int32_t;

struct PerfilDeHumano
{
    std::string Nome;
    float TempoDeReacao, DesvioDoTempo, ErroDeLeitura, Agressividade;

    static const PerfilDeHumano Iniciante, Intermediario, Avancado, Profissional;
};

struct JogadorVisivel
{
    float X, Y, Vx, Vy;
    int32_t Time, Lado;
};

/// Só o que um jogador VÊ da partida.
struct EstadoVisivel
{
    EstadoDaPartida Estado;
    int32_t IndiceDoSacador;
    padel::Caixa CaixaDoSaque;
    float BolaX, BolaY, BolaZ, BolaVx, BolaVy, BolaVz, BolaWx, BolaWy, BolaWz;
    bool BolaEmJogo;
    int32_t TimeDoUltimoGolpe;
    std::optional<TipoDeGolpe> TipoDoUltimoGolpe;
    bool QuicouDepoisDoUltimoGolpe;
    std::array<JogadorVisivel, 4> Jogadores;

    static EstadoVisivel De(const Partida& p);
    Bola CopiaDaBola() const;
};

class HumanoSimulado
{
public:
    HumanoSimulado(int32_t indiceDoJogador, PerfilDeHumano perfil, padel::Aleatorio& aleatorio, bool destro = true);

    PerfilDeHumano Perfil;
    int32_t Indice() const { return indice; }
    /// A entrada deste tick, no referencial do jogador (Dy < 0 = rumo à rede).
    Entrada Decidir(const EstadoVisivel& estado, float dt);

private:
    enum class PlanoDeGolpe { Normal, Ataque, Defesa, Lob, Chiquita, Remate, RemateForte, Bandeja, Vibora };
    struct Leitura
    {
        int32_t Time;
        std::optional<TipoDeGolpe> Tipo;
        bool Quicou;
        bool operator==(const Leitura& o) const { return Time == o.Time && Tipo == o.Tipo && Quicou == o.Quicou; }
        bool operator!=(const Leitura& o) const { return !(*this == o); }
    };
    struct Contato
    {
        float X, Y, T, Z;
    };

    int32_t indice;
    padel::Aleatorio* aleatorio;
    Jogador eu, corpoNoContato;
    Leitura ultimaLeitura{-2, std::nullopt, false};
    float desdeALeitura = 0, desdeOAperto = 0;
    float erroX = 0, erroY = 0, erroDoTempo = 0;
    bool apertouNestaBola = false, segurarAcao = false;
    PlanoDeGolpe plano = PlanoDeGolpe::Normal;
    float esperaDoSaque = -1;

    Entrada NoRally(const EstadoVisivel& e, float dt);
    bool AindaNaoEHoraDeApertar(float segundosAteOPonto) const;
    std::pair<float, float> IrAte(float x, float y, float distancia) const;
    std::optional<Contato> PreverContato(const EstadoVisivel& e) const;
    std::optional<float> TempoAteOPontoIdeal(const EstadoVisivel& e, float x, float y, float esperaPraReagir);
    bool SouQuemBusca(const EstadoVisivel& e, float x, float y) const;
    PlanoDeGolpe EscolherGolpe(const EstadoVisivel& e, float zContato);
    std::pair<float, float> Mira(const EstadoVisivel& e, PlanoDeGolpe p) const;
    Entrada Posicionar(const EstadoVisivel& e, bool atacando);
    Entrada ParaEntrada(float dxMundo, float dyMundo, bool apertou, bool segurando) const;
};
} // namespace padel
