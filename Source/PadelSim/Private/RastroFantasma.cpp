#include "PadelSim/RastroFantasma.h"

namespace padel::rastro_fantasma
{
Rastro Prever(const Bola& bola, int32_t lado, bool jaQuicou)
{
    Rastro rastro;
    if (!bola.EmJogo || bola.Parada || bola.Rolando) return rastro;
    Bola b = bola;
    std::vector<EventoDaBola> eventos;
    eventos.reserve(4);
    int32_t quiques = jaQuicou ? 1 : 0;
    float t = 0;
    for (int32_t i = 0; i < PassosMaximos; i++)
    {
        eventos.clear();
        b.Avancar(Passo, eventos);
        t += Passo;
        rastro.Pontos.push_back({b.X, b.Y, b.Z, t});
        bool acabou = false;
        for (const auto& e : eventos)
        {
            switch (e.Tipo)
            {
            case TipoDeEventoDaBola::Quique:
                if (e.Lado != lado || ++quiques >= 2) acabou = true;
                break;
            case TipoDeEventoDaBola::Parede:
                if (e.Lado != lado || quiques == 0) acabou = true;
                else if (rastro.PrimeiroDepoisDaParede < 0) rastro.PrimeiroDepoisDaParede = i;
                break;
            case TipoDeEventoDaBola::CruzouRede:
                if (e.Lado != lado) acabou = true;   // em CruzouRede, Lado é pra que lado a bola foi
                break;
            case TipoDeEventoDaBola::Saiu:
                acabou = true;
                break;
            case TipoDeEventoDaBola::Rede:
                break;
            }
            if (acabou) break;   // o que vem depois no mesmo passo já é de bola morta (o vidro depois do 2º quique)
        }
        if (acabou || !b.EmJogo || b.Parada || b.Rolando) break;
    }
    return rastro;
}
} // namespace padel::rastro_fantasma
