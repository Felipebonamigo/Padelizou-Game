// Porte linha a linha de Padel.Core/GolpesEspeciais.cs: chiquita, contrapared, remate por 3 e por 4. Cada solucionador
// lança candidatos, SIMULA a Bola e confere a sequência de eventos (CruzouRede, Quique, Parede, Saiu); só vale a solução
// que sobrevive a pequenas variações. Sem solução daquela posição, nullopt — quem chamou escolhe outro golpe.
// A ordem das operações em float é a do C# de propósito: (a*b)*c e a*(b*c) dão resultados diferentes, e o gabarito
// (Testes/PadelSim/Gabarito/golpes-especiais.csv) confere bit a bit — inclusive os sub-passos de bola de cada chamada,
// porque o orçamento da chiquita conta por eles e decide o resultado.
//
// Ficou fora do porte o diagnóstico do C#: GolpesEspeciais.SubPassosSimulados, o modo Coleta (ChamadaDeSolucionador,
// enum Solucionador) e o Anotar que os alimenta. Não mudam nenhum resultado; pra medir, leia Bola::SubPassosNestaThread
// antes e depois da chamada.
//
// LIBM: diferente da Bola, os solucionadores usam funções transcendentes, como o C# (MathF.*): sin e cos (remates:
// lançamento, Reta, NoQuique), atan2 (remate por 4), exp (chiquita: Variacao; remates: NoQuique) e pow (chiquita). No
// Linux o .NET as tira da glibc, a mesma libm do C++ aqui; noutra libm (a do MSVC) o resultado pode mudar no último bit.
#include "PadelSim/GolpesEspeciais.h"

#include "PadelSim/Bola.h"
#include "PadelSim/Golpes.h"
#include "PadelSim/Quadra.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <queue>
#include <vector>

