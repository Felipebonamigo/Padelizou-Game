#include "PadelSim/Arbitro.h"

namespace padel
{
Caixa CaixaDeSaque(int32_t lado, bool direita)
{
    int32_t sinalX = direita ? lado : -lado;
    return Caixa{sinalX > 0 ? 0.f : -quadra::MeiaLargura, sinalX > 0 ? quadra::MeiaLargura : 0.f,
                 lado > 0 ? 0.f : -quadra::LinhaDeSaque, lado > 0 ? quadra::LinhaDeSaque : 0.f};
}

std::optional<int32_t> Arbitro::Receptor() const
{
    if (Golpeador) return 1 - *Golpeador;
    return std::nullopt;
}

void Arbitro::ZerarRally()
{
    EmSaque = false; Golpeador.reset(); QuicouNoReceptor = false; TocouARede = false; Cruzou = false;
}

void Arbitro::NovoPonto() { Faltas = 0; ZerarRally(); }

void Arbitro::IniciarSaque(int32_t time, Caixa c)
{
    ZerarRally();
    EmSaque = true; Golpeador = time; caixa = c;
}

void Arbitro::RegistrarGolpe(int32_t time)
{
    ZerarRally();
    Golpeador = time;
}

bool Arbitro::PodeGolpear(int32_t time) const
{
    if (!Golpeador || *Golpeador == time) return false;
    if (EmSaque && !QuicouNoReceptor) return false;
    return true;
}

std::optional<Decisao> Arbitro::Processar(const EventoDaBola& evento)
{
    if (!Golpeador) return std::nullopt;
    int32_t golpeador = *Golpeador;
    int32_t ladoDoGolpeador = quadra::LadoDoTime(golpeador);
    switch (evento.Tipo)
    {
    case TipoDeEventoDaBola::CruzouRede:
        if (evento.Lado != ladoDoGolpeador) { Cruzou = true; return std::nullopt; }   // em CruzouRede, Lado é pra onde foi
        if (QuicouNoReceptor) return Decisao::Ponto(golpeador, Motivo::VoltouPeloVidro);
        return std::nullopt;
    case TipoDeEventoDaBola::Rede:
        TocouARede = true;
        return std::nullopt;
    case TipoDeEventoDaBola::Quique:
        if (evento.Lado == ladoDoGolpeador) return ContraOGolpeador(TocouARede ? Motivo::Rede : Motivo::NaoPassou);
        if (QuicouNoReceptor) return Decisao::Ponto(golpeador, Motivo::DoisQuiques);
        QuicouNoReceptor = true;
        if (EmSaque)
        {
            if (!caixa.Contem(evento.X, evento.Y)) return ContraOGolpeador(Motivo::ForaDaCaixa);
            if (TocouARede) return Decisao::Let();
        }
        return std::nullopt;
    case TipoDeEventoDaBola::Parede:
        if (evento.Lado == ladoDoGolpeador)
        {
            if (EmSaque) return ContraOGolpeador(Motivo::ParedePropria);
            return std::nullopt;
        }
        if (QuicouNoReceptor) return std::nullopt;
        return ContraOGolpeador(Motivo::ParedeSemQuicar);
    case TipoDeEventoDaBola::Saiu:
        return QuicouNoReceptor ? Decisao::Ponto(golpeador, Motivo::Fora) : ContraOGolpeador(Motivo::Fora);
    }
    return std::nullopt;
}

Decisao Arbitro::ContraOGolpeador(Motivo motivo)
{
    int32_t receptor = *Receptor();
    if (!EmSaque) return Decisao::Ponto(receptor, motivo);
    Faltas += 1;
    return Faltas >= 2 ? Decisao::Ponto(receptor, Motivo::DuplaFalta) : Decisao::Falta(motivo);
}
} // namespace padel
