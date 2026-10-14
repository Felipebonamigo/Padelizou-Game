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
/// AcaoPressionada saca e, no modo manual, começa o balanço; LobPressionada começa um balanço de lob.
struct Entrada
{
    float Dx = 0, Dy = 0;
    bool AcaoPressionada = false, AcaoSegurada = false, LobPressionada = false;
};
} // namespace padel