namespace padel::golpes_especiais
{
namespace
{
constexpr float Pi = 3.14159265358979323846f;   // = MathF.PI
constexpr float Infinito = std::numeric_limits<float>::infinity();
constexpr float NaN = std::numeric_limits<float>::quiet_NaN();

// ───────────── O que o C# tem pronto ─────────────

// MathF.Max/Min (IEEE 754:2019 maximum/minimum): NaN em qualquer argumento dá NaN, e +0 é maior que −0. O std::max/min
// não faz nenhum dos dois (devolve o primeiro argumento quando a comparação com NaN é falsa).
float MaxF(float x, float y)
{
    if (x != y) return !std::isnan(x) ? (y < x ? x : y) : x;
    return std::signbit(y) ? x : y;
}
float MinF(float x, float y)
{
    if (x != y) return !std::isnan(x) ? (x < y ? x : y) : x;
    return std::signbit(x) ? x : y;
}
/// Util.Limitar.
float Limitar(float valor, float minimo, float maximo) { return MinF(maximo, MaxF(minimo, valor)); }
/// MathF.Sign (o C# lança exceção com NaN; aqui nunca chega NaN).
int32_t Sinal(float v) { return (v > 0) - (v < 0); }
bool EhMaisInfinito(float v) { return v == Infinito; }   // float.IsPositiveInfinity

// A libm, nos únicos pontos em que o C# chama MathF.Sin/Cos/Atan2/Exp/Pow — sempre em tempo de execução, chamada por
// ponteiro volatile, pra o compilador não calcular na compilação o seno de uma constante (as 17 elevações do remate são
// constantes num laço de tamanho fixo). Ele calcula com arredondamento correto (gcc pelo MPFR, clang em double), e a
// libm nem sempre arredonda certo: na glibc 2.39, sinf(−26°) dá -0x1.c0e45ep-2 e o correto é -0x1.c0e46p-2. O laço
// `for k < 17: std::sin(elevação k)` com -O3 dá o correto no gcc e no clang; o C# (que chama a glibc) dá o outro.
float (*volatile const SinDaLibm)(float) = [](float x) { return std::sin(x); };
float (*volatile const CosDaLibm)(float) = [](float x) { return std::cos(x); };
float (*volatile const Atan2DaLibm)(float, float) = [](float y, float x) { return std::atan2(y, x); };
float (*volatile const ExpDaLibm)(float) = [](float x) { return std::exp(x); };
float (*volatile const PowDaLibm)(float, float) = [](float x, float y) { return std::pow(x, y); };
float Sin(float x) { return SinDaLibm(x); }
float Cos(float x) { return CosDaLibm(x); }
float Atan2(float y, float x) { return Atan2DaLibm(y, x); }
float Exp(float x) { return ExpDaLibm(x); }
float Pow(float x, float y) { return PowDaLibm(x, y); }

Golpe NovoGolpe(float alvoX, float alvoY, float tempoDeVoo, TipoDeGolpe tipo, const Efeito& efeito, const Velocidade& lancamento)
{
    Golpe g;
    g.AlvoX = alvoX;
    g.AlvoY = alvoY;
    g.TempoDeVoo = tempoDeVoo;
    g.Tipo = tipo;
    g.Efeito = efeito;
    g.Lancamento = lancamento;
    return g;
}

constexpr float Passo = 1.f / 120.f;
constexpr float PassoGrosso = 1.f / 60.f;

// ───────────── Simulação e robustez ─────────────

struct Marca
{
    EventoDaBola Evento;
    float T;
};
using Marcas = std::vector<Marca>;
/// Func<List<Marca>, bool>?: vazio é o null do C#.
using Predicado = std::function<bool(const Marcas&)>;

/// Orçamento de trabalho, em sub-passos de bola: o Simular cobra cada passo antes de dá-lo e para quando não cabe mais.
/// Com pai, cobra dos dois (a parte de uma família de alvo dentro do orçamento da chamada).
class Orcamento
{
public:
    explicit Orcamento(int32_t subPassos, Orcamento* pai = nullptr) : _restante(subPassos), _pai(pai) {}
    bool Esgotado() const { return _esgotado; }
    /// Tira n do que resta; false (e esgotado dali em diante) se não cabe.
    bool Cobrar(int32_t n)
    {
        if (_esgotado || n > _restante || (_pai != nullptr && !_pai->Cobrar(n))) { _esgotado = true; return false; }
        _restante -= n;
        return true;
    }

private:
    int32_t _restante;
    Orcamento* _pai;
    bool _esgotado = false;
};

/// Simula até a bola parar, sair, dar maximoDeEventos eventos ou passar o tempo; devolve os eventos com o instante de
/// cada um. Com prefixo, para assim que os eventos até ali já não podem dar um golpe que vale. Com orçamento, cobra
/// cada passo dele e para quando ele acaba.
Marcas Simular(float x0, float y0, float z0, const Velocidade& v, float segundos, int32_t maximoDeEventos, float passo = Passo,
               const Predicado& prefixo = {}, Orcamento* orcamento = nullptr)
{
    Bola bola;
    bola.Posicionar(x0, y0, z0);
    bola.Lancar(v);
    std::vector<EventoDaBola> eventos;
    eventos.reserve(4);
    Marcas marcas;
    marcas.reserve(static_cast<size_t>(maximoDeEventos) + 2);
    int32_t subPassosPorPasso = static_cast<int32_t>(std::ceil(passo / Bola::PassoMaximo - 1e-3f));
    for (float t = 0; t < segundos && bola.EmJogo && !bola.Parada && static_cast<int32_t>(marcas.size()) < maximoDeEventos;)
    {
        if (orcamento != nullptr && !orcamento->Cobrar(subPassosPorPasso)) break;
        eventos.clear();
        bola.Avancar(passo, eventos);
        t += passo;
        if (eventos.empty()) continue;
        for (const auto& e : eventos) marcas.push_back({e, t});
        if (prefixo && !prefixo(marcas)) break;
    }
    return marcas;
}

Velocidade Escalar(Velocidade v, float f)
{
    v.Vx = v.Vx * f;
    v.Vy = v.Vy * f;
    v.Vz = v.Vz * f;
    return v;
}
float Rapidez(const Velocidade& v) { return std::sqrt(v.Vx * v.Vx + v.Vy * v.Vy + v.Vz * v.Vz); }

/// A margem de força: golpe de potência aguenta 3 % e 0,4 m/s de tremida; golpe de toque (chiquita), 2 % e 0,2 m/s.
struct Margem
{
    float Rapidez, Vz;
};
constexpr Margem DePotencia{0.03f, 0.4f};
constexpr Margem DeToque{0.02f, 0.2f};

/// A solução vale se continua valendo saindo um pouco mais alta e mais baixa, com a bola um pouco mais forte e mais
/// fraca e com o passo de simulação a 60 Hz. nullopt se aguenta todas; senão, os eventos da primeira que não vale.
std::optional<Marcas> PrimeiraVariacaoQueNaoVale(float x0, float y0, float z0, const Velocidade& v, float segundos, int32_t maximoDeEventos,
                                                 Margem margem, const Predicado& vale, const Predicado& prefixo, Orcamento* orcamento = nullptr)
{
    Velocidade variada = v;
    variada.Vz = v.Vz + margem.Vz;
    Marcas m = Simular(x0, y0, z0, variada, segundos, maximoDeEventos, Passo, prefixo, orcamento);
    if (!vale(m)) return m;
    variada = v;
    variada.Vz = v.Vz - margem.Vz;
    m = Simular(x0, y0, z0, variada, segundos, maximoDeEventos, Passo, prefixo, orcamento);
    if (!vale(m)) return m;
    m = Simular(x0, y0, z0, Escalar(v, 1 + margem.Rapidez), segundos, maximoDeEventos, Passo, prefixo, orcamento);
    if (!vale(m)) return m;
    m = Simular(x0, y0, z0, Escalar(v, 1 - margem.Rapidez), segundos, maximoDeEventos, Passo, prefixo, orcamento);
    if (!vale(m)) return m;
    m = Simular(x0, y0, z0, v, segundos, maximoDeEventos, PassoGrosso, prefixo, orcamento);
    if (vale(m)) return std::nullopt;
    return m;
}

bool Robusta(float x0, float y0, float z0, const Velocidade& v, float segundos, int32_t maximoDeEventos, Margem margem,
             const Predicado& vale, const Predicado& prefixo)
{
    return !PrimeiraVariacaoQueNaoVale(x0, y0, z0, v, segundos, maximoDeEventos, margem, vale, prefixo).has_value();
}

// ───────────── Chiquita ─────────────

constexpr float DistanciasDaChiquita[] = {2.0f, 2.5f, 3.0f};
/// O Golpes.Calcular parava a até 12 cm do alvo, quase sempre do lado de cá: mirar ali mantém a mais lenta.
constexpr float AquemDoAlvoDaChiquita = 0.1f;
constexpr float TopspinsDaChiquita[] = {800.f, 1500.f};
/// A inclinação mais alta que se tenta (vz/vh, ~89°).
constexpr float InclinacaoMaximaDaChiquita = 60.f;
/// A folga na rede além do que as variações da robustez mexem na altura ali.
constexpr float FolgaNaRedeDaChiquita = 0.001f, FolgaRelativaNaRedeDaChiquita = 0.1f;
constexpr int32_t IteracoesDaChiquita = 24;
/// O tempo de voo do balão: o mais longo com folga pras variações da robustez dentro dos 3 s da simulação.
constexpr float TempoDoBalao = 2.8f;
/// Orçamento de uma chamada, em sub-passos de bola, e de cada família dentro dela.
constexpr int32_t OrcamentoDaChiquita = 20000, OrcamentoPorFamiliaDaChiquita = 10000;

/// O quanto as variações da robustez mexem na altura na rede, tRede segundos depois do toque, com a bola a essa rapidez:
/// ±2 % de rapidez, ±0,02·g·t²; ±0,2 m/s em vz, amortecido pelo arrasto: ±0,2·(1 − e^(−K·v·t))/(K·v).
/// (A função local estática Variacao do ChiquitaDaFamilia.)
float VariacaoDaChiquita(float tRede, float rapidez)
{
    constexpr float g = Bola::G;
    float kv = Bola::KArrasto() * rapidez;
    float porVz = kv * tRede > 1e-4f ? DeToque.Vz * (1 - Exp(-kv * tRede)) / kv : DeToque.Vz * tRede;
    return (1 + FolgaRelativaNaRedeDaChiquita) * MaxF(porVz, DeToque.Rapidez * g * tRede * tRede) + FolgaNaRedeDaChiquita;
}

/// A chiquita que quica a `distancia` da rede na direção de alvoX: a do arco mais lento até lá cuja altura na rede cabe
/// entre a rede e o limite, com a folga do que as variações da robustez mexem na bola ali. A família é parametrizada
/// pela inclinação do lançamento, p = vz/vh; pra cada p, a rapidez sai de uma secante com intervalo, e p anda por uma
/// secante na altura medida na rede. Quando o que prende é o tempo, o Balao resolve direto o tempo de voo.
std::optional<Golpe> ChiquitaDaFamilia(float x0, float y0, float z0, int32_t ladoDoAlvo, float alvoX, float distancia, float topspin,
                                       Orcamento& orcamento, bool& presaNoTeto)
{
    presaNoTeto = false;
    constexpr float Limite = quadra::AlturaDaRede + FolgaDaChiquitaSobreARede;
    constexpr float g = Bola::G;
    const Predicado Vale = [&](const Marcas& m) {
        return m.size() >= 2
            && m[0].Evento.Tipo == TipoDeEventoDaBola::CruzouRede && m[0].Evento.Lado == ladoDoAlvo
            && m[0].Evento.Z <= Limite
            && m[1].Evento.Tipo == TipoDeEventoDaBola::Quique && m[1].Evento.Lado == ladoDoAlvo
            && std::fabs(m[1].Evento.Y) <= AlcanceDaChiquita;
    };
    const Predicado Prefixo = [&](const Marcas& m) {
        return m[0].Evento.Tipo == TipoDeEventoDaBola::CruzouRede && m[0].Evento.Lado == ladoDoAlvo && m[0].Evento.Z <= Limite
            && (m.size() < 2
                || (m[1].Evento.Tipo == TipoDeEventoDaBola::Quique && m[1].Evento.Lado == ladoDoAlvo && std::fabs(m[1].Evento.Y) <= AlcanceDaChiquita));
    };

    // Direção e efeito como o Golpes.Calcular: topspin de quem vai em d̂ é o eixo (up × d̂) = (−dy, dx, 0).
    float dx = alvoX - x0, dy = ladoDoAlvo * distancia - y0;
    float distanciaAoAlvo = std::sqrt(dx * dx + dy * dy);
    float dirX = dx / distanciaAoAlvo, dirY = dy / distanciaAoAlvo;
    float w = topspin * (2 * Pi / 60.f);
    const Efeito efeito{topspin, 0};
    float alcance = distanciaAoAlvo - AquemDoAlvoDaChiquita;
    float ateARede = distanciaAoAlvo * std::fabs(y0) / std::fabs(dy);
    float fracao = ateARede / alcance;
    if (!(fracao > 0 && fracao < 1)) return std::nullopt;   // em cima da rede (y0 = 0) a Bola não vê a bola cruzar
    // O caminho fica acima da corda até o quique: se ela já passa acima do limite na rede, não há chiquita pra esse alvo.
    if (z0 * (1 - fracao) > Limite + 0.02f) return std::nullopt;

    // O modelo no vácuo, em p: a rapidez horizontal pra quicar no alcance e a altura na rede.
    auto VhNoVacuo = [&](float p) { return alcance * std::sqrt(g / (2 * (z0 + p * alcance))); };
    auto ZNaRedeNoVacuo = [&](float p) { return z0 * (1 - fracao * fracao) + p * ateARede * (1 - fracao); };
    // O intervalo aberto de p onde pode haver chiquita. Embaixo: a rapidez máxima do remate (no vácuo) e a reta do
    // lançamento. Em cima, ~89°.
    float pBaixo = MaxF((g * alcance * alcance / (2 * RapidezMaximaDoRemate * RapidezMaximaDoRemate) - z0) / alcance,
                        (quadra::AlturaDaRede - z0) / ateARede);
    float pAlto = InclinacaoMaximaDaChiquita;
    if (!(pBaixo < pAlto)) return std::nullopt;
    auto PrimeiroP = [&](const std::function<bool(float)>& verdadeiro) -> float {   // num predicado falso… verdadeiro em p: onde vira (+∞: nunca)
        float lo = pBaixo, hi = pAlto;
        if (verdadeiro(lo)) return lo;
        if (!verdadeiro(hi)) return Infinito;
        for (int32_t i = 0; i < 30; i++)
        {
            float meio = 0.5f * (lo + hi);
            if (verdadeiro(meio)) hi = meio;
            else lo = meio;
        }
        return hi;
    };
    float pMaisLento = (std::sqrt(alcance * alcance + z0 * z0) - z0) / alcance;
    auto VariacaoNoVacuo = [&](float p) { return VariacaoDaChiquita(ateARede / VhNoVacuo(p), VhNoVacuo(p) * std::sqrt(1 + p * p)); };
    float piso = PrimeiroP([&](float p) { return ZNaRedeNoVacuo(p) - VariacaoNoVacuo(p) >= quadra::AlturaDaRede; });
    float teto = PrimeiroP([&](float p) { return ZNaRedeNoVacuo(p) + VariacaoNoVacuo(p) > Limite; });
    float inclinacao = piso <= teto ? Limitar(pMaisLento, piso, teto) : 0.5f * (MinF(piso, pAlto) + MinF(teto, pAlto));
    if (!(inclinacao > pBaixo && inclinacao < pAlto)) inclinacao = 0.5f * (pBaixo + pAlto);

    // Pra p fixo: a rapidez horizontal entre uma curta e uma longa. O arrasto encurta a bola como no Golpes.Calcular.
    float vh = VhNoVacuo(inclinacao) * (1 + Bola::KArrasto() * alcance * 0.5f);
    float vhCurta = 0, erroCurta = NaN, vhLonga = Infinito, erroLonga = NaN;
    float zNaRedeDaCurta = NaN;   // a altura na rede da curta (NaN: quicou antes dela)
    bool longaNoAr = false;
    float pAnterior = NaN, zAnterior = NaN;   // o último (p, altura na rede) medido, pra secante
    int32_t mesmoSentido = 0;                  // quantas mudanças de p seguidas no mesmo sentido

    // Muda p. subir: esse p é baixo demais (vira o piso do intervalo); senão, alto demais (vira o teto). false se o
    // intervalo fechou: essa família não tem chiquita.
    auto MudarInclinacao = [&](bool subir, float zNaRede, float zQuerido) -> bool {
        mesmoSentido = subir == (mesmoSentido > 0) ? mesmoSentido + (subir ? 1 : -1) : (subir ? 1 : -1);
        zQuerido += (subir ? 1 : -1) * MinF(0.1f, FolgaNaRedeDaChiquita * (Pow(3.f, std::fabs(static_cast<float>(mesmoSentido)) - 1) - 1));
        if (subir) pBaixo = MaxF(pBaixo, inclinacao);
        else pAlto = MinF(pAlto, inclinacao);
        if (!(pAlto - pBaixo > 1e-3f * (1 + std::fabs(inclinacao)))) return false;
        float nova = NaN;
        if (std::isfinite(zNaRede) && std::isfinite(zQuerido))
        {
            float derivada = ateARede * (1 - fracao);   // no vácuo
            if (std::isfinite(pAnterior) && std::fabs(inclinacao - pAnterior) > 1e-4f)
            {
                float secante = (zNaRede - zAnterior) / (inclinacao - pAnterior);
                if (secante > 0.2f * derivada) derivada = secante;
            }
            pAnterior = inclinacao;
            zAnterior = zNaRede;
            nova = inclinacao + (zQuerido - zNaRede) / derivada;
        }
        if (!(nova > pBaixo && nova < pAlto)) nova = 0.5f * (pBaixo + pAlto);
        vh *= VhNoVacuo(nova) / VhNoVacuo(inclinacao);
        inclinacao = nova;
        vhCurta = 0; erroCurta = NaN; vhLonga = Infinito; erroLonga = NaN; zNaRedeDaCurta = NaN; longaNoAr = false;
        return true;
    };

    // O candidato que cruzou e quica no alvo: a altura na rede cabe na janela da robustez, e ele aguenta as variações?
    // Devolve o golpe; senão, em zQuerido, a altura na rede que ele devia ter (NaN: o que falhou não é a altura na rede).
    auto Avaliar = [&](const Velocidade& v, const Marcas& m, float vhDoLancamento, float& zQuerido) -> std::optional<Golpe> {
        float zNaRede = m[0].Evento.Z;
        float variacao = VariacaoDaChiquita(Limitar(ateARede / vhDoLancamento, m[0].T - Passo, m[0].T), Rapidez(v));
        zQuerido = NaN;
        if (zNaRede > Limite - variacao) { zQuerido = Limite - variacao - FolgaNaRedeDaChiquita; return std::nullopt; }
        if (zNaRede < quadra::AlturaDaRede + variacao) { zQuerido = quadra::AlturaDaRede + variacao + FolgaNaRedeDaChiquita; return std::nullopt; }
        std::optional<Marcas> falha = PrimeiraVariacaoQueNaoVale(x0, y0, z0, v, 3.f, 2, DeToque, Vale, Prefixo, &orcamento);
        if (orcamento.Esgotado()) return std::nullopt;
        if (!falha) return NovoGolpe(m[1].Evento.X, m[1].Evento.Y, m[1].T, TipoDeGolpe::Chiquita, efeito, v);
        // Passou por cima do limite numa variação: mais baixo. Bateu na rede ou quicou antes dela: mais alto. Outra coisa:
        // não é a altura na rede. (at: o falha[0] do C# também lança exceção com a lista vazia.)
        const EventoDaBola primeiro = falha->at(0).Evento;
        if (primeiro.Tipo == TipoDeEventoDaBola::CruzouRede && primeiro.Z > Limite) zQuerido = zNaRede - (primeiro.Z - Limite) - FolgaNaRedeDaChiquita;
        else if (primeiro.Tipo == TipoDeEventoDaBola::Rede) zQuerido = zNaRede + MaxF(variacao, 5 * FolgaNaRedeDaChiquita);
        else if (primeiro.Tipo == TipoDeEventoDaBola::Quique) zQuerido = zNaRede + 0.05f;
        return std::nullopt;
    };

    // O balão: colada na rede e abaixo dela, o que manda é o tempo. Newton direto em (vh, vz) pra quicar no alvo em
    // TempoDoBalao. (No C#, os parâmetros se chamam vh e vz e escondem os de fora.)
    auto Balao = [&](float vhDoBalao, float vzDoBalao) -> std::optional<Golpe> {
        for (int32_t iteracao = 0; iteracao < 6; iteracao++)
        {
            const Velocidade v{vhDoBalao * dirX, vhDoBalao * dirY, vzDoBalao, 0, -dirY * w, dirX * w, 0};
            Marcas m = Simular(x0, y0, z0, v, 3.f, 2, Passo, {}, &orcamento);
            if (orcamento.Esgotado() || m.empty() || m[0].Evento.Tipo != TipoDeEventoDaBola::CruzouRede || m[0].Evento.Lado != ladoDoAlvo)
                return std::nullopt;
            if (m.size() < 2) { vzDoBalao -= 0.5f; continue; }   // ainda no ar aos 3 s
            if (m[1].Evento.Tipo != TipoDeEventoDaBola::Quique) return std::nullopt;
            float tempo = m[1].T - Passo / 2;   // quicou em algum ponto do último passo
            float erroNoQuique = (m[1].Evento.X - x0) * dirX + (m[1].Evento.Y - y0) * dirY - alcance, erroNoTempo = tempo - TempoDoBalao;
            if (std::fabs(erroNoQuique) < 0.05f && std::fabs(erroNoTempo) < 0.06f)
            {
                float descartado = NaN;
                return Avaliar(v, m, vhDoBalao, descartado);
            }
            float tempoPorVz = tempo / MaxF(0.5f, g * tempo - vzDoBalao);
            float dvz = -erroNoTempo / tempoPorVz;
            vhDoBalao += (-erroNoQuique - vhDoBalao * tempoPorVz * dvz) / tempo;
            vzDoBalao += dvz;
            if (!(vhDoBalao > 0.02f) || !std::isfinite(vhDoBalao) || !std::isfinite(vzDoBalao)) return std::nullopt;
        }
        return std::nullopt;
    };

    for (int32_t iteracao = 0; iteracao < IteracoesDaChiquita; iteracao++)
    {
        const Velocidade v{vh * dirX, vh * dirY, inclinacao * vh, 0, -dirY * w, dirX * w, 0};
        Marcas m = Simular(x0, y0, z0, v, 3.f, 2, Passo, {}, &orcamento);
        if (orcamento.Esgotado()) return std::nullopt;

        // Onde ela caiu: erro (quique − alcance) quando cruzou e quicou; senão, só o lado — curta ou longa.
        float erro = NaN, zNaRede = NaN;
        bool curta = false, noAr = false;
        if (m.empty()) { curta = false; noAr = true; }
        else if (m[0].Evento.Tipo == TipoDeEventoDaBola::CruzouRede && m[0].Evento.Lado == ladoDoAlvo)
        {
            zNaRede = m[0].Evento.Z;
            if (m.size() < 2) { curta = false; noAr = true; }
            else if (m[1].Evento.Tipo == TipoDeEventoDaBola::Quique)
            {
                erro = (m[1].Evento.X - x0) * dirX + (m[1].Evento.Y - y0) * dirY - alcance;
                curta = erro < 0;
            }
            else curta = false;   // parede ou saída antes de quicar
        }
        // Bateu na rede: passou abaixo dela.
        else if (m[0].Evento.Tipo == TipoDeEventoDaBola::Rede) { curta = true; zNaRede = quadra::AlturaDaRede - FolgaNaRedeDaChiquita; }
        else if (m[0].Evento.Tipo == TipoDeEventoDaBola::Quique) curta = true;
        else return std::nullopt;   // parede ou saída antes da rede

        if (std::fabs(erro) < 0.05f)
        {
            float zQuerido = NaN;
            if (std::optional<Golpe> golpe = Avaliar(v, m, vh, zQuerido))
            {
                // Colada na rede (ela no primeiro quinto do caminho), o arco mais lento passava alto demais: desceu.
                presaNoTeto = inclinacao < pMaisLento && fracao < 0.2f;
                return golpe;
            }
            if (orcamento.Esgotado()) return std::nullopt;
            // O que falhou não foi a altura na rede: perto do fim dos 3 s, é o tempo — o balão.
            if (!std::isfinite(zQuerido)) return m[1].T >= TempoDoBalao ? Balao(vh, inclinacao * vh) : std::optional<Golpe>{};
            if (!MudarInclinacao(zQuerido > zNaRede, zNaRede, zQuerido)) return std::nullopt;
            continue;
        }
        // Curta e já quase no fim dos 3 s: quicar no alvo com esse p passa do tempo — é o balão.
        if (curta && std::isfinite(erro) && m[1].T >= TempoDoBalao + 0.1f) return Balao(vh, inclinacao * vh);

        // Fora do alvo. Com p fixo, a altura na rede cresce com a rapidez e nunca passa da reta do lançamento: alta demais
        // e curta, ou baixa demais e longa — ou baixa demais sem que nem a reta alcance a janela —: muda p já.
        if (std::isfinite(zNaRede))
        {
            float variacao = VariacaoDaChiquita(Limitar(ateARede / vh, 0.f, m[0].T), vh * std::sqrt(1 + inclinacao * inclinacao));
            float reta = z0 + inclinacao * ateARede;
            // Bateu na rede com a reta do lançamento mal passando a janela: quem resolve é p, não rapidez.
            if (m[0].Evento.Tipo == TipoDeEventoDaBola::Rede && reta < quadra::AlturaDaRede + variacao + 0.05f)
            {
                if (!MudarInclinacao(true, reta - 0.05f, quadra::AlturaDaRede + variacao + FolgaNaRedeDaChiquita)) return std::nullopt;
                continue;
            }
            if (zNaRede > Limite - variacao && curta)
            {
                if (!MudarInclinacao(false, zNaRede, Limite - variacao - FolgaNaRedeDaChiquita)) return std::nullopt;
                continue;
            }
            if (zNaRede < quadra::AlturaDaRede + variacao && (!curta || reta < quadra::AlturaDaRede + variacao))
            {
                if (!MudarInclinacao(true, zNaRede, quadra::AlturaDaRede + variacao + FolgaNaRedeDaChiquita)) return std::nullopt;
                continue;
            }
        }
        // Aperta o intervalo da rapidez.
        if (curta && vh > vhCurta) { vhCurta = vh; erroCurta = erro; zNaRedeDaCurta = zNaRede; }
        if (!curta && vh < vhLonga) { vhLonga = vh; erroLonga = erro; longaNoAr = noAr; }
        if (vhLonga < vhCurta * 1.003f)
        {
            // O intervalo fechou sem quicar no alvo. Ainda no ar aos 3 s do lado longo: é o balão. Bateu na rede do lado
            // curto: sobe p.
            if (longaNoAr) return Balao(vhCurta, inclinacao * vhCurta);
            if (!MudarInclinacao(true, zNaRedeDaCurta,
                                 quadra::AlturaDaRede + VariacaoDaChiquita(ateARede / vhCurta, vhCurta * std::sqrt(1 + inclinacao * inclinacao)) + FolgaNaRedeDaChiquita))
                return std::nullopt;
            continue;
        }
        // Próxima rapidez: secante entre as duas pontas quando as duas quicaram; senão, pela lei de potência, dentro do
        // intervalo; senão, o meio geométrico dele.
        float proxima;
        if (std::isfinite(erroCurta) && std::isfinite(erroLonga) && std::isfinite(vhLonga))
            proxima = vhCurta + (vhLonga - vhCurta) * (-erroCurta) / (erroLonga - erroCurta);
        else if (std::isfinite(erro))
            proxima = vh * Pow(alcance / MaxF(0.05f, alcance + erro), 0.67f);
        else proxima = curta ? vh * 1.3f : vh * 0.75f;
        if (std::isfinite(vhLonga) && vhCurta > 0 && !(proxima > vhCurta && proxima < vhLonga)) proxima = std::sqrt(vhCurta * vhLonga);
        else if (std::isfinite(vhLonga) && !(proxima < vhLonga)) proxima = 0.75f * vhLonga;
        else if (vhCurta > 0 && !(proxima > vhCurta)) proxima = 1.3f * vhCurta;
        if (!(proxima > 0.02f) || !std::isfinite(proxima) || proxima * std::sqrt(1 + inclinacao * inclinacao) > RapidezMaximaDoRemate)
            return std::nullopt;
        vh = proxima;
    }
    return std::nullopt;
}

/// Famílias de alvo: quicar a 2,0, 2,5 ou 3,0 m da rede na direção de alvoX, com 800 ou 1.500 rpm de topspin. Para na
/// primeira distância que tem chiquita, salvo se a melhor ficou presa no teto da rede; e no fim do orçamento.
std::optional<Golpe> ResolverChiquita(float x0, float y0, float z0, int32_t ladoDoAlvo, float alvoX)
{
    alvoX = Limitar(alvoX, -quadra::MeiaLargura + 0.5f, quadra::MeiaLargura - 0.5f);
    if (quadra::LadoDe(y0) == ladoDoAlvo) return std::nullopt;                      // já do lado do alvo: não há rede pra cruzar
    if (std::fabs(y0) < DistanciaMinimaDaChiquitaDaRede) return std::nullopt;       // colada na rede: raquete na rede
    Orcamento orcamento(OrcamentoDaChiquita);
    std::optional<Golpe> melhor;
    float menorRapidez = Infinito;
    bool melhorPresaNoTeto = false;
    for (float distancia : DistanciasDaChiquita)
    {
        for (float topspin : TopspinsDaChiquita)
        {
            Orcamento daFamilia(OrcamentoPorFamiliaDaChiquita, &orcamento);
            bool presaNoTeto = false;
            std::optional<Golpe> g = ChiquitaDaFamilia(x0, y0, z0, ladoDoAlvo, alvoX, distancia, topspin, daFamilia, presaNoTeto);
            if (!g || !g->Lancamento || !(Rapidez(*g->Lancamento) < menorRapidez)) continue;
            menorRapidez = Rapidez(*g->Lancamento);
            melhorPresaNoTeto = presaNoTeto;
            melhor = g;
        }
        if ((melhor && !melhorPresaNoTeto) || orcamento.Esgotado()) return melhor;
    }
    return melhor;
}

// ───────────── Contrapared ─────────────

constexpr int32_t ColunasDaContrapared = 8;   // vy = 6, 8, …, 20 m/s
constexpr int32_t LinhasDaContrapared = 10;   // vz = 4, 5, …, 13 m/s

/// Num predicado falso… verdadeiro sobre 0..n−1, o primeiro índice verdadeiro (n se nenhum), galopando a partir da dica.
int32_t PrimeiroVerdadeiro(int32_t n, int32_t dica, const std::function<bool(int32_t)>& verdadeiro)
{
    int32_t h = std::clamp(dica, 0, n - 1);
    int32_t lo, hi;   // falso em lo − 1 (ou lo = 0), verdadeiro em hi (ou hi = n)
    if (verdadeiro(h))
    {
        lo = 0;
        hi = h;
        for (int32_t salto = 1; hi > 0; salto *= 2)
        {
            int32_t k = std::max(0, h - salto);
            if (!verdadeiro(k)) { lo = k + 1; break; }
            hi = k;
            if (k == 0) break;
        }
    }
    else
    {
        lo = h + 1;
        hi = n;
        for (int32_t salto = 1; lo < n; salto *= 2)
        {
            int32_t k = std::min(n - 1, h + salto);
            if (verdadeiro(k)) { hi = k; break; }
            lo = k + 1;
            if (k == n - 1) break;
        }
    }
    while (lo < hi)
    {
        int32_t meio = (lo + hi) / 2;
        if (verdadeiro(meio)) hi = meio;
        else lo = meio + 1;
    }
    return lo;
}

/// float.CompareTo: NaN antes de tudo, −0 igual a +0 (o Comparer<float>.Default da fila de prioridade do C#).
int32_t Comparar(float a, float b)
{
    if (a < b) return -1;
    if (a > b) return 1;
    if (a == b) return 0;
    if (std::isnan(a)) return std::isnan(b) ? 0 : -1;
    return 1;
}

/// A grade de sempre — vy de 6 a 20 m/s, vz de 4 a 13, vx mirando alvoX em ~1,3 s — e a nota de sempre: |quique − 5 m|
/// + 0,05·rapidez, a menor entre as robustas, achada por busca binária em cada linha e uma fila por nota.
std::optional<Golpe> ResolverContrapared(float x0, float y0, float z0, int32_t ladoDoAlvo, float alvoX)
{
    int32_t meuLado = -ladoDoAlvo;
    const Predicado Vale = [&](const Marcas& m) {
        return m.size() >= 3
            && m[0].Evento.Tipo == TipoDeEventoDaBola::Parede && m[0].Evento.Parede == QualParede::Fundo && m[0].Evento.Lado == meuLado
            && m[1].Evento.Tipo == TipoDeEventoDaBola::CruzouRede && m[1].Evento.Lado == ladoDoAlvo
            && m[2].Evento.Tipo == TipoDeEventoDaBola::Quique && m[2].Evento.Lado == ladoDoAlvo;
    };
    const Predicado Prefixo = [&](const Marcas& m) {
        return m[0].Evento.Tipo == TipoDeEventoDaBola::Parede && m[0].Evento.Parede == QualParede::Fundo && m[0].Evento.Lado == meuLado
            && (m.size() < 2 || (m[1].Evento.Tipo == TipoDeEventoDaBola::CruzouRede && m[1].Evento.Lado == ladoDoAlvo))
            && (m.size() < 3 || (m[2].Evento.Tipo == TipoDeEventoDaBola::Quique && m[2].Evento.Lado == ladoDoAlvo));
    };

    // A bola vai à parede e volta: ~1,3 s até quicar do outro lado. Mira o x do alvo com isso.
    float vx = Limitar(alvoX, -quadra::MeiaLargura + 0.5f, quadra::MeiaLargura - 0.5f) - x0;
    vx /= 1.3f;
    auto Lancamento = [&](int32_t coluna, int32_t linha) {
        return Velocidade{vx, static_cast<float>(meuLado * (6 + 2 * coluna)), static_cast<float>(4 + linha), 0};
    };

    std::optional<Marcas> simuladas[ColunasDaContrapared][LinhasDaContrapared];
    auto Eventos = [&](int32_t coluna, int32_t linha) -> const Marcas* {
        const Velocidade v = Lancamento(coluna, linha);
        if (Rapidez(v) > RapidezMaximaDaContrapared) return nullptr;
        std::optional<Marcas>& simulada = simuladas[coluna][linha];
        if (!simulada) simulada = Simular(x0, y0, z0, v, 4.f, 3, Passo, Prefixo);
        return &*simulada;
    };
    // Longa: rápida demais, ou cruzou a rede e não quicou do outro lado antes de parede ou saída.
    auto Longa = [&](const Marcas* m) {
        return m == nullptr
            || (m->size() >= 2 && (*m)[1].Evento.Tipo == TipoDeEventoDaBola::CruzouRede && (*m)[1].Evento.Lado == ladoDoAlvo && !Vale(*m));
    };
    // Já passou do quique a 5 m (ou virou longa): falso… verdadeiro ao longo da linha.
    auto Passou = [&](int32_t coluna, int32_t linha) {
        const Marcas* m = Eventos(coluna, linha);
        return Longa(m) || (m != nullptr && Vale(*m) && std::fabs((*m)[2].Evento.Y) >= 5.f);
    };
    auto Nota = [&](const Marcas& m, const Velocidade& v) { return std::fabs(std::fabs(m[2].Evento.Y) - 5.f) + 0.05f * Rapidez(v); };

    // Fila por (nota, coluna, linha), a menor primeiro: o empate fica com quem vinha antes na varredura antiga. Cada
    // (coluna, linha) entra no máximo uma vez, então as prioridades são distintas e a ordem de saída não depende do heap.
    struct Candidata
    {
        int32_t Coluna, Linha, Sentido;
        float NotaDela;   // não "Nota": esconderia o lambda Nota (aviso do MSVC no /W4)
    };
    auto DepoisDe = [](const Candidata& a, const Candidata& b) {
        int32_t c = Comparar(a.NotaDela, b.NotaDela);
        if (c == 0) c = a.Coluna < b.Coluna ? -1 : a.Coluna > b.Coluna ? 1 : 0;
        if (c == 0) c = a.Linha < b.Linha ? -1 : a.Linha > b.Linha ? 1 : 0;
        return c > 0;
    };
    std::priority_queue<Candidata, std::vector<Candidata>, decltype(DepoisDe)> fila(DepoisDe);
    auto Enfileirar = [&](int32_t coluna, int32_t linha, int32_t sentido) {
        if (coluna < 0 || coluna >= ColunasDaContrapared) return;
        const Marcas* m = Eventos(coluna, linha);
        if (m == nullptr || !Vale(*m)) return;
        fila.push({coluna, linha, sentido, Nota(*m, Lancamento(coluna, linha))});
    };
    // De vz alto pra baixo: vz menor pede vy maior, e a coluna da linha anterior é a dica.
    int32_t dica = ColunasDaContrapared / 2;
    for (int32_t linha = LinhasDaContrapared - 1; linha >= 0; linha--)
    {
        int32_t primeira = PrimeiroVerdadeiro(ColunasDaContrapared, dica, [&](int32_t coluna) { return Passou(coluna, linha); });
        dica = primeira;
        Enfileirar(primeira - 1, linha, -1);
        Enfileirar(primeira, linha, +1);
    }
    while (!fila.empty())
    {
        const Candidata c = fila.top();
        fila.pop();
        const Velocidade v = Lancamento(c.Coluna, c.Linha);
        const Marcas* m = Eventos(c.Coluna, c.Linha);
        if (m != nullptr && Robusta(x0, y0, z0, v, 4.f, 3, DePotencia, Vale, Prefixo))
        {
            const Marca& quique = (*m)[2];
            return NovoGolpe(quique.Evento.X, quique.Evento.Y, quique.T, TipoDeGolpe::Contrapared, Efeito{}, v);
        }
        Enfileirar(c.Coluna + c.Sentido, c.Linha, c.Sentido);   // não aguentou: a próxima da linha, pra fora
    }
    return std::nullopt;
}

// ───────────── Remate por 3 e por 4 ─────────────

constexpr int32_t NiveisDeRapidez = 10;   // 22, 24, …, 40 m/s
constexpr int32_t Elevacoes = 17;         // -8°, -10°, …, -40°

/// Folgas dos cortes do remate: todas só alargam o que se simula; nunca cortam o que a varredura antiga acharia.
constexpr float FolgaNaReta = 0.01f, FolgaNaAltura = 0.25f, FolgaNaRede = 0.01f, FolgaDoPerfil = 0.03f;

/// Qual plano a bola cruza primeiro: a dúvida vai até um sub-passo de caminho na rapidez máxima do remate.
constexpr float FolgaDaQuina = RapidezMaximaDoRemate * Bola::PassoMaximo + 0.02f;

/// A reta, vista de cima, de um remate sem efeito num azimute: a distância (na reta) até a rede e até a parede de saída,
/// a altura mais baixa do topo da parede ali e se dá pra sair por baixo (a porta) — ou nada.
struct Reta
{
    float AteARede, AteASaida, AlturaMinima;
    bool PodeSairBaixo;

