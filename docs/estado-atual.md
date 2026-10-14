# Estado atual

Diário em ordem cronológica reversa: o mais novo em cima.

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

**Decisão pendente (Felipe) — determinismo entre plataformas**
- A IA e os golpes especiais usam `logf`, `cosf`, `sinf`, `expf`, `powf` e `atan2f`. A glibc e a UCRT da Microsoft
  dão bits diferentes; até versões da glibc diferem (a 2.41 começou a trocar funções float por versões corretamente
  arredondadas). Hoje: o jogo online não sofre (o Unreal replica do servidor), mas a prova bit a bit só vale no Linux
  com a glibc 2.39 — no Windows, as partidas do gabarito são informativas.
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
