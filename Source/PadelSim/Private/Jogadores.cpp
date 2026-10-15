// Porte linha a linha de Padel.Core/Jogadores.cs (Jogador e IA). Min/Max/Limitar com a semântica do .NET (mat::).
#include "PadelSim/Jogadores.h"

#include "PadelSim/Arbitro.h"
#include "PadelSim/GolpesEspeciais.h"
#include "PadelSim/Matematica.h"
#include "PadelSim/Partida.h"
#include "PadelSim/Quadra.h"

#include <cmath>
#include <limits>

namespace padel
{
namespace
{
constexpr float PesoDaBolaNoCorpo = 0.35f, PesoDoReves = 0.1f, PesoDoRevesAlto = 0.45f;
/// Até onde o corpo vai na largura (o resto da quadra é o braço).
constexpr float LimiteLateral = 4.7f;
/// Abaixo disso (m/s, no chão, relativa ao corpo) a bola quase cai na vertical: o atraso vai a zero junto.
constexpr float RapidezPlenaDoAtraso = 1.f;
} // namespace

const PerfilDeIA& perfis::Por(Dificuldade d)
{
    switch (d)
    {
    case Dificuldade::Facil: return Facil;
    case Dificuldade::Dificil: return Dificil;
    default: return Medio;
    }
}

// ───────────── Jogador ─────────────

Jogador::Jogador(int32_t time, int32_t indice, std::string nome, bool humano, float velocidade)
    : Time(time), Indice(indice), Nome(std::move(nome)), Humano(humano), Velocidade(velocidade), Lado(quadra::LadoDoTime(time)),
      Alcance(humano ? 1.35f : 1.1f)   // braço + raquete; o humano ganha folga de propósito
{
    X = XDaMetade();
    Y = Lado * 6.f;
}

float Jogador::XDaMetade() const { return Lado * (Indice == 0 ? 2.5f : -2.5f); }
float Jogador::Rapidez() const { return std::sqrt(Vx * Vx + Vy * Vy); }
float Jogador::LateralDe(float x) const { return (x - X) * Lado; }
float Jogador::FrontalDe(float y) const { return (Y - y) * Lado; }
LadoDoGolpe Jogador::LadoDoGolpePara(float x) const { return LateralDe(x) * LadoDaRaquete() >= 0 ? LadoDoGolpe::Drive : LadoDoGolpe::Reves; }

std::pair<float, float> Jogador::PontoDeContato(LadoDoGolpe lado) const
{
    int32_t paraOLado = lado == LadoDoGolpe::Drive ? LadoDaRaquete() : -LadoDaRaquete();
    return {X + paraOLado * Lado * DistanciaIdealDoContato, Y};
}

float Jogador::DistanciaAoPontoIdeal(float x, float y) const { return DistanciaAoPontoIdeal(X, Y, x, y); }

float Jogador::AtrasoDoContato(const Bola& bola) const
{
    auto [px, py] = PontoDeContato(LadoDoGolpePara(bola));
    float vx = bola.Vx - Vx, vy = bola.Vy - Vy;
    float v = std::sqrt(vx * vx + vy * vy);
    if (v < 1e-3f) return 0;
    float passou = ((bola.X - px) * vx + (bola.Y - py) * vy) / v;
    return mat::Max(0, passou) * mat::Min(1, v / RapidezPlenaDoAtraso);
}

float Jogador::DistanciaAoPontoIdeal(float corpoX, float corpoY, float x, float y) const
{
    float foraDaLinha = DistanciaIdealDoContato - std::fabs((x - corpoX) * Lado), frontal = (corpoY - y) * Lado;
    return std::sqrt(foraDaLinha * foraDaLinha + frontal * frontal);
}

float Jogador::XParaBater(float xDaBola, bool bolaAlta) const { return XParaBater(xDaBola, LadoParaBater(xDaBola, bolaAlta)); }

float Jogador::XParaBater(float xDaBola, LadoDoGolpe lado) const
{
    return xDaBola - (lado == LadoDoGolpe::Drive ? 1 : -1) * LadoDaRaquete() * Lado * DistanciaIdealDoContato;
}

LadoDoGolpe Jogador::LadoParaBater(float xDaBola, bool bolaAlta) const
{
    float xDrive = XParaBater(xDaBola, LadoDoGolpe::Drive), xReves = XParaBater(xDaBola, LadoDoGolpe::Reves);
    bool driveCabe = std::fabs(xDrive) <= LimiteLateral, revesCabe = std::fabs(xReves) <= LimiteLateral;
    if (driveCabe != revesCabe) return driveCabe ? LadoDoGolpe::Drive : LadoDoGolpe::Reves;
    float preferenciaPeloDrive = bolaAlta ? 1.2f : 0.4f;   // metros a mais que ele anda pra não bater de revés
    return std::fabs(xDrive - X) <= std::fabs(xReves - X) + preferenciaPeloDrive ? LadoDoGolpe::Drive : LadoDoGolpe::Reves;
}

bool Jogador::BolaAtrasJuntoAoVidro(const Bola& bola) const
{
    return quadra::LadoDe(bola.Y) == Lado && FrontalDe(bola.Y) < 0 && quadra::MeioComprimento - std::fabs(bola.Y) <= DistanciaDoVidroDaContrapared;
}

void Jogador::Limitar()
{
    float x = mat::Limitar(X, -LimiteLateral, LimiteLateral);
    float y = Lado > 0 ? mat::Limitar(Y, 0.5f, 9.7f) : mat::Limitar(Y, -9.7f, -0.5f);
    if (x != X) { X = x; Vx = 0; }
    if (y != Y) { Y = y; Vy = 0; }
}

void Jogador::Mover(float dx, float dy, float dt)
{
    float n = std::sqrt(dx * dx + dy * dy);
    float desejadoX = 0, desejadoY = 0;
    if (n > 1e-6f)
    {
        float forca = mat::Min(1, n);
        desejadoX = dx / n * forca * Velocidade;
        desejadoY = dy / n * forca * Velocidade;
    }
    float ddx = desejadoX - Vx, ddy = desejadoY - Vy;
    float dd = std::sqrt(ddx * ddx + ddy * ddy);
    if (dd > 1e-6f)
    {
        bool acelerando = desejadoX * desejadoX + desejadoY * desejadoY > Vx * Vx + Vy * Vy;
        float passo = (acelerando ? Aceleracao : Frenagem) * dt;
        if (dd <= passo) { Vx = desejadoX; Vy = desejadoY; }
        else { Vx += ddx / dd * passo; Vy += ddy / dd * passo; }
    }
    X += Vx * dt;
    Y += Vy * dt;
    Limitar();
}

bool Jogador::IrPara(float x, float y, float dt)
{
    float dx = x - X, dy = y - Y;
    float d = std::sqrt(dx * dx + dy * dy);
    if (d < 0.05f) { Mover(0, 0, dt); return true; }
    float vDesejada = mat::Min(Velocidade, std::sqrt(2 * Frenagem * d));
    float f = vDesejada / Velocidade;
    Mover(dx / d * f, dy / d * f, dt);
    return d < 0.1f;
}

float Jogador::TempoParaChegar(float d) const
{
    float tAcel = Velocidade / Aceleracao;
    float dAcel = 0.5f * Aceleracao * tAcel * tAcel;
    return d <= dAcel ? std::sqrt(2 * d / Aceleracao) : tAcel + (d - dAcel) / Velocidade;
}

float Jogador::DistanciaAte(float x, float y) const { return mat::Distancia(X, Y, x, y); }

bool Jogador::Alcanca(const Bola& bola) const { return DistanciaAte(bola.X, bola.Y) <= Alcance && bola.Z >= 0 && bola.Z <= AlturaMaxima; }
bool Jogador::AlcancaConfortavelmente(const Bola& bola) const
{
    return DistanciaAte(bola.X, bola.Y) <= AlcanceConfortavel && bola.Z >= 0 && bola.Z <= AlturaMaxima;
}

bool Jogador::PodeBaterAgora(const Bola& bola) const { return PodeBaterAgora(bola, PassoPadraoDaPrevisao, true); }

bool Jogador::PodeBaterAgora(const Bola& bola, float dt, bool jaQuicou) const
{
    if (!Alcanca(bola)) return false;
    float bx = bola.X + bola.Vx * dt, by = bola.Y + bola.Vy * dt, bz = bola.Z + bola.Vz * dt;
    float cx = X + Vx * dt, cy = Y + Vy * dt;
    bool encerraNoChao = bz < 0 && jaQuicou;   // o primeiro quique devolve a bola pra cima
    bool continuaAoAlcance = mat::Distancia(cx, cy, bx, by) <= Alcance && !encerraNoChao && bz <= AlturaMaxima;
    if (!continuaAoAlcance) return true;   // última chance
    if (bola.Z < AlturaDaBolaBaixa && bola.Vz > 0) return false;   // rente ao chão e subindo: espera ela subir
    if (jaQuicou && bola.Vz < 0 && bz < AlturaDaBolaBaixa) return true;   // descendo pro segundo chão: bate antes de ficar baixa
    return DistanciaAoPontoIdeal(cx, cy, bx, by) >= DistanciaAoPontoIdeal(bola.X, bola.Y);
}

void Jogador::IniciarBalanco(IntencaoDoGolpe intencao)
{
    balanco = DuracaoDoBalanco;
    tempoNoBalanco = 0;
    intencaoDoBalanco = intencao;
}

void Jogador::AvancarTempo(float dt)
{
    Cooldown = mat::Max(0, Cooldown - dt);
    balancoTerminouNoAr = false;
    noInstanteDoContato = false;
    if (balanco > 0)
    {
        float antes = tempoNoBalanco;
        balanco = mat::Max(0, balanco - dt);
        tempoNoBalanco += dt;
        // O passo mais perto do momento ideal (a 120 Hz, o 14º: 0,1167 s): o que cruza o momento menos meio passo.
        float meioPasso = dt / 2;
        noInstanteDoContato = antes + meioPasso < MomentoIdealDoBalanco && tempoNoBalanco + meioPasso >= MomentoIdealDoBalanco;
        if (balanco == 0) balancoTerminouNoAr = true;
    }
}

float Jogador::DificuldadeDoGolpe(const Bola& bola) const
{
    float esticado = mat::Limitar((DistanciaAte(bola.X, bola.Y) - DistanciaIdealDoContato) / (Alcance - DistanciaIdealDoContato), 0, 1);
    float lateral = std::fabs(LateralDe(bola.X));
    float noCorpo = lateral < DistanciaDoCorpo ? PesoDaBolaNoCorpo * (1 - lateral / DistanciaDoCorpo) : 0;
    float reves = LadoDoGolpePara(bola) == LadoDoGolpe::Reves ? (bola.Z > AlturaDoRevesAlto ? PesoDoRevesAlto : PesoDoReves) : 0;
    return mat::Limitar(esticado * 0.8f + (bola.Z < AlturaDaBolaBaixa ? 0.4f : 0) + (bola.Rapidez() > 18 ? 0.4f : 0) + noCorpo + reves, 0, 1.6f);
}

// ───────────── IA ─────────────

namespace
{
/// Com a bola acima disso e o jogador a menos de DistanciaDaRedeDoRemateParaFora da rede, a IA pode tentar o por 3 / por 4.
constexpr float AlturaDoRemateParaFora = 2.2f, DistanciaDaRedeDoRemateParaFora = 4.f;
/// Com os dois adversários na rede e a bola baixa no fundo, metade das vezes chiquita.
constexpr float ChanceDeChiquita = 0.5f;
/// Até onde o corpo vai na largura (o mesmo limite do Jogador).
constexpr float LimiteLateralDoCorpo = 4.7f;
/// O meio de cada metade da largura: onde fica o parceiro de um humano.
constexpr float MeioDaMetade = 2.5f;
/// A formação do parceiro humano: a menos de ZonaDeRede da rede ele subiu; a mais de LinhaDaFrente ele voltou.
constexpr float ZonaDeRede = 4.5f, LinhaDaFrente = 5.5f;

Jogador* ParceiroDe(const std::array<Jogador*, 2>& meus, const Jogador& j) { return meus[0] == &j ? meus[1] : meus[0]; }
} // namespace

IA::IA(int32_t time, const PerfilDeIA& perfil, Aleatorio& a) : Time(time), Perfil(perfil), aleatorio(&a) {}

std::optional<int32_t> IA::Responsavel() const
{
    if (plano && plano->Responsavel) return plano->Responsavel->Time * 2 + plano->Responsavel->Indice;
    return std::nullopt;
}

void IA::AoSacar(Partida& partida, int32_t timeSacador)
{
    Posicao = timeSacador == Time ? Formacao::Rede : Formacao::Fundo;   // quem saca sobe; quem recebe fica atrás
    // Ponto novo: os dois sabem onde a dupla fica, e cada um joga do lado em que começou.
    for (int i = 0; i < 2; i++) { formacao[i] = Posicao; pendente[i].reset(); esperaOParceiro[i] = false; }
    auto meus = partida.JogadoresDoTime(Time);
    Jogador* indice0 = meus[0]->Indice == 0 ? meus[0] : meus[1];
    Jogador* indice1 = meus[0]->Indice == 0 ? meus[1] : meus[0];
    ladosTrocados = indice0->LateralDe(indice1->X) > 0;   // o 1 está à direita do 0
    xDaBolaLido = partida.Bola.X;
    for (Jogador* j : meus)
    {
        if (!j->Humano) continue;
        profundidadeDoHumanoLida = std::fabs(j->Y);
        humanoNaRede = profundidadeDoHumanoLida < ZonaDeRede;
    }
    Planejar(partida, true);
}

void IA::AoGolpear(Partida& partida, const Golpe& golpe, const Jogador& jogador)
{
    if (jogador.Time == Time)
    {
        bool naFrente = std::fabs(jogador.Y) < 5.5f;
        TipoDeGolpe t = golpe.Tipo;
        bool sobe = t == TipoDeGolpe::Lob || t == TipoDeGolpe::Smash || t == TipoDeGolpe::Bandeja || t == TipoDeGolpe::Vibora ||
                    t == TipoDeGolpe::Chiquita || t == TipoDeGolpe::SmashPor3 || t == TipoDeGolpe::SmashPor4;   // a chiquita é o passaporte pra rede
        MudarFormacao(partida, sobe || naFrente ? Formacao::Rede : Formacao::Fundo, jogador);
        esperaOParceiro[0] = esperaOParceiro[1] = true;
    }
    else
    {
        esperaOParceiro[0] = esperaOParceiro[1] = false;
    }
    Planejar(partida, true);
}

/// A dupla muda (ou confirma) a formação: quem mandou segue na hora; o outro, depois de AtrasoDaFormacao — ou, com
/// ChanceDeDesencontro, nem percebe. No time de um humano os dois seguem na hora, sem sorteio (vale se ele virar IA).
void IA::MudarFormacao(Partida& partida, Formacao nova, const Jogador& quem)
{
    Posicao = nova;
    auto meus = partida.JogadoresDoTime(Time);
    for (Jogador* j : meus)
    {
        int32_t i = j->Indice;
        if (j == &quem || formacao[i] == nova || j->Humano || ParceiroDe(meus, *j)->Humano) { formacao[i] = nova; pendente[i].reset(); continue; }
        if (pendente[i] == nova) continue;
        if (Perfil.ChanceDeDesencontro > 0 && aleatorio->Proximo() < Perfil.ChanceDeDesencontro) { pendente[i].reset(); continue; }
        pendente[i] = nova;
        mudaEm[i] = relogio + Perfil.AtrasoDaFormacao;
    }
}

void IA::AoRebater(Partida& partida)
{
    if (plano) Planejar(partida, false);
}

void IA::Planejar(Partida& partida, bool novaTrajetoria)
{
    const Bola& bola = partida.Bola;
    const Arbitro& arbitro = partida.Arbitro;
    std::optional<Plano> planoAnterior = plano;
    plano.reset();
    if (!bola.EmJogo || arbitro.Golpeador == Time) return;
    auto candidatos = Prever(bola, arbitro);
    if (candidatos.empty()) return;
    auto meus = partida.JogadoresDoTime(Time);

    Jogador* escolhido = nullptr;
    Candidato alvo{};
    for (const auto& c : candidatos)
    {
        Jogador* melhor = nullptr;
        float melhorFolga = -std::numeric_limits<float>::infinity();
        for (Jogador* j : meus)
        {
            float distancia = j->DistanciaAte(c.X, c.Y);
            if (c.Voleio && !(std::fabs(j->Y) < 5 && distancia < 2.5f)) continue;
            float folga = c.T - (j->TempoParaChegar(distancia) + (j->Humano ? 0 : Perfil.Reacao));
            if (folga >= 0 && folga > melhorFolga) { melhorFolga = folga; melhor = j; }
        }
        if (melhor) { escolhido = melhor; alvo = c; break; }
    }
    if (!escolhido)
    {
        // Ninguém chega a tempo: o mais perto do último ponto possível tenta mesmo assim.
        alvo = candidatos.back();
        float menor = std::numeric_limits<float>::infinity();
        for (Jogador* j : meus)
        {
            float d = j->DistanciaAte(alvo.X, alvo.Y);
            if (d < menor) { menor = d; escolhido = j; }
        }
    }
    if (!escolhido) return;   // só se as distâncias vierem NaN

    float sigma = ErroDeLeitura(alvo.T);
    int32_t meuLado = quadra::LadoDoTime(Time);
    float ruidoX = aleatorio->Gaussiana();
    float xLido = mat::Limitar(alvo.X + ruidoX * sigma, -4.7f, 4.7f);
    Plano novo;
    novo.Responsavel = escolhido;
    novo.Lado = escolhido->LadoParaBater(xLido, alvo.Z > Jogador::AlturaDoRevesAlto);
    novo.AlvoY = meuLado * mat::Limitar(std::fabs(alvo.Y) + aleatorio->Gaussiana() * sigma * 0.7f, 0.5f, 9.7f);
    novo.AlvoT = alvo.T;
    novo.CriadoEm = novaTrajetoria || !planoAnterior ? relogio : planoAnterior->CriadoEm;
    novo.PlanejadoEm = relogio;
    novo.XDaBola = alvo.X;
    novo.RuidoX = ruidoX;
    plano = novo;
    // A bola vai passar por cima da dupla na rede (quem vai nela vai buscá-la atrás da linha de saque): recuam juntos.
    if (Posicao == Formacao::Rede && std::fabs(plano->AlvoY) >= quadra::LinhaDeSaque) MudarFormacao(partida, Formacao::Fundo, *escolhido);
}

float IA::ErroDeLeitura(float segundosAteABola) const { return Perfil.ErroDePrevisao * mat::Limitar(segundosAteABola / 0.8f, 0, 1); }

float IA::XDoCorpo(const Plano& p, const Jogador& j) const
{
    float falta = p.AlvoT - (relogio - p.PlanejadoEm);
    float xLido = mat::Limitar(p.XDaBola + p.RuidoX * ErroDeLeitura(falta), -4.7f, 4.7f);
    return mat::Limitar(j.XParaBater(xLido, p.Lado), -4.7f, 4.7f);
}

std::vector<IA::Candidato> IA::Prever(const Bola& bola, const Arbitro& arbitro) const
{
    Bola clone = bola;
    int32_t meuLado = quadra::LadoDoTime(Time);
    constexpr float passo = 1.f / 60.f;
    std::vector<Candidato> candidatos;
    std::vector<EventoDaBola> eventos;
    int32_t quiques = 0;
    float t = 0;
    bool acabou = false;
    for (int i = 0; i < 240 && clone.EmJogo && !clone.Parada && !acabou; i++)
    {
        eventos.clear();
        clone.Avancar(passo, eventos);
        t += passo;
        for (const auto& e : eventos)
        {
            if (e.Tipo != TipoDeEventoDaBola::Quique) continue;
            if (e.Lado == meuLado) quiques += 1; else acabou = true;
            if (quiques >= 2) acabou = true;
        }
        if (acabou) break;
        if (quadra::LadoDe(clone.Y) != meuLado) continue;
        if (quiques == 0 && arbitro.EmSaque) continue;
        if (clone.Rolando) break;
        if (clone.Z < 0.35f || clone.Z > 2.3f) continue;
        if (quiques > 0 && clone.Vz > 0 && clone.Z < 0.9f) continue;   // subindo logo depois do quique: espera
        candidatos.push_back({clone.X, clone.Y, clone.Z, t, quiques == 0});
        if (candidatos.size() > 150) break;
    }
    return candidatos;
}

void IA::Reagir(float dt, Partida& partida)
{
    relogio += dt;
    auto meus = partida.JogadoresDoTime(Time);
    // O que a dupla acompanha anda atrasado de AtrasoDaFormacao (filtro de primeira ordem): o x da bola e a profundidade
    // do parceiro humano.
    float alfa = dt / (mat::Max(Perfil.AtrasoDaFormacao, dt) + dt);
    xDaBolaLido += (partida.Bola.X - xDaBolaLido) * alfa;
    for (Jogador* j : meus) if (j->Humano) profundidadeDoHumanoLida += (std::fabs(j->Y) - profundidadeDoHumanoLida) * alfa;
    for (int i = 0; i < 2; i++)
        if (pendente[i] && relogio >= mudaEm[i]) { formacao[i] = *pendente[i]; pendente[i].reset(); }

    for (Jogador* j : meus)
    {
        if (j->Humano) continue;
        if (plano && plano->Responsavel == j)
        {
            if (relogio - plano->CriadoEm < Perfil.Reacao) { j->Mover(0, 0, dt); continue; }
            j->IrPara(XDoCorpo(*plano, *j), plano->AlvoY, dt);
        }
        else
        {
            auto [x, profundidade] = LugarNaFormacao(*j, *ParceiroDe(meus, *j), plano);
            j->IrPara(x, j->Lado * profundidade, dt);
        }
    }
}

/// Onde o jogador fica na formação: x no mundo e profundidade (m da rede). O porquê de cada linha está no C#.
std::pair<float, float> IA::LugarNaFormacao(Jogador& j, const Jogador& parceiro, const std::optional<Plano>& p)
{
    int32_t i = j.Indice;
    float separacao = Perfil.SeparacaoDaDupla;
    float limiteDoCentro = mat::Max(0, LimiteLateralDoCorpo - separacao / 2);
    float centro = mat::Limitar(Perfil.AcompanhamentoDaBola * xDaBolaLido, -limiteDoCentro, limiteDoCentro);
    if (parceiro.Humano)
    {
        // O humano não segue a IA: a IA lê a formação dele (com histerese) e fica no meio da sua metade.
        if (profundidadeDoHumanoLida < ZonaDeRede) humanoNaRede = true;
        else if (profundidadeDoHumanoLida > LinhaDaFrente) humanoNaRede = false;
        CobrirOOutroLado(j, parceiro.X, 0);
        return {j.Lado * VagaDe(j) * MeioDaMetade, humanoNaRede ? ProfundidadeDaRede : ProfundidadeDoFundo};
    }

    float minha = formacao[i] == Formacao::Rede ? ProfundidadeDaRede : ProfundidadeDoFundo;
    float profundidade = minha;
    float folga = Perfil.FolgaDoBloco;
    if (p && p->Responsavel == &parceiro)
    {
        // O parceiro vai na bola: quem fica anda até a folga da profundidade dele e, se ele foi bem do lado de cá, troca.
        if (formacao[i] == Posicao && relogio - p->CriadoEm >= Perfil.AtrasoDaFormacao)
        {
            float dele = std::fabs(p->AlvoY);
            profundidade = mat::Limitar(mat::Limitar(minha, dele - folga, dele + folga), ProfundidadeDaRede, ProfundidadeMaximaDoBloco);
            // Um cambio por trajetória: a releitura (quique, parede) não desfaz a troca no meio do caminho.
            if (p->CriadoEm != cambioNaTrajetoria && CobrirOOutroLado(j, XDoCorpo(*p, parceiro), centro))
                cambioNaTrajetoria = p->CriadoEm;
        }
    }
    else if (esperaOParceiro[i] && formacao[i] == formacao[parceiro.Indice])
    {
        // Indo pra formação depois de um golpe da dupla: quem está mais à frente espera o outro chegar à folga, uma vez.
        float eu = std::fabs(j.Y);
        float ele = std::fabs(parceiro.Y) + parceiro.Vy * parceiro.Lado * j.Velocidade / (2 * j.Aceleracao);
        float frente = (eu - ele) * mat::Sinal(minha - eu);
        if (frente > folga) profundidade = eu;
        else esperaOParceiro[i] = false;
    }
    return {centro + j.Lado * VagaDe(j) * separacao / 2, profundidade};
}

/// +1 se o jogador está na vaga da direita da dupla (no referencial dela), -1 se na da esquerda.
int32_t IA::VagaDe(const Jogador& j) const { return (j.Indice == 0 ? 1 : -1) * (ladosTrocados ? -1 : 1); }

/// O "cambio": o parceiro (xDele, no mundo) invadiu a metade da quadra da vaga de j e passou do centro da dupla por mais
/// de um quarto da separação — e j troca de vaga, cobrindo o lado vazio. O quarto da separação é a histerese; a metade
/// conta porque o centro anda com a bola. Devolve se trocou.
bool IA::CobrirOOutroLado(const Jogador& j, float xDele, float centro)
{
    int32_t ladoDaMinhaVaga = j.Lado * VagaDe(j);   // no mundo
    if (xDele * ladoDaMinhaVaga <= 0 || (xDele - centro) * ladoDaMinhaVaga <= Perfil.SeparacaoDaDupla / 4) return false;
    ladosTrocados = !ladosTrocados;
    return true;
}

Golpe IA::EscolherGolpe(Jogador& jogador, const Bola& bola, Partida& partida)
{
    Golpe g = Escolher(jogador, bola, partida);
    g.Lado = jogador.LadoDoGolpePara(bola);
    return g;
}

Golpe IA::Escolher(Jogador& jogador, const Bola& bola, Partida& partida)
{
    Aleatorio& r = *aleatorio;
    int32_t ladoDoAlvo = -jogador.Lado;
    auto adversarios = partida.JogadoresDoTime(1 - Time);
    float dificuldade = jogador.DificuldadeDoGolpe(bola);
    auto Novo = [](float x, float y, float t, TipoDeGolpe tipo, bool ignorarRede = false, Efeito efeito = {}) {
        Golpe g;
        g.AlvoX = x; g.AlvoY = y; g.TempoDeVoo = t; g.Tipo = tipo; g.IgnorarRede = ignorarRede; g.Efeito = efeito;
        return g;
    };

    if (r.Proximo() < Perfil.ChanceDeErro * (1 + 2.5f * dificuldade))
    {
        // C#: os argumentos são avaliados da esquerda pra direita — o sorteio do x vem antes de montar o golpe.
        if (r.Proximo() < 0.5f) { float x = (r.Proximo() - 0.5f) * 6; return Novo(x, ladoDoAlvo * 0.3f, 0.45f, TipoDeGolpe::Erro, true); }      // na rede
        float x = (r.Proximo() - 0.5f) * 6;
        return Novo(x, ladoDoAlvo * 11.5f, 0.55f, TipoDeGolpe::Erro, true);   // no vidro sem quicar
    }
    // Último recurso: a bola passou, está junto ao próprio vidro e depois dele não volta batível — contrapared.
    if (jogador.BolaAtrasJuntoAoVidro(bola) && !VoltaBativel(bola, partida.Arbitro))
        if (auto contrapared = golpes_especiais::Contrapared(bola.X, bola.Y, bola.Z, ladoDoAlvo, 0)) return *contrapared;
    // Bola muito alta perto da rede: às vezes (pela dificuldade) tenta tirar a bola da quadra, se a física deixar.
    if (bola.Z > AlturaDoRemateParaFora && std::fabs(jogador.Y) < DistanciaDaRedeDoRemateParaFora && r.Proximo() < Perfil.ChanceDeRemateParaFora)
    {
        int32_t saidaCruzada = bola.X >= 0 ? -1 : 1;
        auto Por3 = [&] { return golpes_especiais::SmashPor3(bola.X, bola.Y, bola.Z, ladoDoAlvo, saidaCruzada); };
        auto Por4 = [&] { return golpes_especiais::SmashPor4(bola.X, bola.Y, bola.Z, ladoDoAlvo, bola.X); };
        std::optional<Golpe> remate;
        if (r.Proximo() < 0.5f) { remate = Por3(); if (!remate) remate = Por4(); }
        else { remate = Por4(); if (!remate) remate = Por3(); }
        if (remate) return *remate;
    }
    // Bola alta: smash bem na rede; senão bandeja (segura, funda, com slice) ou víbora (agressiva, com sidespin).
    if (bola.Z > 1.5f && std::fabs(jogador.Y) < 6.5f)
    {
        bool bemAlta = bola.Z > 2.0f && std::fabs(jogador.Y) < 4.5f;
        if (bemAlta && r.Proximo() < 0.6f)
        {
            float x = (r.Proximo() - 0.5f) * 7;
            float y = ladoDoAlvo * (3 + r.Proximo() * 3);
            return Novo(x, y, 0.4f, TipoDeGolpe::Smash, false, {1500, 0});
        }
        if (r.Proximo() < 0.35f)
        {
            float paraOLado = r.Proximo() < 0.5f ? -1.f : 1.f;
            return Novo(3.8f * paraOLado, ladoDoAlvo * 6.5f, 0.7f, TipoDeGolpe::Vibora, false, {-800, 1800 * paraOLado * ladoDoAlvo});
        }
        float x = (r.Proximo() - 0.5f) * 6;
        return Novo(x, ladoDoAlvo * 7.5f, 0.95f, TipoDeGolpe::Bandeja, false, {-1500, 500});
    }
    int32_t adversariosNaRede = 0;
    for (Jogador* a : adversarios) if (std::fabs(a->Y) < 4.5f) adversariosNaRede++;
    // Os dois na rede, eu no fundo com a bola baixa: chiquita nos pés de um deles.
    if (adversariosNaRede == 2 && std::fabs(jogador.Y) > 6.5f && bola.Z < 1.0f && r.Proximo() < ChanceDeChiquita)
    {
        float alvoX = adversarios[r.Proximo() < 0.5f ? 0 : 1]->X;
        if (auto chiquita = golpes_especiais::Chiquita(bola.X, bola.Y, bola.Z, ladoDoAlvo, alvoX)) return *chiquita;
    }
    if (adversariosNaRede >= 1 && r.Proximo() < Perfil.ChanceDeLob)
    {
        float x = (r.Proximo() - 0.5f) * 5;
        return Novo(x, ladoDoAlvo * 8.2f, 1.6f, TipoDeGolpe::Lob, false, {-400, 0});
    }
    // Golpe de fundo pro buraco: o canto mais longe dos dois adversários.
    float melhorX = 0, melhorDistancia = -1;
    for (float x : {-3.3f, 0.f, 3.3f})
    {
        float d = mat::Min(adversarios[0]->DistanciaAte(x, ladoDoAlvo * 6.8f), adversarios[1]->DistanciaAte(x, ladoDoAlvo * 6.8f));
        if (d > melhorDistancia) { melhorDistancia = d; melhorX = x; }
    }
    float ruido = Perfil.ErroDeMira * (1 + 1.2f * dificuldade);
    float alvoX = mat::Limitar(melhorX + r.Gaussiana() * ruido, -4.5f, 4.5f);
    float alvoY = ladoDoAlvo * mat::Limitar(std::fabs(ladoDoAlvo * 6.8f + r.Gaussiana() * ruido * 0.6f), 2, 9.4f);
    return Novo(alvoX, alvoY, 0.8f, TipoDeGolpe::Normal, false, {1200, 0});
}

bool IA::VoltaBativel(const Bola& bola, const Arbitro& arbitro) const
{
    Bola clone = bola;
    int32_t meuLado = quadra::LadoDoTime(Time);
    int32_t quiquesQueRestam = arbitro.QuicouNoReceptor ? 0 : 1;
    bool bateuNoVidro = false;
    std::vector<EventoDaBola> eventos;
    for (int i = 0; i < 180 && clone.EmJogo && !clone.Parada && !clone.Rolando; i++)
    {
        eventos.clear();
        clone.Avancar(1.f / 60.f, eventos);
        for (const auto& e : eventos)
        {
            if (e.Tipo == TipoDeEventoDaBola::Saiu || e.Tipo == TipoDeEventoDaBola::CruzouRede) return false;
            if (e.Tipo == TipoDeEventoDaBola::Parede && e.Lado == meuLado) bateuNoVidro = true;
            if (e.Tipo == TipoDeEventoDaBola::Quique && e.Lado == meuLado && --quiquesQueRestam < 0) return false;
        }
        bool vemPraRede = clone.Vy * meuLado < 0;
        if (bateuNoVidro && vemPraRede && clone.Z >= 0.35f && clone.Z <= 2.3f) return true;
    }
    return false;
}
} // namespace padel