    static std::optional<Reta> Do(float x0, float y0, int32_t ladoDoAlvo, float azimute, int32_t lateralDaSaida)
    {
        float dx = Sin(azimute), du = Cos(azimute);   // du: rumo ao fundo do lado do alvo
        float u0 = ladoDoAlvo * y0;                   // < 0: do lado de cá da rede
        if (u0 >= 0 || du <= 1e-3f) return std::nullopt;   // já do lado do alvo, ou nunca chega à rede
        float ateARede = -u0 / du;
        float ateOFundo = (quadra::MeioComprimento - u0) / du;
        float ateALateral = dx > 1e-6f ? (quadra::MeiaLargura - x0) / dx : dx < -1e-6f ? (-quadra::MeiaLargura - x0) / dx : Infinito;
        if (ateARede > ateALateral + FolgaDaQuina) return std::nullopt;   // bate na própria lateral antes da rede
        // Na quina (fundo e lateral quase juntos) não se sabe qual parede vem primeiro: vale a mais perto e o topo mais baixo.
        float ateASaida = MinF(ateOFundo, ateALateral);
        float yNaLateral = std::fabs(u0 + du * ateALateral);   // |y| de onde a reta cruza a lateral
        float topoDaLateral = yNaLateral >= quadra::InicioDoCanto + FolgaNaReta ? quadra::AlturaDaParede : MinF(quadra::AlturaDoMeio, quadra::AlturaDoDegrau);
        bool porta = yNaLateral >= quadra::InicioDaPorta - FolgaNaReta && yNaLateral <= quadra::FimDaPorta + FolgaNaReta;
        bool quina = std::fabs(ateOFundo - ateALateral) <= FolgaDaQuina;
        if (lateralDaSaida == 0)
        {
            if (!(ateOFundo < ateALateral + FolgaDaQuina)) return std::nullopt;
            return Reta{ateARede, ateASaida, quina ? MinF(quadra::AlturaDaParede, topoDaLateral) : quadra::AlturaDaParede, quina && porta};
        }
        if (Sinal(dx) != lateralDaSaida || ateALateral > ateOFundo + FolgaDaQuina) return std::nullopt;
        return Reta{ateARede, ateASaida, quina ? MinF(quadra::AlturaDaParede, topoDaLateral) : topoDaLateral, porta};
    }
};

/// A trajetória de lado — altura contra distância horizontal — de um remate sem efeito, que é a mesma em todos os
/// azimutes: a Bola de verdade num corredor livre, reposta mais atrás perto do fundo. Só simula o trecho pedido.
class Perfil
{
public:
    /// temPorta: algum azimute sai pela porta — aí o perfil segue mesmo com a bola baixa.
    Perfil(float horizontal, float vz, float z0, bool temPorta)
    {
        _eventos.reserve(4);
        _s.reserve(96);
        _z.reserve(96);
        _quicouNoPasso.reserve(96);
        // Um sub-passo de caminho: o quique e a passagem por um plano podem cair no mesmo sub-passo e em ordens diferentes.
        _folgaNaReta = horizontal * Bola::PassoMaximo + 0.02f;
        _temPorta = temPorta;
        _bola.Posicionar(0, Inicio, z0);
        _bola.Lancar(Velocidade{0, -horizontal, vz, 0});
        _s.push_back(0);
        _z.push_back(z0);
        _quicouNoPasso.push_back(false);
    }

