// Transforma os eventos da física em decisão de ponto, pelas regras do padel (Padel.Core/Arbitro.cs).
#pragma once

#include "PadelSim/Quadra.h"
#include "PadelSim/Tipos.h"

#include <optional>

namespace padel
{
enum class Motivo : int32_t { DoisQuiques, Rede, NaoPassou, ParedeSemQuicar, ParedePropria, Fora, VoltouPeloVidro, DuplaFalta, ForaDaCaixa, BolaMorta };
enum class TipoDeDecisao : int32_t { Ponto, Falta, Let };

struct Decisao
{
    TipoDeDecisao Tipo;
    int32_t Para;
    padel::Motivo Motivo;   // tipo qualificado: o membro tem o mesmo nome
    static Decisao Ponto(int32_t para, padel::Motivo motivo) { return {TipoDeDecisao::Ponto, para, motivo}; }
    static Decisao Falta(padel::Motivo motivo) { return {TipoDeDecisao::Falta, -1, motivo}; }
    static Decisao Let() { return {TipoDeDecisao::Let, -1, padel::Motivo::Rede}; }
};

struct Caixa
{
    float XMin = 0, XMax = 0, YMin = 0, YMax = 0;
    bool Contem(float x, float y) const { return x >= XMin && x <= XMax && y >= YMin && y <= YMax; }
    float CentroX() const { return (XMin + XMax) / 2; }
    float CentroY() const { return (YMin + YMax) / 2; }
};

/// Caixa de saque no lado dado, à direita ou à esquerda de quem está naquele lado (da rede até a linha de saque).
Caixa CaixaDeSaque(int32_t lado, bool direita);

/// A bola precisa cruzar a rede e quicar no chão do outro lado ANTES de tocar qualquer parede de lá; quicou do lado de
/// quem bateu é ponto contra; depois de quicar, paredes à vontade e o segundo quique encerra; saque na caixa diagonal,
/// duas faltas é ponto, let se tocou a rede e caiu na caixa; o mesmo time não bate duas vezes seguidas.
class Arbitro
{
public:
    int32_t Faltas = 0;
    bool EmSaque = false;
    /// Time que bateu por último; vazio antes do saque.
    std::optional<int32_t> Golpeador;
    bool QuicouNoReceptor = false;
    bool TocouARede = false;
    bool Cruzou = false;

    std::optional<int32_t> Receptor() const;
    void NovoPonto();
    void IniciarSaque(int32_t time, Caixa caixa);
    void RegistrarGolpe(int32_t time);
    bool PodeGolpear(int32_t time) const;
    /// Vazio = segue o jogo.
    std::optional<Decisao> Processar(const EventoDaBola& evento);

private:
    Caixa caixa{};
    void ZerarRally();
    Decisao ContraOGolpeador(Motivo motivo);
};
} // namespace padel
