// Tipos comuns da física. Porte fiel do Padel.Core (C#, repositório padelizou, padelizou-arena/Padel.Core), que é o
// gabarito: Testes/PadelSim/Gabarito tem as trajetórias que este código precisa reproduzir.
#pragma once

#include "PadelSim/Exportar.h"

#include <cstdint>

namespace padel
{
enum class TipoDeEventoDaBola : int32_t { Quique, Parede, Rede, CruzouRede, Saiu };
enum class QualParede : int32_t { Lateral, Fundo };
/// Aberto: onde não há parede (porta, ou acima da altura do trecho) — é a superfície dos eventos Saiu.
enum class Superficie : int32_t { Chao, Vidro, Grade, Aberto };

/// Um acontecimento físico da bola. O árbitro decide o que ele significa. Em Parede, Superficie é o painel atingido
/// (Vidro ou Grade, em Material); em Saiu, é Aberto, Parede diz por qual parede a bola saiu e PelaPorta se foi por uma das portas.
/// Em CruzouRede, Lado é pra que lado a bola foi.
struct PADELSIM_API EventoDaBola
{
    TipoDeEventoDaBola Tipo;
    float X, Y, Z;
    int32_t Lado;
    QualParede Parede = QualParede::Lateral;
    Superficie Material = Superficie::Chao;   // Superficie no Padel.Core (o nome do tipo não pode repetir aqui)
    bool PelaPorta = false;
};

/// Velocidade inicial de um golpe, com o efeito (spin, em rad/s) que a raquete imprimiu.
struct PADELSIM_API Velocidade
{
    float Vx = 0, Vy = 0, Vz = 0, Tempo = 0;
    float Wx = 0, Wy = 0, Wz = 0;
};

/// Efeito pedido num golpe, em rpm: topspin positivo (slice negativo) e sidespin (positivo curva pra esquerda de quem bate).
struct PADELSIM_API Efeito
{
    float TopspinRpm = 0, SidespinRpm = 0;
};
} // namespace padel
