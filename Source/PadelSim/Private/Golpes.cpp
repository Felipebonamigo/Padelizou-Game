// Porte de Golpes.Calcular (Padel.Core/Bola.cs).
#include "PadelSim/Golpes.h"

#include "PadelSim/Bola.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace padel::golpes
{
namespace
{
constexpr float RpmParaRadPorSegundo = 2 * 3.14159265358979323846f / 60.f;

Velocidade ChuteNoVacuo(float x0, float y0, float z0, float alvoX, float alvoY, float t, float distancia, float wx, float wy, float wz)
{
    float compensacao = 1 + Bola::KArrasto() * distancia * 0.5f;   // o arrasto quadrático encurta a bola
    float vx = (alvoX - x0) / t * compensacao;
    float vy = (alvoY - y0) / t * compensacao;
    float vz = (0.5f * Bola::G * t * t - z0) / t;
    return {vx, vy, vz, t, wx, wy, wz};
}
} // namespace

Velocidade Calcular(float x0, float y0, float z0, float alvoX, float alvoY, float tempoDeVoo, float /*folgaNaRede*/, bool ignorarRede, Efeito efeito)
{
    float dx = alvoX - x0, dy = alvoY - y0;
    float distancia = std::sqrt(dx * dx + dy * dy);
    float dirX = distancia > 1e-4f ? dx / distancia : 0, dirY = distancia > 1e-4f ? dy / distancia : 1;
    // Topspin de quem vai em d̂ é o eixo (up × d̂) = (−dy, dx, 0); sidespin é o eixo z.
    float wt = efeito.TopspinRpm * RpmParaRadPorSegundo, ws = efeito.SidespinRpm * RpmParaRadPorSegundo;
    float wx = -dirY * wt, wy = dirX * wt, wz = ws;

    float t = tempoDeVoo;
    Velocidade v = ChuteNoVacuo(x0, y0, z0, alvoX, alvoY, t, distancia, wx, wy, wz);
    if (ignorarRede) return v;

    Bola clone;
    std::vector<EventoDaBola> eventos;
    eventos.reserve(8);
    for (int i = 0; i < 12; i++)
    {
        clone.Posicionar(x0, y0, z0);
        clone.Lancar(v);
        eventos.clear();
        float tempoDeQueda = 0;
        bool bateuNaRede = false, caiu = false;
        float caiuX = 0, caiuY = 0;
        for (int passo = 0; passo < 4 * 60 && clone.EmJogo && !caiu && !bateuNaRede; passo++)
        {
            clone.Avancar(1.f / 60.f, eventos);
            tempoDeQueda += 1.f / 60.f;
            for (const auto& e : eventos)
            {
                if (e.Tipo == TipoDeEventoDaBola::Rede) { bateuNaRede = true; break; }
                if (e.Tipo == TipoDeEventoDaBola::Quique || e.Tipo == TipoDeEventoDaBola::Parede || e.Tipo == TipoDeEventoDaBola::Saiu)
                {
                    caiu = true; caiuX = e.X; caiuY = e.Y;
                    break;
                }
            }
            eventos.clear();
        }
        if (bateuNaRede)
        {
            // Arco mais alto: recomeça do chute no vácuo com mais tempo de voo.
            t *= 1.12f;
            v = ChuteNoVacuo(x0, y0, z0, alvoX, alvoY, t, distancia, wx, wy, wz);
            continue;
        }
        if (!caiu) break;
        float erroX = alvoX - caiuX, erroY = alvoY - caiuY;
        if (erroX * erroX + erroY * erroY < 0.12f * 0.12f) break;
        float tv = std::max(0.2f, tempoDeQueda);
        v.Vx = v.Vx + erroX / tv * 0.9f;
        v.Vy = v.Vy + erroY / tv * 0.9f;
    }
    return v;
}
} // namespace padel::golpes