    /// False só quando é CERTO que o remate nesse azimute não vale. Na dúvida (dentro das folgas), true.
    bool PodeValer(const Reta& reta)
    {
        float ms = _folgaNaReta;
        SimularAte(reta.AteASaida + 2 * ms);
        if (_quique1 < reta.AteARede - ms) return false;
        if (_quique1 > reta.AteARede + ms)
        {
            std::optional<float> naRede = AlturaEm(reta.AteARede);
            if (naRede && *naRede < quadra::AlturaDaRede - FolgaDoPerfil) return false;
        }
        if (_quique1 > reta.AteASaida + ms && _s.back() >= reta.AteASaida + ms) return false;
        if (_quique2 < reta.AteASaida - ms) return false;
        if (_rolou < reta.AteASaida - ms && !reta.PodeSairBaixo) return false;
        if (_acabou && _s.back() < reta.AteASaida - ms) return false;
        if (!reta.PodeSairBaixo && _quique1 < reta.AteASaida - ms)
        {
            if (_desceuAbaixoDoMenorTopo < reta.AteASaida - ms) return false;
            std::optional<float> naParede = AlturaEm(reta.AteASaida);
            if (naParede && *naParede < reta.AlturaMinima - FolgaDoPerfil) return false;
        }
        return true;
    }

private:
    static constexpr float Inicio = -0.05f;
    /// O topo mais baixo de parede (fora da porta). static readonly no C#.
    static float MenorTopo()
    {
        static const float menorTopo = MinF(quadra::AlturaDaParede, MinF(quadra::AlturaDoMeio, quadra::AlturaDoDegrau));
        return menorTopo;
    }

