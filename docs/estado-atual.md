# Estado atual

Diário em ordem cronológica reversa: o mais novo em cima.

## 16/10/2026 — projeto Unreal e quadra cinza jogável (sessão local do PC)

- **Pedidos 0001 e 0002 respondidos** (`docs/pedidos/`). `PadelizouGame.uproject` (UE 5.8.3) com os módulos `PadelizouGame`,
  `PadelSim` e `PadelSimTestes` (Editor). `PadelSim.Gabarito` roda os 8 `Testes*.cpp` dentro do Unreal: **0 falhas**, os mesmos
  números do ctest. O UBT 5.8 compila com `/fp:fast` por padrão e isso reprova o gabarito (mutante: Matemática 987 falhas, Partidas
  72); o `PadelSim.Build.cs` fixa `FPSemantics = Precise` e vale para Editor e Game (os dois alvos compilam).
- **Pedido à nuvem:** classe ou função livre nova do motor leva `PADELSIM_API` (`PadelSim/Exportar.h`), e nada de
  `static constexpr float A = .., B = ..;` numa classe exportada (o MSVC recusa). Sem isso o CMake passa e o link do Unreal falha.
- **Quadra cinza** (`/Game/Mapas/QuadraCinza`): `APadelPartidaAtor` dono da `Partida` + `PassoFixo(120)`, paredes de
  `quadra::Paineis()`, quatro cápsulas, câmera de TV, Enhanced Input (4 botões), placar UMG. Partida inteira contra o Médio: Manual
  `5-4(2)` e Automático `4-0`, Tick do ator em ~0,05 ms. `Coordenadas.h`: Unreal X = y, **Y = -x**, Z = z (m → cm).
- **Playtest do Felipe:** A/D invertidos (era a conversão; corrigido), quadra tapando a visão (grade translúcida), bonecos feios
  (cápsulas), Manual duro demais e "o botão não bate" (o Manual exige apertar ~0,12 s antes do contato). Hoje: **Automático por
  padrão**, assistência Casual, F4 câmera lenta, e **indicador visual de quando apertar** no Manual (previsão com a física do PadelSim).
- **Falta:** o Felipe jogar o Manual com o indicador e dizer o feeling; testar o gamepad físico; decidir se alarga a janela do Casual
  (regra, vai pelo `Padel.Core`). A etapa seguinte do design (6 golpes e a saída de parede com o rastro-fantasma) vem depois.

## 15/10/2026 (4) — assistência, parte 2: o movimento assistido

- Com o analógico solto, o humano que a IA do time mandou na bola anda sozinho até o ponto de contato (`IA::AlvoDoCorpo`:
  ao lado do x de verdade da bola, sem o erro de leitura) — no Casual de onde estiver, no Padrão só a menos de 1 m. O
  analógico mexido manda; no Pro, nada. Core `4733b73`.
- Medido contra a Médio, formato curto: o Iniciante que só aperta botão faz 51,6 % dos pontos (58 % das partidas) no
  Casual, contra 21,5 % (0 %) no Pro. Tabela e as duas perguntas que isso levanta (o analógico desligar a ajuda; a
  ajuda ler a bola sem erro) no `docs/ASSISTENCIA.md`.
- Gabarito: partida 11 (Iniciante só no botão, Casual). Sem o porte, as partidas 9, 10 e 11 reprovaram; portado, 8
  testes verdes em gcc e clang (partidas: 21.969 verificações). Falta a parte 3: o RB mirando a saída de parede.

## 15/10/2026 (3) — janela do timing e os achados da revisão adversarial

- **Assistência, parte 1 (janela do timing):** `Assistencia { Pro, Padrao, Casual }`, de cada jogador e trocável a
  qualquer hora (`Jogador::Assistencia`; o inicial dos humanos em `OpcoesDaPartida`). No Pro (o padrão, o jogo de hoje) a
  raquete só pega a bola no instante do balanço; no Padrão (±20 ms) e no Casual (±40 ms), no passo da janela com a bola
  mais perto do ideal. Core `1d0358f`; números e proposta inteira em `docs/ASSISTENCIA.md`. Movimento assistido e o RB
  mirando a saída de parede vêm depois.
