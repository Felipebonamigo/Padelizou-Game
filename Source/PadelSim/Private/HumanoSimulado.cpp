// Porte linha a linha de Padel.Core/HumanoSimulado.cs.
#include "PadelSim/HumanoSimulado.h"

#include "PadelSim/Matematica.h"
#include "PadelSim/Partida.h"
#include "PadelSim/Quadra.h"

#include <cmath>
#include <vector>

namespace padel
{
const PerfilDeHumano PerfilDeHumano::Iniciante{"Iniciante", 0.35f, 0.070f, 0.9f, 0.2f};
const PerfilDeHumano PerfilDeHumano::Intermediario{"Intermediário", 0.27f, 0.030f, 0.5f, 0.45f};
const PerfilDeHumano PerfilDeHumano::Avancado{"Avançado", 0.22f, 0.020f, 0.3f, 0.6f};
const PerfilDeHumano PerfilDeHumano::Profissional{"Profissional", 0.18f, 0.012f, 0.15f, 0.75f};

namespace
{
constexpr float PassoDaLeitura = 1.f / 60.f;
constexpr float MagnitudeDaMira = 0.45f;   // mira sem sair correndo: passa dos limiares (0,3) e anda devagar

float LimitarEntrada(float v) { return mat::Limitar(v, -1, 1); }

/// O primeiro mínimo da distância ao ponto ideal com a bola ao alcance: fecha quando ela sai do alcance ou já vai longe.
struct MaisPerto
{
    float distancia = 0;
    std::optional<float> Quando;
    bool Fechado = false;

