// Física da bola em 3D (x, y no chão; z pra cima), metros, segundos, quilos, rad/s: gravidade, arrasto quadrático,
// Magnus, quique com transferência de spin, vidro diferente de grade, rede, saída por cima da parede e pela porta.
//
// DETERMINISMO: só float (IEEE 754 single), soma, multiplicação, divisão e raiz — sem sin/cos/random, sem FMA, sem
// fast-math. É o que faz o resultado sair igual ao do Padel.Core e igual em qualquer máquina (host e cliente online).
#pragma once

#include "PadelSim/Exportar.h"

#include "PadelSim/Tipos.h"
#include <cstdint>
#include <vector>

namespace padel
{
class PADELSIM_API Bola
{
public:
    // Bola de padel (regra FIP): 56–59,4 g; 6,35–6,77 cm de diâmetro; quique de 135–145 cm ao cair de 2,54 m.
    static constexpr float Massa = 0.057f;
    static constexpr float Raio = 0.0335f;
    static constexpr float G = 9.81f;
    static constexpr float MomentoDeInercia = 2.f / 3.f;   // casca esférica: I = (2/3) m R²
    static constexpr float RestituicaoDoChao = 0.775f;
    static constexpr float AtritoDoChao = 0.6f;
    static constexpr float RestituicaoDoVidro = 0.85f;
    static constexpr float AtritoDoVidro = 0.25f;
    static constexpr float RestituicaoDaGrade = 0.4f;
    static constexpr float AtritoDaGrade = 0.8f;
    static constexpr float VariacaoDaRestituicaoDaGrade = 0.1f;
    static constexpr float SenoDoDesvioMaximoDaGrade = 0.25881904f;   // sen 15°, escrito direto (sem libm)
    static constexpr float RestituicaoDaRede = 0.2f;
    static constexpr float PassoMaximo = 1.f / 240.f;
    static constexpr float VzMinimoParaQuicar = 0.7f;

    /// Desaceleração por arrasto = KArrasto · v²; Magnus = KMagnus · Cl · v · (ω̂ × v⃗).
    static float KArrasto();
    static float KMagnus();

    float X = 0, Y = 0, Z = 1;
    float Vx = 0, Vy = 0, Vz = 0;
    /// Spin em rad/s. Topspin de uma bola indo em +y é −x; sidespin é o eixo z.
    float Wx = 0, Wy = 0, Wz = 0;
    bool EmJogo = false;
    bool Rolando = false;
    bool Parada = false;

    void Reiniciar();
    void Posicionar(float x, float y, float z);
    void Lancar(const Velocidade& v);

    float Rapidez() const;
    float Spin() const;

    /// Avança dt segundos em sub-passos de até PassoMaximo, acumulando os eventos do intervalo.
    void Avancar(float dt, std::vector<EventoDaBola>& eventos);

    /// Quantos sub-passos de integração as bolas deram NESTA thread até agora (qualquer bola). Os solucionadores dos
    /// golpes especiais têm orçamento contado por ele — então ele decide resultado, não é só diagnóstico.
    static int64_t SubPassosNestaThread();

private:
    void Passo(float h, std::vector<EventoDaBola>& eventos);
    void RebaterNaParede(Superficie superficie, float nx, float ny, float cx, float cy, float cz);
    void RebaterNaGrade(float nx, float ny, float cx, float cy, float cz);
    bool Rebater(float nx, float ny, float nz, float restituicao, float atrito);
    void Sair(std::vector<EventoDaBola>& eventos, float x, float y, float z, QualParede parede, bool pelaPorta);
};
} // namespace padel