- **Revisão adversarial de hoje** (achados corrigidos, cada um com teste visto falhar ou mutante que agora reprova):
  - ALTA: a trava de "um cambio por trajetória" bloqueava o cambio legítimo de quando o replanejamento passa a bola pro
    outro (Difícil: 1.429 passos com os dois na mesma metade). Agora vale por trajetória e por quem vai (`3c0318e`).
    `REALISMO.md` e D10 remedidos: Iniciante × Médio 37,5 % dos pontos.
  - MÉDIA: a sala online aceitava quatro botões e assistência, que o protocolo não carrega; o `ServidorDaPartida` agora
    recusa (`4c56571`). No C++, `Partida::Opcoes` era mutável e furava a recusa do Automático: virou `const`, com
    `static_assert` no teste.
  - BAIXA: o rastro dava saída de parede a uma bola morta (2º quique e vidro no mesmo passo); o teste do rastro era
    tautológico (agora compara com a bola da `Partida`); a prioridade dos botões juntos não tinha teste; e duas frases
    deste diário eram falsas (abaixo, corrigidas).
- **Gabarito novo, `golpes-do-humano.csv`:** 45 cenas dos caminhos que nenhuma partida exercita (A segurado, por 3, smash
  de recurso, botões juntos, víbora com RB e a janela). Antes do porte reprovaram as 12 da janela, a partida (cambio e
  janela, partidas 0–5 e 7–10) e o rastro novo (2º quique e vidro); portado, 8 testes verdes em gcc e clang. Teste novo:
  `TestesDosGolpesDoHumano`.

## 15/10/2026 (2) — rastro-fantasma da saída de parede

- Pilar 1 do design ("a parede é a estrela"): `RastroFantasma.Prever` no `Padel.Core` (`2c48d97`) e
  `rastro_fantasma::Prever` no PadelSim. Roda a física da partida adiante numa cópia da bola, no passo da partida, até a
  bola deixar de ter jogo do lado de quem recebe (segundo quique, vidro antes do quique, outro lado, saída, 3 s), e
  marca o primeiro ponto depois da batida no vidro já quicada. Não é regra: é previsão.
- Como o Unreal usa, pelo nível de assistência do design: **Casual**, desenha a partir de `PrimeiroDepoisDaParede`
  assim que a bola vem; **Padrão**, pede o rastro no passo do evento `Parede` (aí ele inteiro é a saída); **Pro**, nada.
  Vai num pedido da etapa de semanas 5–6 (os 6 golpes e a saída de parede), depois da quadra cinza.
- 9 testes no Core; com a casca vazia, 8 falharam (o da bola fora de jogo já passava), entre eles o de que o rastro é o
  caminho da bola quando ninguém bate — que só ficou de verdade na revisão de 15/10 (agora compara com a `Partida`) (Core
  684 verdes; Arena `verificar_tudo.sh` verde). Gabarito com 8 rastros (`rastros.csv`, `rastros-pontos.csv`); o C++
  reprovou com a casca vazia (1.496 de 1.502) e, portado, deu 0 falhas; 7 testes verdes em gcc e clang.

## 15/10/2026 — controle de quatro botões

- O controle do `DESIGN.md` (A chapado, B lob, X cortado, Y ataque, RB modificador) entrou no `Padel.Core` (`7867e84`)
  e no PadelSim: `OpcoesDaPartida.Controle = QuatroBotoes`, só no modo Manual. O botão que começa o balanço escolhe a
  família do golpe e o analógico só a direção:

  | Botão | Bola baixa | Bola alta |
  |---|---|---|
  | A | Normal (segurado no contato: Ataque) | Normal (idem) |
  | B | Lob | Lob |
  | X | Defesa (o slice) | Bandeja |
  | Y | Ataque | Smash; segurado: por 4 (com lado forte, por 3); com RB: Víbora |

  O padrão segue o de dois botões do Arena, que não muda. **Decisões minhas, a confirmar no playtest:** A segurado =
  Ataque (o "segurar carrega potência" do design), B não vira chiquita (ficou fora da fatia), o RB por enquanto só
  troca o smash pela víbora (o "espera a bola voltar do vidro" depende da assistência de movimento, que ainda não
  existe) e o "globo de defesa" na rede é o mesmo lob.