    Bola _bola;
    std::vector<EventoDaBola> _eventos;
    float _folgaNaReta = 0;
    bool _temPorta = false;
    std::vector<float> _s, _z;
    std::vector<bool> _quicouNoPasso;
    float _quique1 = Infinito, _quique2 = Infinito, _rolou = Infinito;
    float _desceuAbaixoDoMenorTopo = Infinito;
    float _t = 0, _reposto = 0;
    bool _acabou = false;   // o tempo do Simular acabou ou a bola parou
    bool _parado = false;   // não adianta seguir: quicou duas vezes, rolou, ou desceu abaixo do menor topo (sem porta)

    /// Simula (com o passo e o limite de tempo do Simular) até passar da distância ate, ou até não adiantar mais.
    void SimularAte(float ate)
    {
        while (!_acabou && !_parado && _s.back() <= ate)
        {
            if (!(_t < 2.5f && _bola.EmJogo && !_bola.Parada)) { _acabou = true; return; }
            _eventos.clear();
            _bola.Avancar(Passo, _eventos);
            _t += Passo;
            bool quicou = false;
            for (const auto& e : _eventos)
            {
                if (e.Tipo != TipoDeEventoDaBola::Quique) continue;
                quicou = true;
                float sq = _reposto + (Inicio - e.Y);
                if (EhMaisInfinito(_quique1)) _quique1 = sq;
                else if (EhMaisInfinito(_quique2)) _quique2 = sq;
                if (_bola.Rolando) _rolou = MinF(_rolou, sq);
            }
            float s = _reposto + (Inicio - _bola.Y);
            _s.push_back(s);
            _z.push_back(_bola.Z);
            _quicouNoPasso.push_back(quicou);
            if (!EhMaisInfinito(_quique1) && !_temPorta && _bola.Vz < 0 && _bola.Z < MenorTopo() - FolgaDoPerfil)
                _desceuAbaixoDoMenorTopo = MinF(_desceuAbaixoDoMenorTopo, s);
            if (_bola.Rolando || !EhMaisInfinito(_quique2) || !EhMaisInfinito(_desceuAbaixoDoMenorTopo)) { _parado = true; return; }
            if (_bola.Y < -9.5f)
            {
                const Velocidade v{_bola.Vx, _bola.Vy, _bola.Vz, 0, _bola.Wx, _bola.Wy, _bola.Wz};
                float y = _bola.Y + 9.4f;
                _reposto += y - _bola.Y;
                _bola.Posicionar(_bola.X, y, _bola.Z);
                _bola.Lancar(v);
            }
        }
    }

