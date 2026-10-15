// Fumaça do eixo e da escala: a bola do PadelSim, lançada de (0, 9, 1) rumo à rede a 60 quadros por segundo, tem de
// aparecer no Unreal no lugar certo (cm, X = comprimento da quadra, Y = -largura, Z = altura).
#include "Misc/AutomationTest.h"
#include "Coordenadas.h"
#include "PadelSim/Bola.h"

#include <vector>

BEGIN_DEFINE_SPEC(FFumacaSpec, "PadelizouGame.Fumaca",
                  EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FFumacaSpec)

void FFumacaSpec::Define()
{
    Describe("Coordenadas", [this]()
    {
        It("metros do PadelSim viram centímetros do Unreal, com y vindo para X e -x para Y", [this]()
        {
            const FVector P = PadelCoordenadas::ParaUnreal(2.f, 9.f, 1.f);
            TestEqual("X do Unreal = y do PadelSim em cm", P.X, 900.0);
            TestEqual("Y do Unreal = -x do PadelSim em cm (a direita de quem olha da casa é -Y)", P.Y, -200.0);
            TestEqual("Z do Unreal = z do PadelSim em cm", P.Z, 100.0);
        });
        It("volta ao PadelSim sem perder nada", [this]()
        {
            const FVector P = PadelCoordenadas::ParaUnreal(-3.5f, 4.25f, 0.5f);
            float X, Y, Z;
            PadelCoordenadas::ParaPadel(P, X, Y, Z);
            TestEqual("x", X, -3.5f);
            TestEqual("y", Y, 4.25f);
            TestEqual("z", Z, 0.5f);
        });
    });

    Describe("a bola a 60 quadros por segundo", [this]()
    {
        It("sai de (0, 9, 1) rumo à rede: X do Unreal só diminui e a bola cruza a rede", [this]()
        {
            padel::Bola Bola;
            Bola.Reiniciar();
            Bola.Posicionar(0.f, 9.f, 1.f);
            padel::Velocidade V;
            V.Vy = -10.f;
            V.Vz = 5.f;
            Bola.Lancar(V);

            const FVector Inicio = PadelCoordenadas::ParaUnreal(Bola.X, Bola.Y, Bola.Z);
            TestEqual("início em X", Inicio.X, 900.0);
            TestEqual("início em Y", Inicio.Y, 0.0);
            TestEqual("início em Z", Inicio.Z, 100.0);

            bool Cruzou = false;
            double XAnterior = Inicio.X;
            std::vector<padel::EventoDaBola> Eventos;
            for (int Quadro = 0; Quadro < 60 && !Cruzou; ++Quadro)
            {
                Eventos.clear();
                Bola.Avancar(1.f / 60.f, Eventos);
                const FVector P = PadelCoordenadas::ParaUnreal(Bola.X, Bola.Y, Bola.Z);
                TestTrue(*FString::Printf(TEXT("quadro %d: X diminui"), Quadro), P.X < XAnterior);
                XAnterior = P.X;
                for (const padel::EventoDaBola& E : Eventos)
                {
                    if (E.Tipo == padel::TipoDeEventoDaBola::CruzouRede) Cruzou = true;
                }
            }
            TestTrue("cruzou a rede em menos de 1 s", Cruzou);
        });
    });
}
