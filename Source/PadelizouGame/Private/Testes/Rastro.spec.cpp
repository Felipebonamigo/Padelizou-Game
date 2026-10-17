// Rastro-fantasma da saída de parede e cor no pé do jogador (pilar 1 e controle do DESIGN.md). Só a escolha do que
// mostrar; a previsão em si é padel::rastro_fantasma::Prever, que já tem gabarito.
#include "Misc/AutomationTest.h"
#include "PadelRastro.h"
#include "PadelSim/Bola.h"
#include "PadelSim/RastroFantasma.h"

namespace
{
padel::Rastro RastroDeTeste(int Pontos, int PrimeiroDepoisDaParede)
{
    padel::Rastro R;
    R.Pontos.resize(Pontos);
    R.PrimeiroDepoisDaParede = PrimeiroDepoisDaParede;
    return R;
}
} // namespace

BEGIN_DEFINE_SPEC(FRastroSpec, "PadelizouGame.Rastro",
                  EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FRastroSpec)

void FRastroSpec::Define()
{
    Describe("Que trecho mostrar, por nível de assistência", [this]()
    {
        It("Pro: nunca", [this]()
        {
            TestFalse("sem trecho", PadelRastro::Escolher(RastroDeTeste(50, 10), padel::Assistencia::Pro, true).has_value());
        });
        It("Casual: sempre, da primeira batida no vidro (já quicada) até o fim", [this]()
        {
            const auto T = PadelRastro::Escolher(RastroDeTeste(50, 10), padel::Assistencia::Casual, false);
            TestTrue("há trecho", T.has_value());
            if (T)
            {
                TestEqual("começa depois da parede", T->Inicio, 10);
                TestEqual("vai até o fim", T->Fim, 50);
            }
        });
        It("Casual: se a saída de parede não vem pela frente, nada", [this]()
        {
            TestFalse("sem trecho", PadelRastro::Escolher(RastroDeTeste(50, -1), padel::Assistencia::Casual, false).has_value());
        });
        It("Padrão: só o rastro pedido no passo da batida no vidro, e ele inteiro", [this]()
        {
            const auto T = PadelRastro::Escolher(RastroDeTeste(50, 10), padel::Assistencia::Padrao, true);
            TestTrue("há trecho na batida", T.has_value());
            if (T)
            {
                TestEqual("começa no início", T->Inicio, 0);
                TestEqual("vai até o fim", T->Fim, 50);
            }
            TestFalse("sem trecho antes da batida", PadelRastro::Escolher(RastroDeTeste(50, 10), padel::Assistencia::Padrao, false).has_value());
        });
        It("rastro vazio não vira trecho", [this]()
        {
            TestFalse("Casual", PadelRastro::Escolher(RastroDeTeste(0, 0), padel::Assistencia::Casual, false).has_value());
            TestFalse("Padrão", PadelRastro::Escolher(RastroDeTeste(0, 0), padel::Assistencia::Padrao, true).has_value());
        });
    });

    Describe("Com a física de verdade", [this]()
    {
        It("uma bola que cruza a rede, quica e bate no vidro de fundo da casa dá um trecho Casual depois da batida", [this]()
        {
            padel::Bola B;
            B.Reiniciar();
            B.Posicionar(0.f, -3.f, 1.4f);
            padel::Velocidade V;
            V.Vy = 16.f;
            V.Vz = 1.5f;
            B.Lancar(V);
            const padel::Rastro R = padel::rastro_fantasma::Prever(B, 1, false);
            AddInfo(FString::Printf(TEXT("pontos %d, primeiro depois da parede %d"), static_cast<int>(R.Pontos.size()), R.PrimeiroDepoisDaParede));
            TestTrue("o rastro tem pontos", R.Pontos.size() > 0);
            const auto T = PadelRastro::Escolher(R, padel::Assistencia::Casual, false);
            TestTrue("tem trecho Casual", T.has_value());
            if (T)
            {
                TestTrue("o trecho começa depois do início", T->Inicio > 0);
                TestTrue("e cabe no rastro", T->Fim <= static_cast<int>(R.Pontos.size()) && T->Inicio < T->Fim);
            }
        });
    });

    Describe("Cor no pé do jogador (erro do golpe)", [this]()
    {
        It("erro perfeito é verde, médio amarelo, péssimo vermelho", [this]()
        {
            const FLinearColor Verde = PadelRastro::CorDoErro(0.f), Amarelo = PadelRastro::CorDoErro(0.7f), Vermelho = PadelRastro::CorDoErro(1.6f);
            TestTrue("verde domina o G", Verde.G > Verde.R && Verde.G > Verde.B);
            TestTrue("amarelo tem R e G altos e B baixo", Amarelo.R > 0.7f && Amarelo.G > 0.7f && Amarelo.B < 0.3f);
            TestTrue("vermelho domina o R", Vermelho.R > Vermelho.G && Vermelho.R > Vermelho.B);
        });
        It("a cor só piora com o erro (G cai e R sobe passando do verde ao vermelho)", [this]()
        {
            float RAnterior = -1.f;
            for (float E = 0.f; E <= 1.6f; E += 0.1f)
            {
                const FLinearColor C = PadelRastro::CorDoErro(E);
                TestTrue("R não desce", C.R >= RAnterior - 1e-4f);
                RAnterior = C.R;
            }
        });
    });
}
