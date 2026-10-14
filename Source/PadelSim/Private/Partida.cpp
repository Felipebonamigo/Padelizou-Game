// Porte linha a linha de Padel.Core/Partida.cs. A ordem dos sorteios importa (o C# avalia argumentos da esquerda pra
// direita): cada Proximo()/Gaussiana() fica numa variável na ordem em que o C# o faria.
#include "PadelSim/Partida.h"

#include "PadelSim/Golpes.h"
#include "PadelSim/GolpesEspeciais.h"
#include "PadelSim/Matematica.h"
#include "PadelSim/Quadra.h"

#include <cmath>
#include <random>

namespace padel
{
namespace
{
/// Erro do golpe do humano = PesoDoCorpoNoErro × DificuldadeDoGolpe + PesoDoAtraso × AtrasoDoContato (por metro).
constexpr float PesoDoCorpoNoErro = 1.0f, PesoDoAtraso = 1.0f;
constexpr float AlturaDaBolaAlta = 1.5f;

uint32_t SementeAleatoria() { return std::random_device{}(); }

float Componente(float v) { return std::isfinite(v) ? mat::Limitar(v, -1, 1) : 0; }

Golpe NovoGolpe(float x, float y, float t, TipoDeGolpe tipo, bool ignorarRede = false, Efeito efeito = {})
{
    Golpe g;
    g.AlvoX = x; g.AlvoY = y; g.TempoDeVoo = t; g.Tipo = tipo; g.IgnorarRede = ignorarRede; g.Efeito = efeito;
    return g;
}

TipoDeEventoDaPartida TraduzirEvento(TipoDeEventoDaBola tipo)
{
    switch (tipo)
    {
    case TipoDeEventoDaBola::Quique: return TipoDeEventoDaPartida::Quique;
    case TipoDeEventoDaBola::Parede: return TipoDeEventoDaPartida::Parede;
    case TipoDeEventoDaBola::Rede: return TipoDeEventoDaPartida::Rede;
    case TipoDeEventoDaBola::CruzouRede: return TipoDeEventoDaPartida::CruzouRede;
    default: return TipoDeEventoDaPartida::Saiu;
    }
}
} // namespace

const char* Partida::TextoDoMotivo(padel::Motivo motivo)
{
    switch (motivo)
    {
    case padel::Motivo::DoisQuiques: return "dois quiques";
    case padel::Motivo::Rede: return "na rede";
    case padel::Motivo::NaoPassou: return "não passou";
    case padel::Motivo::ParedeSemQuicar: return "no vidro sem quicar";
    case padel::Motivo::ParedePropria: return "na própria parede";
    case padel::Motivo::Fora: return "pra fora da quadra";
    case padel::Motivo::VoltouPeloVidro: return "voltou pelo vidro";
    case padel::Motivo::DuplaFalta: return "dupla falta";
    case padel::Motivo::ForaDaCaixa: return "saque fora da caixa";
    case padel::Motivo::BolaMorta: return "bola morta";
    }
    return "";
}

Partida::Partida(OpcoesDaPartida opcoes)
    : Opcoes(opcoes), Aleatorio(opcoes.Semente ? *opcoes.Semente : SementeAleatoria()),
      Placar(opcoes.PontosCorridos ? padel::Placar::DePontosCorridos(*opcoes.PontosCorridos, 0) : padel::Placar(opcoes.PontoDeOuro, opcoes.SetsParaVencer, 0, opcoes.GamesPorSet, opcoes.PontosDoTieBreak))
{
    const PerfilDeIA& rival = perfis::Por(Opcoes.Dificuldade);
    const auto& h = Opcoes.Humanos;
    Jogadores.reserve(4);
    Jogadores.emplace_back(0, 0, "Você", h[0], h[0] ? Jogador::VelocidadeDoHumano : perfis::Parceiro.Velocidade);
    Jogadores.emplace_back(0, 1, "Parceiro", h[1], h[1] ? Jogador::VelocidadeDoHumano : perfis::Parceiro.Velocidade);
    Jogadores.emplace_back(1, 0, "Rival 1", h[2], h[2] ? Jogador::VelocidadeDoHumano : rival.Velocidade);
    Jogadores.emplace_back(1, 1, "Rival 2", h[3], h[3] ? Jogador::VelocidadeDoHumano : rival.Velocidade);
    IAs.reserve(2);
    IAs.emplace_back(0, perfis::Parceiro, Aleatorio);
    IAs.emplace_back(1, rival, Aleatorio);
    IniciarPonto();
}

std::array<Jogador*, 2> Partida::JogadoresDoTime(int32_t time)
{
    return time == 0 ? std::array<Jogador*, 2>{&Jogadores[0], &Jogadores[1]} : std::array<Jogador*, 2>{&Jogadores[2], &Jogadores[3]};
}

Jogador& Partida::Sacador()
{
    auto s = Placar.SacadorAtual();
    return Jogadores[s.Time * 2 + s.Jogador];
}

void Partida::Emitir(const EventoDaPartida& evento)
{
    if (AoEvento) AoEvento(evento);
}

static int32_t IndiceDe(const Jogador& j) { return j.Time * 2 + j.Indice; }

void Partida::IniciarPonto()
{
    Jogador& sacador = Sacador();
    int32_t lado = sacador.Lado;
    bool direita = Placar.LadoDoSaqueAtual() == LadoDoSaque::Direita;
    int32_t sinalDoSacador = direita ? lado : -lado;   // metade em que o sacador está
    int32_t timeReceptor = 1 - sacador.Time;
    auto [receptor, parceiroDoReceptor] = QuemRecebe(timeReceptor, direita);
    auto dupla = JogadoresDoTime(sacador.Time);
    Jogador& parceiroDoSacador = dupla[0] != &sacador ? *dupla[0] : *dupla[1];

    sacador.X = sinalDoSacador * 2.6f; sacador.Y = lado * 8.3f;
    parceiroDoSacador.X = -sinalDoSacador * 2.3f; parceiroDoSacador.Y = lado * 3.6f;
    receptor->X = -sinalDoSacador * 2.8f; receptor->Y = -lado * 8.0f;
    parceiroDoReceptor->X = sinalDoSacador * 2.3f; parceiroDoReceptor->Y = -lado * 4.2f;
    for (auto& j : Jogadores) j.Cooldown = 0;

    Bola.Reiniciar();
    Bola.Posicionar(sacador.X, sacador.Y, 0.8f);
    CaixaDoSaque = CaixaDeSaque(-lado, direita);
    RallyAtual = 0;
    rolandoHa = 0;
    UltimoGolpe.reset();
    Estado = EstadoDaPartida::Saque;
    Temporizador = sacador.Humano ? EsperaDoSaqueHumano : EsperaDoSaqueDaIA;
    bool segundo = Arbitro.Faltas > 0;
    if (sacador.Humano) Mensagem = padel::Mensagem{segundo ? "Segundo saque — aperte pra sacar" : "Seu saque — aperte pra sacar", false, true};
    else Mensagem.reset();
    Emitir({TipoDeEventoDaPartida::SaquePreparado, IndiceDe(sacador), sacador.Time});
}

std::pair<Jogador*, Jogador*> Partida::QuemRecebe(int32_t timeReceptor, bool direita)
{
    auto dupla = JogadoresDoTime(timeReceptor);
    Jogador* receptor = direita ? dupla[0] : dupla[1];   // índice 0 = metade direita do time = diagonal do saque à direita
    return {receptor, receptor == dupla[0] ? dupla[1] : dupla[0]};
}

void Partida::Sacar(Entrada entrada)
{
    entrada = Saneada(entrada);
    Jogador& sacador = Sacador();
    Caixa caixa = CaixaDoSaque;
    float alvoX, alvoY;
    if (sacador.Humano)
    {
        alvoX = mat::Limitar(caixa.CentroX() + entrada.Dx * sacador.Lado * 1.8f + Aleatorio.Gaussiana() * 0.35f, caixa.XMin + 0.3f, caixa.XMax - 0.3f);
        alvoY = caixa.CentroY() - sacador.Lado * 0.5f;
    }
    else
    {
        float ruido = IAs[sacador.Time].Perfil.ErroDeMira * 0.55f;
        alvoX = caixa.CentroX() + Aleatorio.Gaussiana() * ruido;
        alvoY = caixa.CentroY() + Aleatorio.Gaussiana() * ruido;
    }
    Bola.Lancar(golpes::Calcular(Bola.X, Bola.Y, Bola.Z, alvoX, alvoY, 1.0f, 0.3f, false, {-600, 0}));   // saque por baixo, com slice
    Arbitro.IniciarSaque(sacador.Time, caixa);
    sacador.Cooldown = 0.5f;
    sacador.Golpes += 1;
    UltimoGolpe = GolpeDado{IndiceDe(sacador), TipoDeGolpe::Saque, TempoDeJogo, LadoDoGolpe::Drive};
    Estado = EstadoDaPartida::Rally;
    Mensagem.reset();
    RallyAtual = 1;
    Estatisticas.Golpes += 1;
    for (auto& ia : IAs) ia.AoSacar(*this, sacador.Time);
    Emitir({TipoDeEventoDaPartida::Golpe, IndiceDe(sacador), sacador.Time, std::nullopt, TipoDeGolpe::Saque, LadoDoGolpe::Drive});
}

void Partida::Avancar(float dt, const Entrada* novas)
{
    if (!(dt > 0)) return;
    for (int i = 0; i < 4; i++) entradas[i] = novas ? Saneada(novas[i]) : Entrada{};
    TempoDeJogo += dt;
    for (auto& j : Jogadores) j.AvancarTempo(dt);
    switch (Estado)
    {
    case EstadoDaPartida::Saque:
    {
        Temporizador -= dt;
        Jogador& sacador = Sacador();
        const Entrada& entrada = EntradaDe(sacador);
        if ((sacador.Humano && entrada.AcaoPressionada) || Temporizador <= 0) Sacar(entrada);
        break;
    }
    case EstadoDaPartida::Rally:
        Rally(dt);
        break;
    case EstadoDaPartida::FimDoPonto:
        Temporizador -= dt;
        if (Temporizador <= 0)
        {
            // Fim: Time = o vencedor, ou −1 no empate dos pontos corridos.
            if (Placar.Acabou())
            {
                Estado = EstadoDaPartida::Fim;
                Emitir({TipoDeEventoDaPartida::Fim, -1, Placar.Vencedor.value_or(-1)});
            }
            else IniciarPonto();
        }
        break;
    default:
        break;
    }
}

const Entrada& Partida::EntradaDe(const Jogador& j) const { return entradas[j.Time * 2 + j.Indice]; }

/// Fronteira de confiança: direção não finita vale 0, e cada componente fica em [-1, 1].
Entrada Partida::Saneada(Entrada e)
{
    e.Dx = Componente(e.Dx);
    e.Dy = Componente(e.Dy);
    return e;
}

void Partida::Rally(float dt)
{
    bool manual = Opcoes.ModoDeGolpe == padel::ModoDeGolpe::Manual;
    for (auto& j : Jogadores)
    {
        if (!j.Humano) continue;
        const Entrada& e = EntradaDe(j);
        j.Mover(e.Dx * j.Lado, e.Dy * j.Lado, dt);   // do referencial do jogador pro mundo
        if (!manual) continue;
        if (j.BalancoTerminouNoAr())
        {
            // O balanço acabou sem tocar na bola: raquete no ar, e um instante pra se recompor.
            j.BalancosNoAr += 1;
            j.Cooldown = 0.45f;
            Emitir({TipoDeEventoDaPartida::Errou, IndiceDe(j), j.Time});
        }
        if ((e.AcaoPressionada || e.LobPressionada) && !j.Balancando() && j.Cooldown <= 0)
        {
            j.IniciarBalanco(e.LobPressionada);
            Emitir({TipoDeEventoDaPartida::Balanco, IndiceDe(j), j.Time});
        }
    }
    for (auto& ia : IAs) ia.Reagir(dt, *this);

    if (Bola.EmJogo && !Bola.Rolando)
    {
        for (auto& j : Jogadores)
        {
            if (j.Cooldown > 0 || !Arbitro.PodeGolpear(j.Time)) continue;
            if (j.Humano && manual)
            {
                // A raquete passa pelo ponto de contato uma vez por balanço: sem balanço, ou fora desse instante, a bola passa.
                if (!j.NoInstanteDoContato() || !j.Alcanca(Bola)) continue;
            }
            else if (!j.PodeBaterAgora(Bola, dt, Arbitro.QuicouNoReceptor)) continue;
            Golpear(j);
            break;
        }
    }

    std::vector<EventoDaBola> eventos;
    eventos.reserve(4);
    Bola.Avancar(dt, eventos);
    bool rebateu = false;
    for (const auto& evento : eventos)
    {
        if (evento.Tipo == TipoDeEventoDaBola::Saiu) FixarNoPontoDeSaida();
        Emitir({TraduzirEvento(evento.Tipo)});
        if (auto decisao = Arbitro.Processar(evento)) { Decidir(*decisao); return; }
        if (evento.Tipo == TipoDeEventoDaBola::Quique || evento.Tipo == TipoDeEventoDaBola::Parede) rebateu = true;
    }
    if (rebateu) for (auto& ia : IAs) ia.AoRebater(*this);

    if (Bola.Rolando)
    {
        rolandoHa += dt;
        if (rolandoHa > 0.6f)
        {
            // Bola rolando é bola que já quicou e não vai quicar de novo: conta como o quique seguinte.
            auto decisao = Arbitro.Processar({TipoDeEventoDaBola::Quique, Bola.X, Bola.Y, 0, quadra::LadoDe(Bola.Y)});
            Decidir(decisao ? *decisao : Decisao::Ponto(Arbitro.Receptor().value_or(0), padel::Motivo::BolaMorta));
        }
    }
    else
    {
        rolandoHa = 0;
    }
    if (Estado == EstadoDaPartida::Rally && !Bola.EmJogo)
    {
        Decidir(Decisao::Ponto(Arbitro.Receptor().value_or(0), padel::Motivo::Fora));
    }
}

/// A Bola marca a saída um sub-passo além do plano da parede; o que fica parado na tela é o ponto de saída, no plano.
void Partida::FixarNoPontoDeSaida()
{
    Bola.X = mat::Limitar(Bola.X, -quadra::MeiaLargura, quadra::MeiaLargura);
    Bola.Y = mat::Limitar(Bola.Y, -quadra::MeioComprimento, quadra::MeioComprimento);
}

void Partida::Golpear(Jogador& jogador)
{
    Golpe golpe = jogador.Humano ? GolpeDoHumano(jogador, EntradaDe(jogador)) : IAs[jogador.Time].EscolherGolpe(jogador, Bola, *this);
    Bola.Lancar(golpes_especiais::VelocidadeDe(golpe, Bola.X, Bola.Y, Bola.Z));
    jogador.EncerrarBalanco();
    jogador.Cooldown = 0.4f;
    jogador.Golpes += 1;
    Arbitro.RegistrarGolpe(jogador.Time);
    UltimoGolpe = GolpeDado{IndiceDe(jogador), golpe.Tipo, TempoDeJogo, golpe.Lado};
    RallyAtual += 1;
    Estatisticas.Golpes += 1;
    for (auto& ia : IAs) ia.AoGolpear(*this, golpe, jogador);
    Emitir({TipoDeEventoDaPartida::Golpe, IndiceDe(jogador), jogador.Time, std::nullopt, golpe.Tipo, golpe.Lado});
}

Golpe Partida::GolpeDoHumano(Jogador& jogador, const Entrada& entrada)
{
    Golpe g = EscolherGolpeDoHumano(jogador, entrada);
    g.Lado = jogador.LadoDoGolpePara(Bola);
    return g;
}

/// A tabela de prioridades do golpe do humano está no Partida.cs (EscolherGolpeDoHumano), com o porquê de cada linha.
Golpe Partida::EscolherGolpeDoHumano(Jogador& jogador, const Entrada& entrada)
{
    int32_t lado = jogador.Lado, ladoDoAlvo = -lado;
    bool manual = Opcoes.ModoDeGolpe == padel::ModoDeGolpe::Manual;
    float erro = mat::Limitar(PesoDoCorpoNoErro * jogador.DificuldadeDoGolpe(Bola) + PesoDoAtraso * jogador.AtrasoDoContato(Bola), 0, 1.6f);
    UltimoErroDoHumano = erro;
    if (erro > 0.95f && Aleatorio.Proximo() < 0.35f + (erro - 0.95f))
    {
        // Golpe muito ruim: metade na rede, metade no vidro sem quicar.
        if (Aleatorio.Proximo() < 0.5f) { float x = (Aleatorio.Proximo() - 0.5f) * 6; return NovoGolpe(x, ladoDoAlvo * 0.3f, 0.45f, TipoDeGolpe::Erro, true); }
        float x = (Aleatorio.Proximo() - 0.5f) * 6;
        return NovoGolpe(x, ladoDoAlvo * 11.5f, 0.55f, TipoDeGolpe::Erro, true);
    }
    float sigma = 0.25f + 0.8f * erro;
    auto Ruido = [&] { return Aleatorio.Gaussiana() * sigma; };
    float x = entrada.Dx < -0.3f ? -3.3f * lado : entrada.Dx > 0.3f ? 3.3f * lado : jogador.X * 0.4f;
    x = mat::Limitar(x + Ruido(), -4.5f, 4.5f);
    bool frente = entrada.Dy < -0.3f, tras = entrada.Dy > 0.3f;
    bool ladoForte = std::fabs(entrada.Dx) >= LimiarDoLadoForte;
    int32_t paraOLado = (entrada.Dx < 0 ? -1 : 1) * lado;   // o lado forte, no mundo
    bool bolaAlta = Bola.Z > AlturaDaBolaAlta;
    bool lob = manual ? jogador.BalancoDeLob() : entrada.AcaoSegurada;
    bool acaoSegurada = manual ? entrada.AcaoSegurada && !jogador.BalancoDeLob() : entrada.AcaoSegurada;
    auto Smash = [&] { float y = ladoDoAlvo * (4.2f + Ruido()); return NovoGolpe(x, y, 0.4f, TipoDeGolpe::Smash, false, {1500, 0}); };

    if (bolaAlta && frente && acaoSegurada)
    {
        if (erro < ErroMaximoDoRemateForte)
        {
            auto forte = ladoForte ? golpes_especiais::SmashPor3(Bola.X, Bola.Y, Bola.Z, ladoDoAlvo, paraOLado)
                                   : golpes_especiais::SmashPor4(Bola.X, Bola.Y, Bola.Z, ladoDoAlvo, x);
            if (forte) return *forte;
        }
        return Smash();
    }
    if (lob)
    {
        if (frente)
            if (auto chiquita = golpes_especiais::Chiquita(Bola.X, Bola.Y, Bola.Z, ladoDoAlvo, x)) return *chiquita;
        float y = ladoDoAlvo * (8.2f + Ruido() * 0.5f);
        return NovoGolpe(x, y, 1.6f, TipoDeGolpe::Lob, false, {-400, 0});
    }
    if (tras && jogador.BolaAtrasJuntoAoVidro(Bola))
        if (auto contrapared = golpes_especiais::Contrapared(Bola.X, Bola.Y, Bola.Z, ladoDoAlvo, x)) return *contrapared;
    if (bolaAlta)
    {
        if (frente) return Smash();
        if (ladoForte)
        {
            float vx = mat::Limitar(3.8f * paraOLado + Ruido(), -4.5f, 4.5f);
            float vy = ladoDoAlvo * (6.5f + Ruido());
            return NovoGolpe(vx, vy, 0.7f, TipoDeGolpe::Vibora, false, {-800, static_cast<float>(1800 * paraOLado * ladoDoAlvo)});
        }
        float y = ladoDoAlvo * (7.5f + Ruido());
        return NovoGolpe(x, y, 0.95f, TipoDeGolpe::Bandeja, false, {-1500, 500});
    }
    if (frente) { float y = ladoDoAlvo * (4.5f + Ruido()); return NovoGolpe(x, y, 0.6f, TipoDeGolpe::Ataque, false, {2200, 0}); }
    if (tras) { float y = ladoDoAlvo * (7.8f + Ruido()); return NovoGolpe(x, y, 1.05f, TipoDeGolpe::Defesa, false, {-1200, 0}); }
    float y = ladoDoAlvo * (6.6f + Ruido());
    return NovoGolpe(x, y, 0.8f, TipoDeGolpe::Normal, false, {1200, 0});
}

void Partida::Decidir(const Decisao& decisao)
{
    if (decisao.Tipo == TipoDeDecisao::Ponto) { EncerrarPonto(decisao.Para, decisao.Motivo); return; }
    Bola.EmJogo = false;
    Estado = EstadoDaPartida::FimDoPonto;
    Temporizador = 1.3f;
    if (decisao.Tipo == TipoDeDecisao::Falta)
    {
        Estatisticas.Faltas += 1;
        Mensagem = padel::Mensagem{std::string("Falta — ") + TextoDoMotivo(decisao.Motivo) + ". Segundo saque"};
        Emitir({TipoDeEventoDaPartida::Falta, -1, -1, decisao.Motivo});
    }
    else
    {
        Estatisticas.Lets += 1;
        Mensagem = padel::Mensagem{"Let — a bola tocou a rede. Repete o saque"};
        Emitir({TipoDeEventoDaPartida::Let});
    }
}

void Partida::EncerrarPonto(int32_t para, padel::Motivo motivo)
{
    EventoDoPlacar evento = Placar.PontoPara(para);
    Estatisticas.Pontos += 1;
    Estatisticas.Motivos[motivo] += 1;
    Estatisticas.MaiorRally = std::max(Estatisticas.MaiorRally, RallyAtual);
    Bola.EmJogo = false;
    Arbitro.NovoPonto();
    Mensagem = padel::Mensagem{TextoDoPonto(para, motivo, evento), evento.Tipo != TipoDeEventoDoPlacar::Ponto};
    Estado = EstadoDaPartida::FimDoPonto;
    Temporizador = evento.Tipo == TipoDeEventoDoPlacar::Ponto ? 1.5f : 2.4f;
    TipoDeEventoDaPartida tipo = TipoDeEventoDaPartida::Ponto;
    if (evento.Tipo == TipoDeEventoDoPlacar::Game) tipo = TipoDeEventoDaPartida::Game;
    else if (evento.Tipo == TipoDeEventoDoPlacar::Set) tipo = TipoDeEventoDaPartida::Set;
    else if (evento.Tipo == TipoDeEventoDoPlacar::Partida) tipo = TipoDeEventoDaPartida::Partida;
    Emitir({tipo, -1, para, motivo});
}

std::string Partida::TextoDoPonto(int32_t para, padel::Motivo motivo, const EventoDoPlacar& evento) const
{
    std::string descricao = TextoDoMotivo(motivo);
    bool casa = para == 0;
    switch (evento.Tipo)
    {
    case TipoDeEventoDoPlacar::Game: return std::string(casa ? "Game da casa" : "Game dos rivais") + " — " + descricao;
    case TipoDeEventoDoPlacar::Set: return std::string(casa ? "Set da casa!" : "Set dos rivais.") + " " + Placar.Resumo();
    case TipoDeEventoDoPlacar::Partida:
        // Pelo Vencedor, não por quem ganhou o ponto: nos pontos corridos o último ponto pode ser de quem perdeu.
        if (Placar.Vencedor == 0) return "A casa venceu! " + Placar.Resumo();
        if (Placar.Vencedor == 1) return "Os rivais venceram. " + Placar.Resumo();
        return "Empate! " + Placar.Resumo();
    default: return std::string(casa ? "Ponto da casa" : "Ponto dos rivais") + " — " + descricao;
    }
}
} // namespace padel