    /// A altura na distância s, se ela está no trecho simulado e não há quique no passo que a contém.
    std::optional<float> AlturaEm(float s) const
    {
        if (s < 0 || s > _s.back()) return std::nullopt;
        size_t lo = 0, hi = _s.size() - 1;   // _s cresce: a bola nunca volta
        while (hi - lo > 1)
        {
            size_t meio = (lo + hi) / 2;
            if (_s[meio] <= s) lo = meio;
            else hi = meio;
        }
        if (_quicouNoPasso[hi] || _s[hi] <= _s[lo]) return std::nullopt;
        return _z[lo] + (_z[hi] - _z[lo]) * (s - _s[lo]) / (_s[hi] - _s[lo]);
    }
};

/// Limites RIGOROSOS do estado de um remate sem efeito no quique e depois dele, pra rapidez e ângulo abaixo da horizontal
/// dados, saindo de z0 — o que a física da Bola garante, sem simular.
struct NoQuique
{
    float QuicaDepoisDe = 0, QuicaAte = 0, Apice = 0, Inclinacao = 0, HorizontalMaxima = 0, UmSubPasso = 0;

    static NoQuique Limites(float rapidez, float angulo, float z0)
    {
        constexpr float g = Bola::G;
        float k = Bola::KArrasto(), e = Bola::RestituicaoDoChao, fracaoDoAtrito = Bola::MomentoDeInercia / (1 + Bola::MomentoDeInercia);
        float vh0 = rapidez * Cos(angulo), vz0 = rapidez * Sin(angulo);
        float vmax = std::sqrt(rapidez * rapidez + 2 * g * z0);
        float vzMin = MinF(vz0, g / (k * vmax));
        float queda = z0 / vzMin;
        float vhMin = vh0 * Exp(-k * vmax * queda);
        float assintota = g / (k * vhMin);
        float vzNoQuique = vz0 < assintota ? assintota + (vz0 - assintota) * Exp(-k * vhMin * queda) : vz0;
        float inclinacao0 = vz0 / vh0, umSubPasso = vh0 * Bola::PassoMaximo;
        auto Raiz = [&](float curvatura) { return (-inclinacao0 + std::sqrt(inclinacao0 * inclinacao0 + 4 * curvatura * z0)) / (2 * curvatura); };
        float quicaAte = Raiz(g / (2 * vh0 * vh0)) + umSubPasso;
        float quicaDepoisDe = Raiz(g / (2 * vhMin * vhMin)) - umSubPasso;
        float vzDepois = e * vzNoQuique;
        float vhDepoisMin = (1 - fracaoDoAtrito) * vhMin;
        float vhDepoisMax = MaxF((1 - fracaoDoAtrito) * vh0, vh0 - Bola::AtritoDoChao * (1 + e) * vzMin);
        float apice = vzDepois * vzDepois / (2 * g);
        float vDepois = std::sqrt(vhDepoisMax * vhDepoisMax + vzDepois * vzDepois);
        return {quicaDepoisDe, quicaAte, apice, vzDepois / vhDepoisMin, vhDepoisMax + Bola::KMagnus() * 0.5f * vDepois * apice, umSubPasso};
    }

