// Tipos do golpe e da entrada (Padel.Core/Jogadores.cs).
#pragma once

#include "PadelSim/Tipos.h"

#include <optional>

namespace padel
{
enum class Dificuldade : int32_t { Facil, Medio, Dificil };
/// Chiquita: bola baixa e lenta nos pés de quem está na rede. Contrapared: bater na própria parede de fundo pra bola
/// voltar e cruzar a rede. SmashPor3 / SmashPor4: remate que quica do outro lado e sai pela lateral / por cima do fundo.
enum class TipoDeGolpe : int32_t { Saque, Normal, Ataque, Defesa, Lob, Smash, Bandeja, Vibora, Erro, Chiquita, Contrapared, SmashPor3, SmashPor4 };
/// No referencial de quem bate (olhando pra rede): bola do lado da raquete é drive; do outro lado, revés.
enum class LadoDoGolpe : int32_t { Drive, Reves };
enum class Formacao : int32_t { Fundo, Rede };
/// O botão que começou o balanço: A (chapado), B (lob), X (cortado) ou Y (ataque). X e Y só no controle de quatro botões.
enum class IntencaoDoGolpe : int32_t { Chapado, Lob, Cortado, Ataque };
/// DoisBotoes (o do Arena): ação e lob, e o analógico escolhe o golpe com a situação. QuatroBotoes (o da fatia): o botão
/// que começa o balanço escolhe a família do golpe e o analógico só a direção (Partida::EscolherGolpeDoHumano).
enum class Controle : int32_t { DoisBotoes, QuatroBotoes };
/// O nível de assistência do humano, de cada jogador e trocável a qualquer hora (Jogador::Assistencia). Hoje muda a janela
/// do timing (JanelaDoContato): no Pro, o instante do balanço; no Padrão e no Casual, o melhor passo de uma janela em volta.
enum class Assistencia : int32_t { Pro, Padrao, Casual };

/// Um golpe decidido: onde a bola deve cair, em quanto tempo, com que efeito e de que lado do corpo. Lancamento,
/// quando vem, é a velocidade exata que um solucionador já simulou; sem ele, a partida calcula pelo alvo.
struct Golpe
{
    float AlvoX = 0, AlvoY = 0, TempoDeVoo = 0;
    TipoDeGolpe Tipo = TipoDeGolpe::Normal;
    bool IgnorarRede = false;
    padel::Efeito Efeito{};   // o tipo com o nome qualificado: o membro tem o mesmo nome
    LadoDoGolpe Lado = LadoDoGolpe::Drive;
    std::optional<Velocidade> Lancamento;
};

/// Entrada de um humano, no referencial DELE: Dy < 0 é "rumo à rede", Dx > 0 é "minha direita".
/// AcaoPressionada saca e, no modo manual, começa o balanço; LobPressionada começa um balanço de lob. Os outros só valem
/// no controle de quatro botões: X (cortado), Y (ataque, e se segue apertado) e o RB (modificador, segurado).
struct Entrada
{
    float Dx = 0, Dy = 0;
    bool AcaoPressionada = false, AcaoSegurada = false, LobPressionada = false;
    bool CortadoPressionado = false, AtaquePressionado = false, AtaqueSegurado = false, ModificadorSegurado = false;
};
} // namespace padel
