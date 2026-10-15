#include "PadelIndicador.h"

#include "PadelSim/Quadra.h"

#include <vector>

namespace FPadelIndicador
{
std::optional<FPadelPrevisaoDeContato> Prever(const padel::Partida& Partida, int Jogador)
{
    if (Partida.Estado != padel::EstadoDaPartida::Rally || !Partida.Bola.EmJogo) return std::nullopt;
    const padel::Jogador& J = Partida.Jogadores[Jogador];
    if (!J.Humano || J.Cooldown > 0.f || !Partida.Arbitro.PodeGolpear(J.Time)) return std::nullopt;

    padel::Jogador Corpo = J;   // cópia: parado onde está
    Corpo.Vx = 0.f;
    Corpo.Vy = 0.f;
    padel::Bola Bola = Partida.Bola;
    bool bJaQuicou = Partida.Arbitro.QuicouNoReceptor;

    constexpr float Passo = 1.f / 120.f;
    constexpr int PassosMaximos = 240;   // 2 s adiante
    std::vector<padel::EventoDaBola> Eventos;
    for (int I = 1; I <= PassosMaximos; ++I)
    {
        // Na Partida, o contato é testado com a bola ANTES do passo; aqui o passo I testa a bola depois de I-1 passos.
        if (Corpo.Alcanca(Bola) && Corpo.PodeBaterAgora(Bola, Passo, bJaQuicou))
        {
            FPadelPrevisaoDeContato P;
            P.SegundosParaOContato = (I - 1) * Passo;
            P.SegundosParaApertar = P.SegundosParaOContato - padel::Jogador::MomentoIdealDoBalanco;
            P.X = Bola.X;
            P.Y = Bola.Y;
            return P;
        }
        Eventos.clear();
        Bola.Avancar(Passo, Eventos);
        for (const padel::EventoDaBola& E : Eventos)
        {
            if (E.Tipo == padel::TipoDeEventoDaBola::Quique && padel::quadra::LadoDe(E.Y) == J.Lado) bJaQuicou = true;
            if (E.Tipo == padel::TipoDeEventoDaBola::Saiu || E.Tipo == padel::TipoDeEventoDaBola::Rede) return std::nullopt;
        }
        if (!Bola.EmJogo) return std::nullopt;
    }
    return std::nullopt;
}
} // namespace FPadelIndicador