- O humano simulado joga de quatro botões com o mesmo plano de golpe: o Avançado faz 57,6 % dos pontos contra a Médio
  (57,0 % no de dois; 24 partidas de 180 s).
- 8 testes novos no Core, vistos falhar (Core 675 verdes; Arena `verificar_tudo.sh` verde). Os golpes do humano viraram
  funções locais na mesma ordem de sorteio: regerado sem caso novo, o gabarito saiu idêntico.
- Gabarito: partida 8, formato curto de quatro botões (semente 126: o humano passa pelas sete famílias, com um por 4 — mas
  não por 3 nem A segurado, que estão em `golpes-do-humano.csv` desde a revisão de 15/10 —, e o
  set vai ao super tie-break 5-4), e o `PassoFixo` com X, Y e RB. Antes do porte, reprovaram a partida 8 e o
  `PassoFixo`; depois, os 6 testes com 0 falhas em gcc e clang (partidas: 15.362 verificações). Teste novo em C++: a
  Partida recusa quatro botões no Automático (visto falhar sem a recusa).
- Pedido 0002 (quadra cinza) atualizado: os quatro botões no Enhanced Input.

## 14/10/2026 (noite, 4) — revisão da formação: o cambio e o time do humano

- A revisão adversarial da Onda 5 achou três defeitos, corrigidos no `Padel.Core` (`2b7d8a5`) com teste escrito antes
  e visto falhar:
  - o cambio contava o parceiro que ia buscar a bola no meio da metade DELE como invasor (o centro da dupla anda com
    a bola): no Difícil, 54 de 150 cambios faziam quem ficava cruzar a quadra pelo parceiro. Agora o parceiro tem de
    invadir a metade de quem fica;
  - a releitura da bola (quique, parede) desfazia a troca no meio do caminho (11 de 70 em menos de 0,6 s): agora é
    um cambio por trajetória;
  - no time de um humano, a formação guardada ficava a do saque; se o humano virasse IA no meio do ponto (o servidor
    faz isso com quem cai), ia pro fundo com a dupla na rede.
- Core 667 testes verdes; Arena `verificar_tudo.sh` verde. As tabelas da formação no `REALISMO.md` foram remedidas: os
  números "antes" eram de antes da troca da libm e não se reproduziam.
- Gabarito regerado; antes do porte o C++ reprovou onde devia (partidas 0–5 e 7). Porte em `Jogadores.cpp`
  (`CobrirOOutroLado` devolve se trocou; `cambioNaTrajetoria`): 6 testes com 0 falhas, gcc e clang.
- **D10 remedido e decidido:** Iniciante × Médio 43,9 → 37,6 % dos pontos (21 → 4 % das partidas); o resto no
  `REALISMO.md`. Em 15/10 o Felipe decidiu: **o equilíbrio fica como está**.

## 14/10/2026 (noite, 3) — a dupla da IA em formação

- O trabalho da Onda 5 do Arena (25/09), que estava num worktree sem commit, entrou no `Padel.Core` (`fbb69bf`): quem
  não vai na bola anda em bloco com o parceiro (sobe depois de golpe que dá tempo, recua quando a bola passa por cima),
  o centro da dupla segue a bola, o "cambio" cobre o lado vazio, e o parceiro IA de um humano lê a formação dele. É o
  item que o design chama de "o que mais dá cara de padel depois da animação". Números por nível no `PerfilDeIA`.
- Porte em C++ (`IA::LugarNaFormacao` e companhia): as 8 partidas do gabarito iguais ao Core na primeira rodada.
- **Equilíbrio (decisão D10 do Felipe, em aberto):** a formação deixa a IA mais difícil para o humano, sobretudo no
  Fácil e no Médio (Iniciante × Médio: 48,2 → 39,9 % dos pontos; tabela completa no `REALISMO.md` do Arena;
  remedido na entrada acima).
- **Duração da partida curta** (formato de 4 games, super tie-break a 10), 20 partidas por par, humano simulado no
  manual: Intermediário × Médio 6,1 min de mediana (3,5 a 12,6), Avançado × Médio 5,2 (3,4 a 10,8), Intermediário ×
  Fácil 2,9 (2,5 a 4,7), Avançado × Difícil 7,5 (4,2 a 15,6). A meta do pilar 3 é 4 a 6 min: o Médio está nela; as
  pontas longas são os sets que vão a 4-4 e ao super tie-break.