    /// False só se é certo que a bola, nessa reta, não quica entre a rede e a parede de saída nem passa acima do topo dela.
    bool PodeValer(const Reta& reta) const
    {
        float folga = UmSubPasso + 0.02f;
        if (QuicaAte < reta.AteARede - folga || QuicaDepoisDe > reta.AteASaida + folga) return false;
        return reta.PodeSairBaixo || AlturaMaximaNaParede(reta.AteASaida) >= reta.AlturaMinima - FolgaNaAltura;
    }

    /// A altura máxima da bola ao chegar à parede de saída, ateAParede metros (na reta) depois do ponto de partida.
    float AlturaMaximaNaParede(float ateAParede) const
    {
        float depoisDoQuique = MaxF(0.f, ateAParede - QuicaAte);
        float curvatura = Bola::G / (2 * HorizontalMaxima * HorizontalMaxima);
        float topo = Inclinacao / (2 * curvatura);   // onde a parábola-limite para de subir
        float naParede = depoisDoQuique >= topo
            ? Inclinacao * depoisDoQuique - curvatura * depoisDoQuique * depoisDoQuique
            : Inclinacao * topo - curvatura * topo * topo;
        return MinF(Apice, naParede);
    }
};

/// A grade de sempre — rapidez de 22 a 40 m/s, os azimutes dados em ordem de preferência, elevação de -8° a -40°;
/// remate plano, sem efeito — e a escolha de sempre: a primeira que vale e aguenta as margens, na ordem rapidez →
/// azimute → elevação. Só simula de verdade a candidata que os cortes exatos (Reta, NoQuique, Perfil) não descartam.
std::optional<Golpe> Remate(float x0, float y0, float z0, int32_t ladoDoAlvo, TipoDeGolpe tipo, const std::vector<float>& azimutes,
                            int32_t lateralDaSaida, const std::function<bool(const EventoDaBola&)>& saidaCerta)
{
    const Predicado Vale = [&](const Marcas& m) {
        return m.size() >= 3
            && m[0].Evento.Tipo == TipoDeEventoDaBola::CruzouRede && m[0].Evento.Lado == ladoDoAlvo
            && m[1].Evento.Tipo == TipoDeEventoDaBola::Quique && m[1].Evento.Lado == ladoDoAlvo
            && m[2].Evento.Tipo == TipoDeEventoDaBola::Saiu && saidaCerta(m[2].Evento);
    };
    const Predicado Prefixo = [&](const Marcas& m) {
        return m[0].Evento.Tipo == TipoDeEventoDaBola::CruzouRede && m[0].Evento.Lado == ladoDoAlvo
            && (m.size() < 2 || (m[1].Evento.Tipo == TipoDeEventoDaBola::Quique && m[1].Evento.Lado == ladoDoAlvo))
            && (m.size() < 3 || (m[2].Evento.Tipo == TipoDeEventoDaBola::Saiu && saidaCerta(m[2].Evento)));
    };
    // Exatamente as contas da varredura antiga (22 + 2·nível e -8 - 2·k são exatos em float).
    auto ElevacaoDe = [](int32_t k) { return static_cast<float>(-8 - 2 * k) * Pi / 180.f; };
    auto Lancamento = [&](int32_t nivel, int32_t azimute, int32_t k) {
        float rapidez = static_cast<float>(22 + 2 * nivel);
        float elevacao = ElevacaoDe(k);
        float horizontal = rapidez * Cos(elevacao);
        float a = azimutes[static_cast<size_t>(azimute)];
        return Velocidade{horizontal * Sin(a), ladoDoAlvo * horizontal * Cos(a), rapidez * Sin(elevacao), 0};
    };

    // A reta de cada azimute e, por ela, a elevação mais funda que ainda passa a rede (-1: nenhuma). Fora da quadra as
    // contas da reta não valem: sem corte, tudo se simula.
    const int32_t quantosAzimutes = static_cast<int32_t>(azimutes.size());
    bool naCaixa = std::fabs(x0) < quadra::MeiaLargura && std::fabs(y0) < quadra::MeioComprimento && z0 > 0;
    std::vector<std::optional<Reta>> retas(azimutes.size());
    std::vector<int32_t> maisFunda(azimutes.size());
    bool temPorta = false;
    for (int32_t a = 0; a < quantosAzimutes; a++)
    {
        maisFunda[a] = naCaixa ? -1 : Elevacoes - 1;
        if (!naCaixa) continue;
        std::optional<Reta> reta = Reta::Do(x0, y0, ladoDoAlvo, azimutes[a], lateralDaSaida);
        if (!reta) continue;
        retas[a] = reta;
        temPorta = temPorta || reta->PodeSairBaixo;
        for (int32_t k = 0; k < Elevacoes; k++)
        {
            float e = ElevacaoDe(k);
            if (z0 + reta->AteARede * Sin(e) / Cos(e) < quadra::AlturaDaRede - FolgaNaRede) break;   // a reta do lançamento na rede
            maisFunda[a] = k;
        }
    }

    // Perfis por (nível, elevação), feitos quando um azimute precisa e estendidos até a parede de saída de quem pede.
    std::vector<std::unique_ptr<Perfil>> perfis(static_cast<size_t>(NiveisDeRapidez * Elevacoes));
    auto PerfilDe = [&](int32_t nivel, int32_t k) -> Perfil& {
        std::unique_ptr<Perfil>& pronto = perfis[static_cast<size_t>(nivel * Elevacoes + k)];
        if (pronto) return *pronto;
        float rapidez = static_cast<float>(22 + 2 * nivel), elevacao = ElevacaoDe(k);
        pronto = std::make_unique<Perfil>(rapidez * Cos(elevacao), rapidez * Sin(elevacao), z0, temPorta);
        return *pronto;
    };
    NoQuique noQuique[Elevacoes] = {};
    for (int32_t nivel = 0; nivel < NiveisDeRapidez; nivel++)
    {
        if (naCaixa)
            for (int32_t k = 0; k < Elevacoes; k++) noQuique[k] = NoQuique::Limites(static_cast<float>(22 + 2 * nivel), -ElevacaoDe(k), z0);
        for (int32_t a = 0; a < quantosAzimutes; a++)
            for (int32_t k = 0; k <= maisFunda[a]; k++)
            {
                if (retas[a] && (!noQuique[k].PodeValer(*retas[a]) || !PerfilDe(nivel, k).PodeValer(*retas[a]))) continue;
                const Velocidade v = Lancamento(nivel, a, k);
                Marcas m = Simular(x0, y0, z0, v, 2.5f, 3, Passo, Prefixo);
                if (!Vale(m) || !Robusta(x0, y0, z0, v, 2.5f, 3, DePotencia, Vale, Prefixo)) continue;
                const Marca& quique = m[1];
                return NovoGolpe(quique.Evento.X, quique.Evento.Y, quique.T, tipo, Efeito{}, v);
            }
    }
    return std::nullopt;
}

std::optional<Golpe> ResolverSmashPor4(float x0, float y0, float z0, int32_t ladoDoAlvo, float alvoX)
{
    float distanciaAoFundo = MaxF(1.f, quadra::MeioComprimento + std::fabs(y0));
    float preferido = Atan2(alvoX - x0, distanciaAoFundo);
    std::vector<float> azimutes;
    for (float graus : {0.f, -8.f, 8.f, -16.f, 16.f, -24.f, 24.f}) azimutes.push_back(preferido + graus * Pi / 180.f);
    return Remate(x0, y0, z0, ladoDoAlvo, TipoDeGolpe::SmashPor4, azimutes, 0, [&](const EventoDaBola& saiu) {
        return quadra::LadoDe(saiu.Y) == ladoDoAlvo && std::fabs(saiu.Y) >= quadra::MeioComprimento && std::fabs(saiu.X) < quadra::MeiaLargura;
    });
}

std::optional<Golpe> ResolverSmashPor3(float x0, float y0, float z0, int32_t ladoDoAlvo, int32_t ladoDaSaida)
{
    int32_t sinal = ladoDaSaida >= 0 ? 1 : -1;
    std::vector<float> azimutes;
    for (float graus = 10; graus <= 60; graus += 5) azimutes.push_back(sinal * graus * Pi / 180.f);
    return Remate(x0, y0, z0, ladoDoAlvo, TipoDeGolpe::SmashPor3, azimutes, sinal,
                  [&](const EventoDaBola& saiu) { return saiu.X * sinal >= quadra::MeiaLargura; });
}
} // namespace

Velocidade VelocidadeDe(const Golpe& golpe, float x0, float y0, float z0)
{
    if (golpe.Lancamento) return *golpe.Lancamento;
    return golpes::Calcular(x0, y0, z0, golpe.AlvoX, golpe.AlvoY, golpe.TempoDeVoo, 0.3f, golpe.IgnorarRede, golpe.Efeito);
}

/// Chiquita: bola baixa e lenta nos pés de quem está na rede — cai a até AlcanceDaChiquita da rede do lado do alvo,
/// passa a no máximo FolgaDaChiquitaSobreARede acima da rede, com topspin. Entre as que servem, a mais lenta.
std::optional<Golpe> Chiquita(float x0, float y0, float z0, int32_t ladoDoAlvo, float alvoX)
{
    return ResolverChiquita(x0, y0, z0, ladoDoAlvo, alvoX);
}

/// Contrapared: bate-se no próprio vidro de fundo pra bola voltar por cima, cruzar a rede e quicar do outro lado.
std::optional<Golpe> Contrapared(float x0, float y0, float z0, int32_t ladoDoAlvo, float alvoX)
{
    return ResolverContrapared(x0, y0, z0, ladoDoAlvo, alvoX);
}

/// Remate por 4: quica do outro lado e sai por cima da parede de fundo de lá. A mais lenta; empatando, a mais perto de alvoX.
std::optional<Golpe> SmashPor4(float x0, float y0, float z0, int32_t ladoDoAlvo, float alvoX)
{
    return ResolverSmashPor4(x0, y0, z0, ladoDoAlvo, alvoX);
}

/// Remate por 3: quica do outro lado e sai pela lateral do lado ladoDaSaida (sinal de x no mundo). A mais lenta;
/// empatando, a menos cruzada.
std::optional<Golpe> SmashPor3(float x0, float y0, float z0, int32_t ladoDoAlvo, int32_t ladoDaSaida)
{
    return ResolverSmashPor3(x0, y0, z0, ladoDoAlvo, ladoDaSaida);
}
} // namespace padel::golpes_especiais