    void Ver(float t, float d, bool aoAlcance)
    {
        if (Fechado) return;
        if (!aoAlcance) { Fechado = Quando.has_value(); return; }
        if (!Quando || d < distancia) { distancia = d; Quando = t; }
        else if (d > distancia + 0.3f) Fechado = true;   // passou do ponto mais perto e já vai longe
    }
};
} // namespace

EstadoVisivel EstadoVisivel::De(const Partida& p)
{
    const Bola& b = p.Bola;
    EstadoVisivel e{};
    e.Estado = p.Estado;
    auto s = p.Placar.SacadorAtual();
    e.IndiceDoSacador = s.Time * 2 + s.Jogador;
    e.CaixaDoSaque = p.CaixaDoSaque;
    e.BolaX = b.X; e.BolaY = b.Y; e.BolaZ = b.Z;
    e.BolaVx = b.Vx; e.BolaVy = b.Vy; e.BolaVz = b.Vz;
    e.BolaWx = b.Wx; e.BolaWy = b.Wy; e.BolaWz = b.Wz;
    e.BolaEmJogo = b.EmJogo;
    e.TimeDoUltimoGolpe = p.UltimoGolpe ? p.UltimoGolpe->Jogador / 2 : -1;
    if (p.UltimoGolpe) e.TipoDoUltimoGolpe = p.UltimoGolpe->Tipo;
    e.QuicouDepoisDoUltimoGolpe = p.Arbitro.QuicouNoReceptor;
    for (int i = 0; i < 4; i++)
    {
        const Jogador& j = p.Jogadores[i];
        e.Jogadores[i] = {j.X, j.Y, j.Vx, j.Vy, j.Time, j.Lado};
    }
    return e;
}

Bola EstadoVisivel::CopiaDaBola() const
{
    Bola bola;
    bola.Posicionar(BolaX, BolaY, BolaZ);
    bola.Lancar({BolaVx, BolaVy, BolaVz, 0, BolaWx, BolaWy, BolaWz});
    bola.EmJogo = BolaEmJogo;
    return bola;
}

HumanoSimulado::HumanoSimulado(int32_t indiceDoJogador, PerfilDeHumano perfil, padel::Aleatorio& a, bool destro)
    : Perfil(std::move(perfil)), indice(indiceDoJogador), aleatorio(&a),
      eu(indiceDoJogador / 2, indiceDoJogador % 2, Perfil.Nome, true, Jogador::VelocidadeDoHumano),
      corpoNoContato(indiceDoJogador / 2, indiceDoJogador % 2, Perfil.Nome, true, Jogador::VelocidadeDoHumano)
{
    eu.Destro = destro;
    corpoNoContato.Destro = destro;
}

Entrada HumanoSimulado::Decidir(const EstadoVisivel& estado, float dt)
{
    const JogadorVisivel& meu = estado.Jogadores[indice];
    eu.X = meu.X; eu.Y = meu.Y; eu.Vx = meu.Vx; eu.Vy = meu.Vy;
    desdeALeitura += dt;
    desdeOAperto += dt;   // soma os mesmos dt que o Jogador.TempoNoBalanco: o instante de contato cai no mesmo tick

    switch (estado.Estado)
    {
    case EstadoDaPartida::Saque:
    {
        apertouNestaBola = false;
        ultimaLeitura = {-2, std::nullopt, false};   // saque repetido depois de falta é bola nova
        if (estado.IndiceDoSacador != indice) { esperaDoSaque = -1; return {}; }
        if (esperaDoSaque < 0) esperaDoSaque = Perfil.TempoDeReacao + 0.4f + aleatorio->Proximo() * 0.6f;
        esperaDoSaque -= dt;
        if (esperaDoSaque > 0) return {};
        esperaDoSaque = -1;
        return {LimitarEntrada((aleatorio->Proximo() - 0.5f) * 1.2f), 0, true, true, false};
    }
    case EstadoDaPartida::Rally:
        return NoRally(estado, dt);
    default:
        esperaDoSaque = -1;
        return {};
    }
}

Entrada HumanoSimulado::NoRally(const EstadoVisivel& e, float dt)
{
    Leitura leitura{e.TimeDoUltimoGolpe, e.TipoDoUltimoGolpe, e.QuicouDepoisDoUltimoGolpe};
    if (leitura != ultimaLeitura)
    {
        bool novoGolpe = leitura.Time != ultimaLeitura.Time || leitura.Tipo != ultimaLeitura.Tipo;
        ultimaLeitura = leitura;
        if (novoGolpe)
        {
            // Bola nova: leitura nova com erro (a releitura no quique corrige parte dele).
            desdeALeitura = 0;
            apertouNestaBola = false;
            segurarAcao = false;
            erroX = aleatorio->Gaussiana() * Perfil.ErroDeLeitura;
            erroY = aleatorio->Gaussiana() * Perfil.ErroDeLeitura;
            erroDoTempo = aleatorio->Gaussiana() * Perfil.DesvioDoTempo;
        }
        else
        {
            erroX *= 0.35f; erroY *= 0.35f;
        }
    }

    bool bolaVemPraMim = e.TimeDoUltimoGolpe >= 0 && e.TimeDoUltimoGolpe != eu.Time && e.BolaEmJogo;
    if (!bolaVemPraMim) return Posicionar(e, e.TimeDoUltimoGolpe == eu.Time);

    auto alvo = PreverContato(e);
    if (!alvo) return Posicionar(e, false);
    float tx = alvo->X, ty = alvo->Y, tempo = alvo->T, zContato = alvo->Z;

    // Só quem está mais perto do ponto de contato vai buscar; o parceiro cobre.
    if (!SouQuemBusca(e, tx, ty)) return Posicionar(e, false);

    bool bolaAlta = zContato > 1.5f;
    float xCorpo = eu.XParaBater(tx + erroX, bolaAlta);
    float yCorpo = ty + erroY * 0.7f;
    float dxMundo = xCorpo - eu.X, dyMundo = yCorpo - eu.Y;
    float distancia = std::sqrt(dxMundo * dxMundo + dyMundo * dyMundo);

    // Timing: aperta quando faltar o momento ideal do balanço até a bola chegar ao ponto ideal, mais o erro de timing.
    bool podeReagir = desdeALeitura >= Perfil.TempoDeReacao;
    if (!apertouNestaBola)
    {
        float esperaPraReagir = mat::Max(0, Perfil.TempoDeReacao - desdeALeitura);
        auto ate = TempoAteOPontoIdeal(e, xCorpo, yCorpo, esperaPraReagir);
        float tempoAteOPonto = ate ? *ate : tempo;
        if (podeReagir && !AindaNaoEHoraDeApertar(tempoAteOPonto))
        {
            apertouNestaBola = true;
            desdeOAperto = 0;
            plano = EscolherGolpe(e, zContato);
            segurarAcao = plano == PlanoDeGolpe::RemateForte;
            bool lob = plano == PlanoDeGolpe::Lob || plano == PlanoDeGolpe::Chiquita;
            auto [dx, dy] = IrAte(xCorpo, yCorpo, distancia);
            return {dx, dy, !lob, segurarAcao, lob};
        }
    }
    if (apertouNestaBola)
    {
        // Balançando: o corpo segue indo pro lugar até o instante de contato; ali a direção é a mira.
        if (desdeOAperto + 1.5f * dt >= Jogador::MomentoIdealDoBalanco)
        {
            auto [mx, my] = Mira(e, plano);
            return {mx, my, false, segurarAcao, false};
        }
        auto [cx, cy] = IrAte(xCorpo, yCorpo, distancia);
        return {cx, cy, false, segurarAcao, false};
    }
    // Indo buscar: corre e freia pra chegar parado (se não reagiu ainda, fica onde está).
    if (!podeReagir) return {};
    auto [ix, iy] = IrAte(xCorpo, yCorpo, distancia);
    return {ix, iy, false, false, false};
}

bool HumanoSimulado::AindaNaoEHoraDeApertar(float segundosAteOPonto) const
{
    return segundosAteOPonto - Jogador::MomentoIdealDoBalanco + erroDoTempo > 0;
}

std::pair<float, float> HumanoSimulado::IrAte(float x, float y, float distancia) const
{
    if (distancia < 0.08f) return {0.f, 0.f};
    float velocidadeQueCabe = std::sqrt(2 * eu.Frenagem * distancia) / eu.Velocidade;
    float forca = mat::Min(1, velocidadeQueCabe);
    Entrada e = ParaEntrada((x - eu.X) / distancia * forca, (y - eu.Y) / distancia * forca, false, false);
    return {e.Dx, e.Dy};
}

std::optional<HumanoSimulado::Contato> HumanoSimulado::PreverContato(const EstadoVisivel& e) const
{
    Bola bola = e.CopiaDaBola();
    std::vector<EventoDaBola> eventos;
    eventos.reserve(4);
    bool precisaQuicar = e.TipoDoUltimoGolpe == TipoDeGolpe::Saque;   // não se voleia o saque
    bool naRede = std::fabs(eu.Y) < 4.5f;
    bool quicou = e.QuicouDepoisDoUltimoGolpe;
    float t = 0;
    std::optional<Contato> primeiroAlcancavel;
    for (int i = 0; i < 180 && bola.EmJogo && !bola.Parada; i++)
    {
        eventos.clear();
        bola.Avancar(PassoDaLeitura, eventos);
        t += PassoDaLeitura;
        for (const auto& ev : eventos)
        {
            if (ev.Tipo != TipoDeEventoDaBola::Quique) continue;
            if (ev.Lado != eu.Lado) return primeiroAlcancavel;   // quicou do outro lado: não é minha
            if (quicou) return primeiroAlcancavel;               // segundo quique: acabou
            quicou = true;
        }
        if (quadra::LadoDe(bola.Y) != eu.Lado || bola.Rolando) continue;
        if (!quicou && (precisaQuicar || !naRede)) continue;   // do fundo, espera quicar
        if (bola.Z < 0.4f || bola.Z > 2.4f) continue;
        if (quicou && bola.Vz > 0 && bola.Z < 0.9f) continue;   // subindo logo depois do quique: espera cair
        float d = mat::Distancia(eu.X, eu.Y, bola.X, bola.Y);
        if (eu.TempoParaChegar(mat::Max(0, d - Jogador::DistanciaIdealDoContato)) + Perfil.TempoDeReacao <= t + 0.15f)
            return Contato{bola.X, bola.Y, t, bola.Z};
        if (!primeiroAlcancavel) primeiroAlcancavel = Contato{bola.X, bola.Y, t, bola.Z};
    }
    return primeiroAlcancavel;
}

std::optional<float> HumanoSimulado::TempoAteOPontoIdeal(const EstadoVisivel& e, float x, float y, float esperaPraReagir)
{
    Jogador& corpo = corpoNoContato;
    corpo.X = eu.X; corpo.Y = eu.Y; corpo.Vx = eu.Vx; corpo.Vy = eu.Vy;
    Bola bola = e.CopiaDaBola();
    std::vector<EventoDaBola> eventos;
    eventos.reserve(4);
    bool saque = e.TipoDoUltimoGolpe == TipoDeGolpe::Saque;
    bool precisaQuicar = saque || std::fabs(y) >= 4.5f;
    bool quicou = e.QuicouDepoisDoUltimoGolpe;
    MaisPerto depoisDoQuique, voleio, qualquer;
    float t = 0;
    constexpr float passo = 1.f / 240.f;   // mais fino que a leitura: o timing se mede em centésimos
    for (int i = 0; i < 720 && bola.EmJogo && !bola.Parada && !depoisDoQuique.Fechado; i++)
    {
        eventos.clear();
        bola.Avancar(passo, eventos);
        t += passo;
        if (t <= esperaPraReagir) corpo.Mover(0, 0, passo); else corpo.IrPara(x, y, passo);
        bool acabou = false;
        for (const auto& ev : eventos)
            if (ev.Tipo == TipoDeEventoDaBola::Quique) { if (ev.Lado != eu.Lado || quicou) acabou = true; else quicou = true; }
        if (acabou) break;
        if (bola.Z < 0 || bola.Z > eu.AlturaMaxima || quadra::LadoDe(bola.Y) != eu.Lado) continue;
        float d = corpo.DistanciaAoPontoIdeal(bola.X, bola.Y);
        bool aoAlcance = corpo.DistanciaAte(bola.X, bola.Y) <= corpo.Alcance;
        if (quicou || !precisaQuicar) depoisDoQuique.Ver(t, d, aoAlcance);
        else if (!saque) voleio.Ver(t, d, aoAlcance);
        if (quicou || !saque) qualquer.Ver(t, d, true);
        // Parada antecipada, com a mesma decisão: cada Quando só anda pra frente, e amostra nova vem depois de t.
        if (depoisDoQuique.Quando && AindaNaoEHoraDeApertar(*depoisDoQuique.Quando)) return *depoisDoQuique.Quando;
        if (!depoisDoQuique.Quando && AindaNaoEHoraDeApertar(t) && qualquer.Quando && AindaNaoEHoraDeApertar(*qualquer.Quando) &&
            (!voleio.Quando || AindaNaoEHoraDeApertar(*voleio.Quando)))
            return t;
    }
    if (depoisDoQuique.Quando) return depoisDoQuique.Quando;
    if (voleio.Quando) return voleio.Quando;
    return qualquer.Quando;
}

bool HumanoSimulado::SouQuemBusca(const EstadoVisivel& e, float x, float y) const
{
    const JogadorVisivel& p = e.Jogadores[indice ^ 1];
    float euD = mat::Distancia(eu.X, eu.Y, x, y), ele = mat::Distancia(p.X, p.Y, x, y);
    return euD <= ele + 0.3f;   // empate: vou eu (o humano não confia no parceiro)
}

HumanoSimulado::PlanoDeGolpe HumanoSimulado::EscolherGolpe(const EstadoVisivel& e, float zContato)
{
    Aleatorio& r = *aleatorio;
    float a = Perfil.Agressividade;
    bool naRede = std::fabs(eu.Y) < 4.5f;
    int32_t adversariosNaRede = 0;
    for (const auto& j : e.Jogadores) if (j.Time != eu.Time && std::fabs(j.Y) < 4.5f) adversariosNaRede++;

    if (zContato > 1.5f)
    {
        if (naRede && zContato > 2.2f && r.Proximo() < a * 0.4f) return PlanoDeGolpe::RemateForte;
        if (naRede && r.Proximo() < a) return PlanoDeGolpe::Remate;
        return r.Proximo() < 0.3f * a ? PlanoDeGolpe::Vibora : PlanoDeGolpe::Bandeja;
    }
    if (adversariosNaRede == 2 && !naRede)
    {
        float s = r.Proximo();
        if (s < 0.35f) return PlanoDeGolpe::Lob;
        if (s < 0.35f + 0.25f * a) return PlanoDeGolpe::Chiquita;
    }
    if (naRede && r.Proximo() < a) return PlanoDeGolpe::Ataque;
    if (!naRede && r.Proximo() < 0.25f * (1 - a)) return PlanoDeGolpe::Defesa;
    return PlanoDeGolpe::Normal;
}

std::pair<float, float> HumanoSimulado::Mira(const EstadoVisivel& e, PlanoDeGolpe p) const
{
    // Canto: o lado da quadra adversária mais vazio (no referencial do jogador, direita = +).
    float somaX = 0;
    for (const auto& j : e.Jogadores) if (j.Time != eu.Time) somaX += j.X;
    float vazioNoMundo = somaX > 0 ? -1.f : 1.f;
    float canto = vazioNoMundo * eu.Lado * MagnitudeDaMira;
    switch (p)
    {
    case PlanoDeGolpe::Ataque:
    case PlanoDeGolpe::Remate:
    case PlanoDeGolpe::RemateForte:
    case PlanoDeGolpe::Chiquita: return {canto, -MagnitudeDaMira};
    case PlanoDeGolpe::Defesa: return {canto, MagnitudeDaMira};
    case PlanoDeGolpe::Vibora: return {mat::Sinal(canto) * 0.8f, 0.f};
    case PlanoDeGolpe::Bandeja:
    case PlanoDeGolpe::Lob: return {canto * 0.5f, 0.f};
    default: return {canto, 0.f};
    }
}

Entrada HumanoSimulado::Posicionar(const EstadoVisivel& e, bool atacando)
{
    apertouNestaBola = false;
    bool lobOuRemate = false;
    if (e.TipoDoUltimoGolpe)
    {
        TipoDeGolpe t = *e.TipoDoUltimoGolpe;
        lobOuRemate = t == TipoDeGolpe::Lob || t == TipoDeGolpe::Smash || t == TipoDeGolpe::Bandeja || t == TipoDeGolpe::Vibora || t == TipoDeGolpe::Chiquita;
    }
    float profundidade = atacando && (lobOuRemate || std::fabs(eu.Y) < 5.5f) ? 3.2f : 6.5f;
    float tx = eu.XDaMetade(), ty = eu.Lado * profundidade;
    float dx = tx - eu.X, dy = ty - eu.Y;
    float d = std::sqrt(dx * dx + dy * dy);
    if (d < 0.15f) return {};
    float forca = mat::Min(1, std::sqrt(2 * eu.Frenagem * d) / eu.Velocidade) * 0.8f;
    return ParaEntrada(dx / d * forca, dy / d * forca, false, false);
}

Entrada HumanoSimulado::ParaEntrada(float dxMundo, float dyMundo, bool apertou, bool segurando) const
{
    return {LimitarEntrada(dxMundo * eu.Lado), LimitarEntrada(dyMundo * eu.Lado), apertou, segurando, false};
}
} // namespace padel
