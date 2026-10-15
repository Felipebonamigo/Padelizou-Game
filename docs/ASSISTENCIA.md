# Níveis de assistência — proposta

> **15/10:** o Felipe disse "siga" depois desta proposta. A parte 1, a janela do timing, está feita (Core `1d0358f` e
> PadelSim), com os números abaixo. O movimento assistido e o RB vêm depois, e o alvo de calibração continua a sugestão
> do fim desta página até ele dizer outro.

O `DESIGN.md` aprovou três níveis (Casual, Padrão, Pro), trocáveis a qualquer hora, que mudam o movimento, o
rastro-fantasma da parede e a janela do timing. O rastro já existe (`rastro_fantasma::Prever`, 15/10). Movimento e
janela mudam a regra do golpe, então entram no `Padel.Core` primeiro, só depois de aprovados. Esta página é a proposta
com os números medidos.

## Proposta

| | Casual | Padrão | Pro |
|---|---|---|---|
| Janela do timing | ±40 ms | ±20 ms | o instante exato (como hoje) |
| Movimento | vai sozinho até o ponto de contato | só o ajuste final (dentro de 1 m) | nenhum |
| Rastro-fantasma | desde que a bola vem | só depois da batida no vidro | desligado |
| RB (saída de parede) | o movimento mira o ponto depois do vidro | idem, no ajuste final | só troca o smash pela víbora |

**Janela do timing:** hoje a raquete só pega a bola no passo mais perto de 0,12 s depois do aperto. Com janela, ela pega
em qualquer passo de [0,12 − J, 0,12 + J], naquele em que a bola está mais perto do ponto ideal (a mesma regra da IA,
`Jogador.PodeBaterAgora`). Com J = 0 é o jogo de hoje.

## O que a janela faz (medido em 15/10, protótipo fora do repositório)

Calibração do Arena (`ferramentas/Calibracao`, humano simulado no manual, 24 partidas por célula, até 4 games). Com
J = 0 o protótipo dá exatamente os números de hoje: a medição confere.

Iniciante × Médio:

| Janela | % dos pontos | % das partidas | golpes bons |
|---|---|---|---|
| ±0 (hoje) | 37,6 | 4 | 35 % |
| ±20 ms | 44,7 | 25 | 42 % |
| ±40 ms | 46,7 | 38 | 47 % |
| ±60 ms | 44,2 | 25 | 55 % |
| ±80 ms | 46,4 | 38 | 55 % |

- O ganho em pontos para por volta de ±40 ms. Acima disso, sobem os golpes bons, mas não os pontos.
- Do Intermediário para cima quase nada muda (Intermediário × Médio: 56,8 % dos pontos com ±0, 57,6 % com ±40 ms): o
  erro de timing deles já é pequeno.
- Só a janela não leva o Iniciante a metade das partidas contra a Médio (38 % no máximo). O resto teria de vir do
  movimento assistido, ou de um alvo diferente.

## O que falta decidir

1. O alvo de cada nível. Sugestão: Iniciante perto de 50 % das partidas contra a Médio no Casual; Intermediário perto
   de 50 % no Padrão.
2. Aprovar a mecânica acima. Com isso, o Claude faz no Core (com teste), mede o movimento assistido com o humano
   simulado até o alvo, regera o gabarito e porta.
