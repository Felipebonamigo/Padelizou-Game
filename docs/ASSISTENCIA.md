# Níveis de assistência — proposta

> **15/10:** o Felipe disse "siga" depois desta proposta. Feitas a parte 1, a janela do timing, e a parte 2, o movimento
> assistido (Core e PadelSim), com os números abaixo. Falta o RB mirar a saída de parede. O alvo de calibração continua a
> sugestão do fim desta página até ele dizer outro — e as medições levantam duas perguntas (abaixo).

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

## O que o movimento assistido faz (medido em 15/10, no Core)

A parte 2 está feita: com o analógico solto, o humano que a IA do time mandou na bola anda sozinho até o ponto de contato
(ao lado do x de verdade da bola, sem o erro de leitura da IA) — no Casual de onde estiver, no Padrão só a menos de 1 m.
O analógico mexido manda; quem não vai na bola não é levado; no Pro, nada.

Humano simulado contra a Médio, formato curto (até 4 games, super tie-break a 10), quatro botões, 24 partidas por linha.
"Só botão" é o mesmo humano simulado com o analógico sempre solto (decide e aperta, não anda):

| Humano | Pro | Padrão | Casual |
|---|---|---|---|
| Iniciante, joga normal | 42,5 % dos pontos (29 % das partidas) | 40,0 % (0 %) | 43,8 % (21 %) |
| Iniciante, só botão | 21,5 % (0 %) | — | **51,6 % (58 %)** |
| Intermediário, joga normal | 55,5 % (83 %) | 55,1 % (75 %) | 57,8 % (100 %) |
| Intermediário, só botão | 25,8 % (0 %) | — | 67,0 % (96 %) |

- **O Casual cumpre a promessa:** quem só aperta botão chega perto de metade das partidas contra a Médio (o alvo sugerido
  abaixo). Sem a ajuda, o mesmo jogador não ganha nenhuma.
- **Quem mexe o analógico perde a ajuda** (Iniciante normal no Casual: 43,8 %, pior que o só botão). É a regra "o
  analógico manda": o jogador de verdade que corre atrás da bola com erro de leitura não recebe o ajuste. Uma saída, se o
  playtest pedir: no Casual, o analógico só empurra (a ajuda soma em vez de desligar).
- **O ajuste é bom demais pra quem já joga bem:** o Intermediário só no botão (67 %) joga melhor que ele mesmo
  correndo (57,8 %), porque o assistido não tem erro de leitura. Saída possível: o assistido ler a bola com um erro
  pequeno, como a IA.
- Com o humano simulado (que sempre corre), o Padrão quase não muda o resultado: o ajuste final só age quando ele para.

## O que falta decidir

1. O alvo de cada nível. Sugestão: Iniciante perto de 50 % das partidas contra a Médio no Casual; Intermediário perto
   de 50 % no Padrão. (Medido: o Intermediário já ganha 83 % no Pro; o alvo do Padrão precisa ser outro.)
2. No Casual, o analógico desliga a ajuda (hoje) ou só empurra?
3. O movimento assistido lê a bola sem erro (hoje) ou com um erro pequeno, pra não jogar melhor que o jogador?
2. Aprovar a mecânica acima. Com isso, o Claude faz no Core (com teste), mede o movimento assistido com o humano
   simulado até o alvo, regera o gabarito e porta.