## 14/10/2026 (noite, 2) — formato curto: set até 4 games, super tie-break a 10

- Pilar 3 do design (partida de 4 a 6 minutos). No `Padel.Core` (`73871f4`): `Placar.GamesPorSet` (padrão 6) e
  `Placar.PontosDoTieBreak` (padrão 7), e as mesmas opções na `OpcoesDaPartida`. Formato curto = 4 e 10: fecha em 4-2
  ou 5-3; em 4-4, super tie-break a 10 por 2. Fora do padrão não vale online no Arena (o protocolo dele só carrega
  ponto de ouro e sets). 9 testes novos no Core, vistos falhar; Core 658 verdes; Arena verde.
- Gabarito: +4 placares curtos e +1 partida curta (humano Avançado × Médio, manual: 4-1). C++ igual: 0 falhas nos 6
  testes, gcc e clang.
- Para o Unreal: a partida da fatia usa `GamesPorSet = 4`, `PontosDoTieBreak = 10` (pedido 0002, passo 8).

## 14/10/2026 (noite) — motor sem libm: os mesmos bits no Linux e no Windows

**Feito** (decisão do Felipe: "sim, faça a troca no Core")
- `Padel.Core`: `MatematicaDeterministica` (seno, cosseno, arco-tangente, exp, log, potência; algoritmos da Cephes, só
  soma/multiplicação/divisão/raiz/floor/bits). `Util.Gaussiana` e `GolpesEspeciais` usam ela; 3^k da chiquita virou
  multiplicação exata. Testes escritos antes e vistos falhar: precisão contra o `Math` em double (≤ 3e-7; potência ≤
  2e-6) e o portão que barra a libm nos arquivos do motor (falhou listando as 14 chamadas). Core: 649 testes verdes;
  Arena (Godot): `verificar_tudo.sh` com os 11 passos verdes. Commit `86ed5be` no `padelizou`.
- Gabarito regerado (inclui `matematica.csv`: 25 mil entradas e saídas em bits). Antes de portar, o C++ reprovou onde
  devia (golpes especiais 100 falhas, partidas 42; física, `PassoFixo`, placar e árbitro seguiram com 0).
- `padel::mat` em C++ (`Matematica.cpp`), espelho linha a linha; `Aleatorio` e golpes especiais usam ela; o
  `TestesDaMatematica` confere as funções e barra a libm no motor. Todos os 6 testes com 0 falhas em gcc e clang,
  Release e Debug.
- CI: o MSVC volta a bloquear tudo (sem passo informativo), e o Linux sai da versão fixa da glibc.
- As partidas do gabarito mudaram um pouco (ex.: a 3, humano Intermediário × Médio, foi de 6-3 a 6-4): o sorteio e os
  remates mudaram no último dígito. O equilíbrio medido (`REALISMO.md`) não foi refeito; é estatístico e não deve
  mudar.

## 14/10/2026 (tarde) — o motor inteiro em C++, igual ao Padel.Core passo a passo

**Feito**
- A partida inteira portada: placar (ponto de ouro, vantagem, tie-break, pontos corridos), árbitro, jogador (inércia,
  balanço, contato no instante), IA (leitura com erro, formação, escolha de golpe), golpes especiais (chiquita,
  contrapared, remates por 3 e por 4, com o orçamento de sub-passos), partida (máquina de estados, golpe do humano pela
  entrada, modo Manual e Automático), humano simulado e `PassoFixo` (quadro → passos de 120 Hz).
- Gabarito gravado pelo `Padel.Core` em `f88ae65`: 7 partidas inteiras (IA × IA nas três dificuldades, humano
  simulado no manual e no automático, pontos corridos, vantagem — 370 mil passos), 199 chamadas de golpe especial,
  296 de `VelocidadeDe`, 5 sequências de quadros do `PassoFixo`, e — o que as partidas não alcançam — 14 placares
  (tie-break, até 3 sets, vantagem, pontos corridos com empate) e 400 sequências do árbitro (falta, let, dupla falta,
  parede própria, fora da caixa). Resultado, gcc e clang, Release e Debug: PadelSim 115.573 / GolpesEspeciais 4.742 /
  Partidas 9.961 / PassoFixo 31.034 / Placar e árbitro 6.056 verificações, **0 falhas**. Cada teste foi visto falhar
  (esboço vazio; `Avancar` desligado; mutações: `dx/d` no último bit → 48, tie-break em 6 → 103, let como falta → 19).
