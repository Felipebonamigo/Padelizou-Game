// O indicador de "quando apertar" (Manual): prevê, com a própria física do PadelSim numa cópia da bola, em quantos
// segundos o balanço tem de começar para o contato sair no ponto ideal. Nenhuma regra nova: só leitura da Partida.
#include "Misc/AutomationTest.h"
#include "PadelIndicador.h"
#include "PadelSimulacao.h"
#include "PadelSim/HumanoSimulado.h"

namespace
{
struct FContagem
{
    int Balancos = 0, Erros = 0, Devolucoes = 0;
};

// Joga `Segundos` de tempo de jogo no Manual de quatro botões (assistência Casual). O jogador 0 anda como o
// HumanoSimulado, mas os botões do rally só saem se `bSeguirIndicador` e o indicador mandar apertar agora.
FContagem Jogar(bool bSeguirIndicador, double Segundos)
{
    padel::OpcoesDaPartida Op;
    Op.GamesPorSet = 4;
    Op.PontosDoTieBreak = 10;
    Op.ModoDeGolpe = padel::ModoDeGolpe::Manual;
    Op.Controle = padel::Controle::QuatroBotoes;
    Op.Assistencia = padel::Assistencia::Casual;
    Op.Semente = 42u;
    FPadelSimulacao Sim{Op};
    padel::Aleatorio Aleatorio{49u};
    padel::HumanoSimulado Humano(0, padel::PerfilDeHumano::Avancado, Aleatorio, true, Op.Controle);

    FContagem C;
    Sim.Partida().AoEvento = [&C](const padel::EventoDaPartida& E)
    {
        if (E.Jogador != 0) return;
        if (E.Tipo == padel::TipoDeEventoDaPartida::Balanco) ++C.Balancos;
        if (E.Tipo == padel::TipoDeEventoDaPartida::Errou) ++C.Erros;
        if (E.Tipo == padel::TipoDeEventoDaPartida::Golpe && E.Golpe && *E.Golpe != padel::TipoDeGolpe::Saque) ++C.Devolucoes;
    };

    constexpr double Passo = 1.0 / FPadelSimulacao::PassosPorSegundo;
    for (double T = 0; T < Segundos && !Sim.Partida().Acabou(); T += Passo)
    {
        padel::Entrada E = Humano.Decidir(padel::EstadoVisivel::De(Sim.Partida()), static_cast<float>(Passo));
        if (Sim.Partida().Estado == padel::EstadoDaPartida::Rally)
        {
            E.AcaoPressionada = E.AcaoSegurada = E.LobPressionada = E.CortadoPressionado = false;
            E.AtaquePressionado = E.AtaqueSegurado = E.ModificadorSegurado = false;
            if (bSeguirIndicador && !Sim.Partida().Jogadores[0].Balancando())
            {
                const auto P = FPadelIndicador::Prever(Sim.Partida(), 0);
                E.AcaoPressionada = P && P->SegundosParaApertar <= Passo * 0.5;
            }
        }
        Sim.Quadro(Passo, E);
    }
    return C;
}
} // namespace

BEGIN_DEFINE_SPEC(FIndicadorSpec, "PadelizouGame.Indicador",
                  EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FIndicadorSpec)

void FIndicadorSpec::Define()
{
    It("sem bola vindo (início do saque) não há previsão", [this]()
    {
        padel::OpcoesDaPartida Op;
        Op.Semente = 1u;
        padel::Partida P(Op);
        TestFalse("sem previsão", FPadelIndicador::Prever(P, 0).has_value());
    });

    It("sem apertar nada, o jogador não devolve nenhuma bola (o teste enxerga a diferença)", [this]()
    {
        const FContagem C = Jogar(false, 240.0);
        TestEqual("devoluções", C.Devolucoes, 0);
        TestEqual("balanços", C.Balancos, 0);
    });

    It("apertando só quando o indicador manda, o jogador bate quase todos os balanços", [this]()
    {
        const FContagem C = Jogar(true, 240.0);
        AddInfo(FString::Printf(TEXT("balanços %d, devoluções %d, erros %d"), C.Balancos, C.Devolucoes, C.Erros));
        TestTrue("fez balanços (o indicador manda apertar a tempo de cada bola)", C.Balancos >= 35);
        TestTrue("devolveu bolas", C.Devolucoes >= 35);
        TestTrue("acerta pelo menos 70% dos balanços", C.Erros * 10 <= C.Balancos * 3);
    });
}
