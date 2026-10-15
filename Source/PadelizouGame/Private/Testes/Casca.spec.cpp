// A casca da partida (pedido 0002): só o que o Unreal faz por cima do PadelSim — passo fixo de 120 Hz, painéis da
// quadra em caixas do Unreal e leitura do controle. Regra de jogo não se testa aqui: vem do gabarito.
#include "Misc/AutomationTest.h"
#include "PadelControle.h"
#include "PadelPaineis.h"
#include "PadelSimulacao.h"
#include "PadelSim/Quadra.h"

BEGIN_DEFINE_SPEC(FCascaSpec, "PadelizouGame.Casca",
                  EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FCascaSpec)

void FCascaSpec::Define()
{
    Describe("Simulação (passo fixo de 120 Hz)", [this]()
    {
        It("60 quadros de 1/60 s dão exatamente 120 passos", [this]()
        {
            FPadelSimulacao Sim{padel::OpcoesDaPartida{}};
            int Passos = 0;
            for (int I = 0; I < 60; ++I) Passos += Sim.Quadro(1.0 / 60.0, padel::Entrada{});
            TestEqual("passos", Passos, 120);
            TestEqual("passos totais", Sim.PassosTotais(), 120);
            TestEqual("tempo de jogo", Sim.Partida().TempoDeJogo, 1.0f, 1e-3f);
        });
        It("um quadro lento de 0,5 s roda 60 passos de uma vez, não um passo de 0,5 s", [this]()
        {
            FPadelSimulacao Sim{padel::OpcoesDaPartida{}};
            TestEqual("passos", Sim.Quadro(0.5, padel::Entrada{}), 60);
            TestEqual("tempo de jogo", Sim.Partida().TempoDeJogo, 0.5f, 1e-3f);
        });
        It("quadros irregulares não deixam deriva: 1 s no relógio são 120 passos", [this]()
        {
            FPadelSimulacao Sim{padel::OpcoesDaPartida{}};
            const double Deltas[] = {0.0071, 0.0163, 0.0095, 0.0333, 0.0021, 0.0117};   // soma 0,0800
            double Soma = 0;
            int Passos = 0;
            while (Soma < 1.0 - 1e-9)
            {
                for (double D : Deltas)
                {
                    const double Usado = FMath::Min(D, 1.0 - Soma);
                    Passos += Sim.Quadro(Usado, padel::Entrada{});
                    Soma += Usado;
                    if (Soma >= 1.0 - 1e-9) break;
                }
            }
            TestEqual("passos em 1 s", Passos, 120);
        });
        It("uma partida inteira de IA contra IA (formato curto) termina, e com a mesma semente dá o mesmo placar", [this]()
        {
            auto Jogar = [](int& PassosOut)
            {
                padel::OpcoesDaPartida Op;
                Op.Humanos = padel::OpcoesDaPartida::NinguemHumano;
                Op.GamesPorSet = 4;
                Op.PontosDoTieBreak = 10;
                Op.Semente = 12345u;
                FPadelSimulacao Sim{Op};
                for (int Quadro = 0; Quadro < 60 * 3600 && !Sim.Partida().Acabou(); ++Quadro)
                {
                    Sim.Quadro(1.0 / 60.0, padel::Entrada{});
                }
                PassosOut = Sim.PassosTotais();
                return std::make_pair(Sim.Partida().Acabou(), Sim.Partida().Placar.Resumo());
            };
            int PassosA = 0, PassosB = 0;
            const auto A = Jogar(PassosA);
            const auto B = Jogar(PassosB);
            TestTrue("acabou em menos de uma hora de jogo", A.first);
            TestTrue("tem resumo", !A.second.empty());
            TestEqual("mesmo placar", FString(UTF8_TO_TCHAR(A.second.c_str())), FString(UTF8_TO_TCHAR(B.second.c_str())));
            TestEqual("mesmos passos", PassosA, PassosB);
        });
    });

    Describe("Painéis da quadra", [this]()
    {
        It("cada painel de quadra::Paineis() vira uma caixa do Unreal colada à parede certa, por fora", [this]()
        {
            const auto& Paineis = padel::quadra::Paineis();
            TestTrue("há painéis", Paineis.size() > 0);
            for (const padel::quadra::Painel& P : Paineis)
            {
                const FPadelCaixa C = PadelPaineis::CaixaDoPainel(P);
                TestTrue("extensões positivas", C.MeiaExtensao.X > 0 && C.MeiaExtensao.Y > 0 && C.MeiaExtensao.Z > 0);
                if (P.Parede == padel::QualParede::Lateral)
                {
                    // x = ±5 m no PadelSim é Y = ∓500 cm no Unreal (Y = -x); a caixa fica de fora (|Y| > 500).
                    TestTrue("lateral por fora", FMath::Abs(C.Centro.Y) > 500.0 && FMath::Sign(C.Centro.Y) == -P.Sinal);
                    TestEqual("começa em Inicio (X do Unreal = y do PadelSim)", C.Centro.X - C.MeiaExtensao.X, P.Inicio * 100.0, 1e-3);
                }
                else
                {
                    // y = ±10 m no PadelSim é X = ±1000 cm no Unreal.
                    TestTrue("fundo por fora", FMath::Abs(C.Centro.X) > 1000.0 && FMath::Sign(C.Centro.X) == P.Sinal);
                    TestEqual("começa em Inicio (Y do Unreal = -x do PadelSim)", C.Centro.Y + C.MeiaExtensao.Y, -P.Inicio * 100.0, 1e-3);
                }
                TestEqual("base em ZMin", C.Centro.Z - C.MeiaExtensao.Z, P.ZMin * 100.0, 1e-3);
                TestEqual("topo em ZMax", C.Centro.Z + C.MeiaExtensao.Z, P.ZMax * 100.0, 1e-3);
            }
        });
    });

    Describe("Controle (quatro botões)", [this]()
    {
        It("W é rumo à rede (Dy < 0) e D é a direita dele (Dx > 0)", [this]()
        {
            FLeituraDoControle L;
            L.Mover = FVector2D(1.0, 1.0);   // D e W
            const padel::Entrada E = PadelControle::MontarEntrada(L);
            TestTrue("Dx > 0", E.Dx > 0);
            TestTrue("Dy < 0", E.Dy < 0);
            TestTrue("diagonal não passa de 1", FMath::Square(E.Dx) + FMath::Square(E.Dy) <= 1.0001f);
        });
        It("A, B, X, Y e RB viram os campos certos da Entrada", [this]()
        {
            FLeituraDoControle L;
            L.AcaoApertou = true; L.AcaoSegurada = true;
            L.LobApertou = true;
            L.CortadoApertou = true;
            L.AtaqueApertou = true; L.AtaqueSegurado = true;
            L.ModificadorSegurado = true;
            const padel::Entrada E = PadelControle::MontarEntrada(L);
            TestTrue("A apertou", E.AcaoPressionada);
            TestTrue("A segurada", E.AcaoSegurada);
            TestTrue("B apertou", E.LobPressionada);
            TestTrue("X apertou", E.CortadoPressionado);
            TestTrue("Y apertou", E.AtaquePressionado);
            TestTrue("Y segurada", E.AtaqueSegurado);
            TestTrue("RB segurado", E.ModificadorSegurado);
        });
        It("solto não manda nada", [this]()
        {
            const padel::Entrada E = PadelControle::MontarEntrada(FLeituraDoControle{});
            TestEqual("Dx", E.Dx, 0.f);
            TestEqual("Dy", E.Dy, 0.f);
            TestFalse("sem botão", E.AcaoPressionada || E.AcaoSegurada || E.LobPressionada || E.CortadoPressionado || E.AtaquePressionado || E.AtaqueSegurado || E.ModificadorSegurado);
        });
    });
}
