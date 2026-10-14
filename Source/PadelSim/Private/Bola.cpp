// Porte linha a linha de Padel.Core/Bola.cs. A ordem das operações é a mesma de propósito: em float, (a*b)*c e
// a*(b*c) dão resultados diferentes, e o gabarito confere até o último bit que importa.
#include "PadelSim/Bola.h"

#include "PadelSim/Quadra.h"

#include <algorithm>
#include <cmath>

namespace padel
{
namespace
{
constexpr float Pi = 3.14159265358979323846f;   // = MathF.PI
constexpr float DensidadeDoAr = 1.2f;
constexpr float CoeficienteDeArrasto = 0.55f;   // bola de feltro nesta faixa de velocidade
constexpr float DecaimentoDoSpinNoAr = 0.12f;   // fração por segundo
const float Area = Pi * Bola::Raio * Bola::Raio;

int32_t Sinal(float v) { return v > 0 ? 1 : v < 0 ? -1 : 0; }   // = MathF.Sign
float Limitar(float v, float minimo, float maximo) { return v < minimo ? minimo : v > maximo ? maximo : v; }

/// Que fração do sub-passo a bola andou até cruzar o plano |c| = limite, indo de antes (dentro) a depois (fora).
/// Se já estava fora no começo do passo (posicionada ali), o cruzamento é o ponto de partida.
float FracaoAtePlano(float antes, float depois, float limite)
{
    float a = std::fabs(antes), d = std::fabs(depois);
    return a >= limite ? 0 : (limite - a) / (d - a);
}

uint32_t Misturar(uint32_t h)
{
    h ^= h >> 16; h *= 0x85EBCA6Bu;
    h ^= h >> 13; h *= 0xC2B2AE35u;
    h ^= h >> 16;
    return h;
}

/// Hash do ponto de contato quantizado em centímetros (fmix32 do MurmurHash3). nearbyint arredonda pro par, como o
/// MathF.Round do C#.
uint32_t HashDoContato(float x, float y, float z)
{
    uint32_t qx = static_cast<uint32_t>(static_cast<int32_t>(std::nearbyint(x * 100)));
    uint32_t qy = static_cast<uint32_t>(static_cast<int32_t>(std::nearbyint(y * 100)));
    uint32_t qz = static_cast<uint32_t>(static_cast<int32_t>(std::nearbyint(z * 100)));
    return Misturar(Misturar(Misturar(qx * 0x9E3779B1u) ^ qy * 0x85EBCA77u) ^ qz * 0xC2B2AE3Du);
}

/// O próximo número do hash, em [0, 1) (24 bits, exatos em float).
thread_local int64_t subPassosNestaThread = 0;

float Uniforme(uint32_t& h)
{
    h = Misturar(h + 0x9E3779B9u);
    return static_cast<float>(h >> 8) / 16777216.f;
}
} // namespace

float Bola::KArrasto()
{
    static const float k = 0.5f * DensidadeDoAr * CoeficienteDeArrasto * Area / Massa;
    return k;
}

float Bola::KMagnus()
{
    static const float k = 0.5f * DensidadeDoAr * Area / Massa;
    return k;
}

void Bola::Reiniciar()
{
    X = 0; Y = 0; Z = 1;
    Vx = 0; Vy = 0; Vz = 0;
    Wx = 0; Wy = 0; Wz = 0;
    EmJogo = false; Rolando = false; Parada = false;
}

void Bola::Posicionar(float x, float y, float z)
{
    X = x; Y = y; Z = z;
    Vx = 0; Vy = 0; Vz = 0;
    Wx = 0; Wy = 0; Wz = 0;
    Rolando = false; Parada = false;
}

void Bola::Lancar(const Velocidade& v)
{
    Vx = v.Vx; Vy = v.Vy; Vz = v.Vz;
    Wx = v.Wx; Wy = v.Wy; Wz = v.Wz;
    EmJogo = true; Rolando = false; Parada = false;
}

float Bola::Rapidez() const { return std::sqrt(Vx * Vx + Vy * Vy + Vz * Vz); }
float Bola::Spin() const { return std::sqrt(Wx * Wx + Wy * Wy + Wz * Wz); }

int64_t Bola::SubPassosNestaThread() { return subPassosNestaThread; }

void Bola::Avancar(float dt, std::vector<EventoDaBola>& eventos)
{
    if (!EmJogo || Parada) return;
    float restante = dt;
    int32_t subPassos = 0;
    while (restante > 1e-7f && EmJogo && !Parada)
    {
        float h = std::min(PassoMaximo, restante);
        Passo(h, eventos);
        restante -= h;
        subPassos++;
    }
    subPassosNestaThread += subPassos;
}

void Bola::Passo(float h, std::vector<EventoDaBola>& eventos)
{
    float xAntes = X, yAntes = Y, zAntes = Z;
    if (Rolando)
    {
        float freio = std::max(0.f, 1 - 2.5f * h);
        Vx *= freio; Vy *= freio; Vz = 0; Z = 0;
        float freioDoSpin = std::max(0.f, 1 - 3.f * h);
        Wx *= freioDoSpin; Wy *= freioDoSpin; Wz *= freioDoSpin;
        if (std::sqrt(Vx * Vx + Vy * Vy) < 0.05f) { Vx = 0; Vy = 0; Parada = true; return; }
    }
    else
    {
        Vz -= G * h;
        float v = Rapidez();
        if (v > 1e-4f)
        {
            // Arrasto quadrático: a = −K·v·v⃗.
            float f = KArrasto() * v * h;
            Vx -= Vx * f; Vy -= Vy * f; Vz -= Vz * f;
            // Magnus: a = K·Cl·v·(ω̂ × v⃗), com Cl = 1 / (2 + v / (R·ω)) (Štěpánek).
            float w = Spin();
            if (w > 1.f)
            {
                float cl = 1.f / (2.f + v / (Raio * w));
                float cx = Wy * Vz - Wz * Vy, cy = Wz * Vx - Wx * Vz, cz = Wx * Vy - Wy * Vx;
                float k = KMagnus() * cl * v / w * h;
                Vx += cx * k; Vy += cy * k; Vz += cz * k;
            }
            float decaimento = 1 - DecaimentoDoSpinNoAr * h;
            Wx *= decaimento; Wy *= decaimento; Wz *= decaimento;
        }
    }
    X += Vx * h; Y += Vy * h; Z += Vz * h;

    // Rede: plano y = 0 até AlturaDaRede.
    if ((yAntes < 0 && Y >= 0) || (yAntes > 0 && Y <= 0))
    {
        float fracao = yAntes / (yAntes - Y);
        float zNaRede = zAntes + (Z - zAntes) * fracao;
        int32_t veioDe = yAntes > 0 ? 1 : -1;
        if (zNaRede < quadra::AlturaDaRede)
        {
            Y = veioDe * 0.03f;
            Vy = -Vy * RestituicaoDaRede;
            Vx *= 0.4f;
            Vz = std::min(Vz, 0.f) * 0.3f;
            Wx *= 0.3f; Wy *= 0.3f; Wz *= 0.3f;
            eventos.push_back({TipoDeEventoDaBola::Rede, X, Y, Z, veioDe});
        }
        else
        {
            eventos.push_back({TipoDeEventoDaBola::CruzouRede, X, Y, zNaRede, -veioDe});
        }
    }

    // Chão.
    if (Z <= 0 && !Rolando)
    {
        Z = 0;
        Rebater(0, 0, 1, RestituicaoDoChao, AtritoDoChao);
        eventos.push_back({TipoDeEventoDaBola::Quique, X, Y, 0, quadra::LadoDe(Y)});
        if (Vz < VzMinimoParaQuicar) { Vz = 0; Rolando = true; }
    }

    // Paredes: o trecho no ponto em que a bola cruzou o plano da parede decide — vidro devolve, grade mata e desvia,
    // aberto (porta, ou acima da altura daquele trecho) a bola sai. Lateral (|x| = 5).
    if (std::fabs(X) > quadra::MeiaLargura)
    {
        int32_t sinal = Sinal(X);
        float f = FracaoAtePlano(xAntes, X, quadra::MeiaLargura);
        float yNaParede = yAntes + (Y - yAntes) * f, zNaParede = zAntes + (Z - zAntes) * f;
        Superficie superficie = quadra::SuperficieDaParede(QualParede::Lateral, yNaParede, zNaParede);
        if (superficie == Superficie::Aberto)
        {
            Sair(eventos, sinal * quadra::MeiaLargura, yNaParede, zNaParede, QualParede::Lateral, quadra::NaPorta(yNaParede, zNaParede));
            return;
        }
        X = sinal * (quadra::MeiaLargura - 0.001f);
        RebaterNaParede(superficie, static_cast<float>(-sinal), 0, sinal * quadra::MeiaLargura, yNaParede, zNaParede);
        eventos.push_back({TipoDeEventoDaBola::Parede, X, Y, Z, quadra::LadoDe(Y), QualParede::Lateral, superficie});
    }
    // Fundo (|y| = 10).
    if (std::fabs(Y) > quadra::MeioComprimento)
    {
        int32_t sinal = Sinal(Y);
        float f = FracaoAtePlano(yAntes, Y, quadra::MeioComprimento);
        float xNaParede = xAntes + (X - xAntes) * f, zNaParede = zAntes + (Z - zAntes) * f;
        Superficie superficie = quadra::SuperficieDaParede(QualParede::Fundo, xNaParede, zNaParede);
        if (superficie == Superficie::Aberto)
        {
            Sair(eventos, xNaParede, sinal * quadra::MeioComprimento, zNaParede, QualParede::Fundo, false);
            return;
        }
        Y = sinal * (quadra::MeioComprimento - 0.001f);
        RebaterNaParede(superficie, 0, static_cast<float>(-sinal), xNaParede, sinal * quadra::MeioComprimento, zNaParede);
        eventos.push_back({TipoDeEventoDaBola::Parede, X, Y, Z, quadra::LadoDe(Y), QualParede::Fundo, superficie});
    }
}

void Bola::RebaterNaParede(Superficie superficie, float nx, float ny, float cx, float cy, float cz)
{
    if (superficie == Superficie::Grade) RebaterNaGrade(nx, ny, cx, cy, cz);
    else Rebater(nx, ny, 0, RestituicaoDoVidro, AtritoDoVidro);
}

/// A grade: restituição entre 0,30 e 0,50 e a direção de saída girada até ±15°, as duas tiradas de um hash do ponto
/// de contato. Mesmo ponto, mesmo rebote, em qualquer máquina. O giro nunca joga a bola pra dentro da grade: se ele
/// comeria mais da metade da velocidade de saída na normal (bola de raspão), gira pro outro lado.
void Bola::RebaterNaGrade(float nx, float ny, float cx, float cy, float cz)
{
    uint32_t h = HashDoContato(cx, cy, cz);
    float restituicao = RestituicaoDaGrade + VariacaoDaRestituicaoDaGrade * (2 * Uniforme(h) - 1);
    if (!Rebater(nx, ny, 0, restituicao, AtritoDaGrade)) return;

    // Pra onde a saída pende: u, um vetor no plano da parede (t1 horizontal ao longo dela, z vertical), com ângulo
    // φ em [-90°, 90°] (seno uniforme); o sinal do giro cobre a outra metade. O eixo do giro é n × u.
    float sf = 2 * Uniforme(h) - 1, cf = std::sqrt(1 - sf * sf);
    float t1x = -ny, t1y = nx;
    float ux = cf * t1x, uy = cf * t1y, uz = sf;
    float ax = ny * uz, ay = -nx * uz, az = nx * uy - ny * ux;
    float s = SenoDoDesvioMaximoDaGrade * (2 * Uniforme(h) - 1), c = std::sqrt(1 - s * s);

    // Rodrigues: v' = v·cos + (a × v)·sen + a·(a·v)·(1 − cos).
    float vn = Vx * nx + Vy * ny;
    float avx = ay * Vz - az * Vy, avy = az * Vx - ax * Vz, avz = ax * Vy - ay * Vx;
    float adotv = (ax * Vx + ay * Vy + az * Vz) * (1 - c);
    float vnGirada = (Vx * c + avx * s + ax * adotv) * nx + (Vy * c + avy * s + ay * adotv) * ny;
    if (vnGirada < 0.5f * vn) s = -s;
    Vx = Vx * c + avx * s + ax * adotv;
    Vy = Vy * c + avy * s + ay * adotv;
    Vz = Vz * c + avz * s + az * adotv;
}

/// Colisão com um plano de normal n (pra dentro da quadra): restituição na normal e, na tangente, o impulso de atrito
/// que leva o ponto de contato a rolar (limitado por μ vezes o impulso normal) — é o que transfere spin.
/// Devolve false se a bola já se afastava do plano.
bool Bola::Rebater(float nx, float ny, float nz, float restituicao, float atrito)
{
    float vn = Vx * nx + Vy * ny + Vz * nz;
    if (vn >= 0) return false;
    float impulsoNormal = -(1 + restituicao) * vn;
    Vx += impulsoNormal * nx; Vy += impulsoNormal * ny; Vz += impulsoNormal * nz;
    // Velocidade do ponto de contato (em −R·n): v_t − R (ω × n).
    float vnDepois = Vx * nx + Vy * ny + Vz * nz;
    float tx = Vx - vnDepois * nx, ty = Vy - vnDepois * ny, tz = Vz - vnDepois * nz;
    float cx = Wy * nz - Wz * ny, cy = Wz * nx - Wx * nz, cz = Wx * ny - Wy * nx;
    float sx = tx - Raio * cx, sy = ty - Raio * cy, sz = tz - Raio * cz;
    float s = std::sqrt(sx * sx + sy * sy + sz * sz);
    if (s < 1e-5f) return true;
    // Impulso (por massa) pra rolar: Δv = −s·α/(1+α); limitado pelo atrito: μ·(1+e)·|vn|.
    float desejado = s * MomentoDeInercia / (1 + MomentoDeInercia);
    float maximo = atrito * impulsoNormal;
    float j = std::min(desejado, maximo);
    float dvx = -sx / s * j, dvy = -sy / s * j, dvz = -sz / s * j;
    Vx += dvx; Vy += dvy; Vz += dvz;
    // Δω = −(1/(α·R)) (n × Δv).
    float k = -1.f / (MomentoDeInercia * Raio);
    Wx += k * (ny * dvz - nz * dvy);
    Wy += k * (nz * dvx - nx * dvz);
    Wz += k * (nx * dvy - ny * dvx);
    return true;
}

/// A bola sai pelo ponto em que cruzou o plano da parede e fica ali — dentro da caixa da quadra, mesmo no canto.
void Bola::Sair(std::vector<EventoDaBola>& eventos, float x, float y, float z, QualParede parede, bool pelaPorta)
{
    EmJogo = false;
    X = Limitar(x, -quadra::MeiaLargura, quadra::MeiaLargura);
    Y = Limitar(y, -quadra::MeioComprimento, quadra::MeioComprimento);
    Z = z;
    eventos.push_back({TipoDeEventoDaBola::Saiu, X, Y, Z, quadra::LadoDe(Y), parede, Superficie::Aberto, pelaPorta});
}
} // namespace padel