- Rotinas (`verificar`, `regerar-gabarito`, `pedido-sessao-local`, `triar-playtest`), formulário de playtest, hook de
  início de sessão que compila o PadelSim na nuvem.
- Pedido 0002 (quadra cinza jogável) escrito para depois do 0001.

**Decisão (tomada às 20h, ver a entrada da noite) — determinismo entre plataformas**
- A IA e os golpes especiais usam `logf`, `cosf`, `sinf`, `expf`, `powf` e `atan2f`. A glibc e a UCRT da Microsoft
  dão bits diferentes; até versões da glibc diferem (a 2.41 começou a trocar funções float por versões corretamente
  arredondadas). Hoje: o jogo online não sofre (o Unreal replica do servidor), mas a prova bit a bit só vale no Linux
  com a glibc 2.39 — no Windows, as partidas do gabarito são informativas.
- **Medido no CI (MSVC, run 38631675205, 14/10):** compila limpo com `/W4 /WX`; bola, `PassoFixo`, placar e
  árbitro dão 0 falhas. Nos golpes especiais, 12 falhas em 4.742 verificações, todas no remate por 3 e por 4 (os que
  usam `sinf`/`cosf`/`atan2f`), de 1 a 6 ulps (pior: 9,5e-07 m/s); a chiquita (`expf`/`powf`) e a contrapared batem
  inteiras, e o orçamento de sub-passos é igual em todas. Nas partidas, 1 das 7 bate inteira (pontos corridos 16);
  nas outras, a primeira diferença aparece entre 11 s e 2 min 15 s de jogo (passo 1.320 a 16.200), também no último
  dígito (ex.: velocidade de um jogador 0,7803393 contra 0,7803391).
- Saída proposta: o `Padel.Core` troca essas seis funções por versões próprias, só com soma, multiplicação, divisão
  e raiz (as 17 elevações e os azimutes do remate por 3 cabem numa tabela; o remate por 4 gira um vetor com seno e
  cosseno constantes; o gaussiano pode usar o método polar, que só precisa de `log` — ou uma tabela). Muda o Core que
  o Arena (Godot) também usa: os resultados mudam no último bit, o equilíbrio não. Regerar o gabarito em seguida.

**Pendente**
- Sessão local: pedido 0001 (projeto Unreal + Automation Spec) e depois o 0002.
- Os quatro botões do design (X bandeja, Y ataque, RB parede) e o rastro-fantasma da parede: nascem no `Padel.Core`
  (entrada nova e regra nova), com teste lá, e chegam pelo gabarito.

## 14/10/2026 — etapa 1 (física) feita na nuvem; o projeto Unreal espera o PC

**Feito**
- Repositório criado pelo Felipe; design aprovado (`docs/DESIGN.md`).
- `Source/PadelSim`: bola (gravidade, arrasto, Magnus, quique com spin, vidro, grade irregular por hash, rede, portas,
  saída por cima), quadra (painéis = física) e `golpes::Calcular` — porte do `Padel.Core`.
- Gabarito: 120 lançamentos (672 eventos, inclusive 7 saídas pela porta) e 90 golpes com os 9 efeitos, gravados pelo
  `Padel.Core` em `105ca75`. Resultado: **desvio zero** em posição, velocidade e spin; 115.573 verificações, 0 falhas,
  em gcc e clang, Release e Debug. Uma mutação de 0,01 na restituição do vidro gera 2.879 falhas (o teste enxerga).
- CI com gcc, clang e **MSVC** (o compilador do Unreal no Windows).
- Git LFS para o conteúdo binário (`.gitattributes`).

**Pendente**
- Pedido 0001 à sessão local: criar o projeto Unreal, ligar o módulo `PadelSim` e a Automation Spec do gabarito.
- Na nuvem, a seguir: jogador (corrida, balanço, contato no instante), árbitro e placar (ponto de ouro, set até 4,
  super tie-break) — mesmo método: gabarito do Core primeiro.
