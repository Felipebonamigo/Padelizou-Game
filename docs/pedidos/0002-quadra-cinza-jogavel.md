# Pedido 0002 — quadra cinza jogável (etapa 2 da fatia, semanas 3–4)

> Da nuvem para a sessão local do PC, 14/10/2026. Faça **depois** do pedido 0001 (projeto criado e
> `PadelSim.Gabarito` verde no editor). Responda no fim deste arquivo, commit e push no mesmo branch.

## Contexto

O `PadelSim` já tem a **partida inteira** em C++ puro, igual ao `Padel.Core` passo a passo (7 partidas de gabarito,
inclusive com o humano simulado): `padel::Partida` (placar, árbitro, bola, jogadores, IA, golpes especiais) e
`padel::PassoFixo` (tempo do quadro → passos de 120 Hz sem deriva e sem perder aperto). Esta etapa é **só a casca**:
desenhar, ler o controle e chamar o `Avancar`. Nenhuma regra nova no Unreal.

Critério de pronto (do `docs/DESIGN.md`): **uma partida inteira de ponta a ponta, contra a IA, com quatro cápsulas,
câmera de TV, drive, lob e saque, e o placar.**

## O que fazer

1. **`APadelPartidaAtor`** (C++, `Source/PadelizouGame/`): dono de um `padel::Partida` e de um `padel::PassoFixo(120)`.
   No `Tick(DeltaSeconds)`: `n = passo.Quadro(DeltaSeconds, entradaLida)`, e para `i` em `0..n-1`:
   `entradas[0] = passo.EntradaDoPasso(i)`; `partida.Avancar(1.f/120.f, entradas)`. Nada de `Avancar(DeltaSeconds)`
   direto: o passo é fixo, ou o jogo deixa de ser o do gabarito.
2. **Coordenadas num lugar só** (`Coordenadas.h`, do pedido 0001): PadelSim em metros (x largura, y comprimento, z
   altura) → Unreal em centímetros. Todo ator lê a posição por ali.
3. **Quadra cinza:** piso 20 × 10 m com as linhas (`quadra::LinhaDeSaque` = 6,95 m), a rede (0,90 m) e as paredes
   **desenhadas de `quadra::Paineis()`**: vidro semitransparente e grade em material cinza escuro. Nenhuma medida de
   parede escrita à mão.
4. **Quatro cápsulas** (1,8 m) nas posições de `partida.Jogadores[i]`, cor por time, olhando pra rede; um disco na
   mão como raquete; um anel no chão para o jogador local. A bola: esfera de 6,7 cm amarela, com sombra.
5. **Câmera de TV:** atrás do time da casa (y > 0), lente equivalente a 35 mm (FOV horizontal ~54°), 7,5 m de altura,
   17,5 m de recuo da linha de fundo, olhando o centro da quadra, seguindo a bola só no eixo lateral, com atraso suave.
6. **Controle (Enhanced Input)**, no referencial do jogador (Dy < 0 = rumo à rede, Dx > 0 = direita dele):
   analógico esquerdo/WASD → `Dx, Dy`; **A/Espaço** → `AcaoPressionada` no quadro do aperto e `AcaoSegurada` enquanto
   apertado; **B/Shift** → `LobPressionada`; **X/Q** → `CortadoPressionado`; **Y/E** → `AtaquePressionado` no quadro do
   aperto e `AtaqueSegurado` enquanto apertado; **RB/Ctrl** → `ModificadorSegurado` enquanto apertado. Teclado e controle.
   A partida usa `OpcoesDaPartida{ .Controle = Controle::QuatroBotoes }` (o PadelSim já tem, desde 15/10): o botão que
   começa o balanço escolhe o golpe — A chapado (segurado: ataque), B lob, X bandeja na bola alta e slice na baixa, Y
   smash na alta (segurado: por 3/por 4; com RB: víbora) e ataque na baixa — e o analógico só a direção.
7. **Modo de golpe:** opção no menu de debug entre `Manual` com quatro botões (o timing é seu; o padrão) e `Automatico`
   (bate sozinho), que é o "Casual" provisório do design. O `Automatico` só existe com `Controle::DoisBotoes` (a
   Partida recusa a outra combinação com `std::invalid_argument`): no de dois botões, frente/trás/lado no analógico é que
   escolhem o golpe. `Partida::Opcoes` é `const`: trocar o modo é criar uma `Partida` nova (a próxima partida).
   Assistência (`Jogador::Assistencia`: Pro, Padrão, Casual) muda a qualquer hora, direto no jogador.
8. **Placar de TV** mínimo (UMG): pontos com `Placar.TextoDosPontos(time)`, games e sets, a `Mensagem` da partida
   quando houver. Fim de partida: tela com `Placar.Resumo()` e "jogar de novo". A partida da fatia é o **formato
   curto** do design: `OpcoesDaPartida{ .GamesPorSet = 4, .PontosDoTieBreak = 10 }` (set até 4, super tie-break a 10).
9. **Eventos:** ligue `partida.AoEvento` a um log na tela por 2 s (Golpe com o tipo, Quique, Parede, Ponto e motivo).
   Serve de depuração agora e de gancho do som e da animação depois.
10. **Prova:** jogue uma partida inteira no Manual contra o Médio e outra no Automático. Cole o placar final e salve duas
    capturas (rally em andamento e tela de fim) em `docs/imagens/`. Meça os ms por quadro do `Tick` do ator com
    `stat game`/Insights: a meta é a partida custar menos de 1 ms por quadro.

## Fora deste pedido

Arte, MetaHumans, animação, som, menus de verdade, online. Nenhuma mudança de regra no Unreal: regra muda no
`Padel.Core` e chega pelo gabarito.

## Resposta da sessão local

Respondido em 15/10/2026, sessão local do PC. **DONE_WITH_CONCERNS** — ressalvas em "O que ficou diferente".

### O que ficou pronto

Mapa `/Game/Mapas/QuadraCinza` (o padrão do projeto). Tudo nasce de um ator, `APadelPartidaAtor`:

| Item do pedido | Onde |
|---|---|
| 1. Ator dono da `Partida` + `PassoFixo(120)` | `FPadelSimulacao` (`PadelSimulacao.h/.cpp`): `n = passo.Quadro(...)`, `Avancar(1/120)` por passo, `EntradaDoPasso(i)` no jogador 0. O ator só chama `Quadro()` (quadro lento limitado a 0,25 s). |
| 2. Coordenadas num lugar só | `Coordenadas.h` (pedido 0001). Todo ator lê por ali. |
| 3. Quadra cinza | Piso 20×10 m, linhas de 5 cm (fundo, laterais, saque a 6,95 m, central), rede de 0,90 m e paredes **todas de `quadra::Paineis()`** (`PadelPaineis.h`): vidro semitransparente (`M_Vidro`) e grade cinza escuro. Nenhuma medida de parede à mão. |
| 4. Quatro cápsulas, raquete, anel, bola | Cilindro + cabeça por jogador (azul = casa, vermelho = rival), nariz mostra a frente, disco como raquete que varre no balanço, anel amarelo no jogador 0, bola amarela com sombra. Posições de `partida.Jogadores[i]`. |
| 5. Câmera de TV | FOV horizontal 54°, 7,5 m de altura, 17,5 m atrás da linha de fundo da casa, olha o centro; segue a bola só no eixo lateral (55 % do x, atraso suave de 0,45 s). |
| 6. Controle (Enhanced Input) | `UInputAction`/`UInputMappingContext` criados em C++: WASD/analógico, A/Espaço, B/Shift, X/Q, Y/E, RB/Ctrl, teclado e controle. `OpcoesDaPartida{ .Controle = QuatroBotoes }`. `PadelControle.h` monta a `Entrada` no referencial do jogador. |
| 7. Modo de golpe | **F2** troca Manual (4 botões) ↔ Automático (2 botões) **na próxima partida** (a `Partida` é nova: `Opcoes` é `const`); **F3** troca a assistência (Pro/Padrão/Casual) direto no jogador, na hora. |
| 8. Placar de TV (UMG) | `UPadelHudWidget`, montado em C++: pontos (`TextoDosPontos`), games e sets, bolinha no sacador, `Mensagem` da partida, "SUPER TIE-BREAK", tela de fim com `Placar.Resumo()` e "R / Start: jogar de novo". Formato curto: `GamesPorSet = 4`, `PontosDoTieBreak = 10`. |
| 9. Eventos | `AoEvento` vira log na tela por 2 s (golpe com o tipo, quique, parede, ponto e motivo, falta, let, game, set, partida). |

### Testes (escritos antes, vistos falhar)

`PadelizouGame.Casca`, 8 testes (falhou por "header não existe"; depois por tolerância de 1e-6 cm em `float`, corrigida):
60 quadros de 1/60 s = 120 passos; quadro lento de 0,5 s = 60 passos; quadros irregulares sem deriva (1 s = 120 passos); partida inteira
IA×IA do formato curto termina e dá o mesmo placar e os mesmos passos com a mesma semente; cada painel de `Paineis()` vira caixa
por fora da parede certa; W+D = `Dx > 0, Dy < 0` e diagonal ≤ 1; A/B/X/Y/RB nos campos certos; solto não manda nada.

Tudo junto, no mesmo turno desta resposta: `ctest` MSVC **8/8**; `Automation RunTests PadelSim+PadelizouGame` → **19 de 19 `Success`,
`EXIT CODE: 0`** (8 do gabarito, 3 da fumaça, 8 da casca).

### Prova de partida (item 10)

Rodei com render (RTX 4070 Ti, 1280×720, `-benchmark -fps=60` para passo de 1/60 s) e semente 42, contra a IA **Médio**, formato curto:

```
UnrealEditor.exe PadelizouGame.uproject /Game/Mapas/QuadraCinza -game -windowed -ResX=1280 -ResY=720 -benchmark -fps=60 \
  -PadelHumanoSimulado -PadelCapturas -PadelSairNoFim -PadelSemente=42 [-PadelModo=auto]
```

| Modo | Placar final | Tempo de jogo | Tick do ator |
|---|---|---|---|
| Manual, 4 botões | **5-4(2)**, a casa venceu (set 4-4 e super tie-break) | 711,0 s (42.628 quadros medidos) | média **0,047 ms**, máximo 0,865 ms |
| Automático, 2 botões | **4-0**, a casa venceu | 326,5 s (19.594 quadros) | média **0,046 ms**, máximo 0,724 ms |

Meta (< 1 ms por quadro): cumprida. O Manual repete com a mesma semente (5-4(2) e 711,0 s nas duas rodadas). Na 1ª rodada, com cache de shader frio,
o máximo foi 1,462 ms (quadros de compilação).
Capturas em `docs/imagens/` (LFS): `rally-manual.png`, `fim-manual.png`, `rally-automatico.png`, `fim-automatico.png`.

**Controle de verdade:** com `-PadelTesteDeEntrada` o ator injeta teclas no `PlayerInput` e loga a `Entrada` que o Enhanced Input entrega.
Resultado: W+D → `Dx=0.71 Dy=-0.71`; Espaço → `A=1 Aseg=1` (um quadro de aperto; segurado até soltar); Shift → `B=1`; E → `Y=1 Yseg=1` (segura até soltar);
Ctrl → `RB=1`; Q → `X=1`; soltar zera tudo.

### O que ficou diferente do pedido e por quê

1. **Quem jogou as partidas da prova foi o `padel::HumanoSimulado` (perfil Avançado), não um humano de mãos.** Sem a mão do Felipe eu não consigo
   apertar botão no tempo. É a mesma `Entrada` do controle (`-PadelHumanoSimulado`), então exercita casca, câmera, HUD, placar e medição, mas **não**
   o "sentir" do controle. O teclado foi provado por injeção de tecla (acima); o **controle físico (gamepad) não foi testado**, só mapeado com os mesmos
   botões. Falta o Felipe jogar com a mão e dizer como está.
2. **Medição:** `FPlatformTime` dentro do `Tick` do ator (simulação + desenho + HUD), ignorando os 180 primeiros quadros, não `stat game`/Insights.
3. **Bola 2,5× maior no visual** (`EscalaVisualDaBola`, padrão 2,5; `1` dá os 6,7 cm reais): a 27 m da câmera a bola real tem ~3 px e some.
4. **"Jogar de novo" e o menu de debug são teclas** (R/Start; F2 modo; F3 assistência), não botões clicáveis de UMG. Menu de verdade fica para a etapa de arte.
5. **Materiais, mapa e GameMode:** `M_Cor`/`M_Vidro` e os mapas saem de `Ferramentas/*.py` (reproduzíveis). `APadelGameMode` não tem pawn.
6. **Alvo Game não compilado** (só Editor), como no 0001; os dois editores abertos continuam bloqueando o build normal (`-NoHotReloadFromIDE`).
7. **Visual a melhorar depois (não regra):** a faixa de grade escura da parede de fundo da casa cobre parte dos jogadores de perto; as cápsulas são cilindros.
   Nenhum defeito do motor encontrado: placar, tie-break, árbitro e IA se comportaram como o gabarito.

### Como abrir e jogar a quadra cinza

1. Duplo clique em `PadelizouGame.uproject` (Unreal 5.8; o mapa padrão já é `QuadraCinza`). Aguarde "Compiling shaders" na 1ª vez.
2. **Play** (Alt+P). Você é o azul com o anel amarelo, contra o Médio.
3. Controle: **WASD** ou analógico esquerdo; **Espaço/A** saca e bate (chapado; segurado = ataque); **Shift/B** lob; **Q/X** cortado; **E/Y** ataque/smash;
   **Ctrl/RB** modificador; **F2** troca Manual ↔ Automático na próxima partida; **F3** assistência; **R/Start** jogar de novo.
4. Sem editor, partida simulada: o comando da seção "Prova de partida" acima.

### Ajustes depois do playtest do Felipe (15/10/2026)

Retorno: "não dá para enxergar direito por causa da quadra, os bonecos estão feios, A e D estão com os lados invertidos, achei muito duro e não consegui analisar o jogo".

1. **A/D invertidos — bug meu na conversão, não do motor.** No motor, `Dx > 0` do time da casa move para +x (`Partida::Avancar`: `Mover(e.Dx * j.Lado, ...)`, `Lado = +1`), e a câmera atrás da casa olha para -X do Unreal, onde a direita da tela é -Y. A conversão do 0001 punha Y = +x. **Agora `Coordenadas.h` é X = y, Y = -x, Z = z** (todos ×100). Teste antes: `Fumaca` e `Casca` passaram a exigir Y = -x; mutante (voltar ao +x) reprova `metros do PadelSim viram centímetros...` e `volta ao PadelSim...`. A resposta do 0001 que diz "Unreal Y = x" vale só até este ajuste.
2. **Quadra tapando a visão:** a grade escura era opaca e a parede de perto cobria a quadra. Agora a grade também é translúcida (`M_Vidro` escuro, opacidade 0,28; vidro 0,14).
3. **Bonecos:** cápsula de verdade (cilindro de 1,30 m + meia esfera de 50 cm em cada ponta = 1,8 m) com viseira escura na frente.
4. **Duro demais:** a assistência inicial passou de Pro para **Casual** (janela de ±40 ms no timing; muda com F3 a qualquer hora). Novo **F4**: câmera lenta 100 % → 50 % → 25 % (`SetGlobalTimeDilation`; o passo fixo de 120 Hz segue valendo), para analisar o jogo.

Reverificado neste turno: `PadelizouGame.*` 11 de 11 `Success`; partida simulada de novo no Pro com semente 42: `5-4(2)`, 711,0 s, Tick do ator com média de 0,051 ms (máximo 0,928 ms) — a conversão não mexeu na simulação. Capturas refeitas em `docs/imagens/`.

### Segundo retorno do playtest (15/10/2026)

Retorno: "A e D funcionaram, mas a jogabilidade está muito estranha: a quadra se mexendo, não gostei; parece que ao apertar o botão para bater na bolinha não vai".

1. **"A quadra se mexendo":** era a câmera seguindo a bola de lado (item 5 do pedido, com 55 % do x da bola). Agora é `SeguimentoLateralDaCamera`, **padrão 0 (câmera parada)**; o acompanhamento fica na propriedade, para quando for desejado.
2. **"O botão não bate":** não era bug de entrada. Provado injetando teclas no jogo (`-PadelTesteDeEntrada`, semente 42, assistência Casual): no **Manual**, Espaço saca (`Jogador 0: saque`) e Shift começa o balanço (`Balanço`), que, fora de hora, termina em `Errou`; no **Automático**, o mesmo aperto vira `drive` sozinho. O Manual exige apertar ~0,12 s antes de a bola chegar ao ponto de contato (janela de ±40 ms no Casual), e o jogo não mostrava o erro (os eventos `Balanço` e `Errou` não iam para a tela).
3. **Mudanças:** (a) o modo padrão passou a **Automático** (F2 volta ao Manual; `-PadelModo=manual` na linha de comando); (b) o `Balanço` e o `Errou: a raquete passou sem pegar a bola` do jogador local aparecem no log da tela; (c) a raquete visual ficou maior (50 cm).
4. **Em aberto (decisão do Felipe):** o timing do Manual. Opções: alargar a janela do Casual (regra, vem do `Padel.Core`), um indicador visual do ponto/hora do contato, ou manter o Automático como padrão até a etapa de feel.

### Indicador de quando apertar (Manual) — 15/10/2026

Decisão do Felipe: indicador visual no Manual; **Automático segue como padrão**.

- **Como funciona** (`PadelIndicador.h/.cpp`): copia a bola e o corpo do jogador local, avança a física do PadelSim (até 2 s) e acha o passo em que o motor deixaria a raquete bater (`Alcanca` + `PodeBaterAgora`, as mesmas funções da `Partida`); o aperto tem de começar `MomentoIdealDoBalanco` (0,12 s) antes. Só lê a `Partida`; nenhuma regra nova. Supõe o corpo parado: se você anda, a previsão se refaz a cada quadro.
- **O que aparece (só no Manual):** um disco no chão, no ponto onde a bola vai estar no contato, que encolhe de 2 m a 50 cm até a hora de apertar; texto "aperte em 0,4 s" em amarelo, e **"APERTE!" em verde** dentro da janela da assistência (no mínimo ±50 ms, para dar para ver). Passou da hora, some. Captura: `docs/imagens/indicador-manual.png`.
- **Testes** (`PadelizouGame.Indicador`, escritos antes: falharam por "header não existe"): sem bola vindo não há previsão; sem apertar nada o jogador devolve 0 bolas; **apertando só quando o indicador manda** (corpo andando como o `HumanoSimulado`, 240 s de jogo, Casual): **42 balanços, 41 devoluções, 1 erro**. Mutante (tirar os 0,12 s): 28 balanços e 25 devoluções, a Spec reprova. (A 1ª versão do teste era frouxa e o mutante passava; apertei o limite para ≥ 35 antes de contar.)
- **Verificado neste turno:** `PadelSim+PadelizouGame` 22 de 22 `Success`; `ctest` MSVC 8/8; partida simulada no Manual com o indicador ligado: `5-4(2)`, 711,0 s, Tick do ator média 0,057 ms (máximo 0,991 ms).
- **Limite:** o indicador diz *quando*, não *onde*: se o corpo não está perto da bola, o balanço erra do mesmo jeito. O disco mostra o ponto, e o resto é andar até ele.

### Rastro-fantasma da saída de parede e cor no pé (15–17/10/2026, sem pedido: itens da etapa 5–6 que só dependem do Unreal)

- **Rastro-fantasma** (pilar 1), pela assistência do jogador local, como no `DESIGN.md`: **Casual** sempre, a partir da primeira batida no vidro já quicada; **Padrão** só quando a bola bate no vidro do lado dele (o rastro tirado nesse momento, inteiro, congelado até a bola ser batida); **Pro** nada. A previsão é `padel::rastro_fantasma::Prever` (com gabarito); `PadelRastro.h` só escolhe o trecho. Pontos ciano a cada 50 ms (instâncias de esfera), o último maior. Capturas: `docs/imagens/rastro-casual.png`, `rastro-padrao.png`.
- **Cor no pé:** o anel do jogador local fica verde, amarelo ou vermelho por 1,2 s depois de cada golpe, pelo `UltimoErroDoHumano` (0 a 1,6). Os pontos de corte são visuais, não regra.
- **Testes** (`PadelizouGame.Rastro`, 8, escritos antes: falharam por "header não existe"): Pro nunca; Casual da batida ao fim; Casual sem saída pela frente nada; Padrão só na batida e inteiro; rastro vazio nada; **com a física de verdade** (bola que cruza a rede, quica e bate no vidro de fundo: 228 pontos, saída a partir do 128); cor verde/amarelo/vermelho e monotônica.
- **Verificado neste turno:** `ctest` MSVC 8/8; `PadelSim+PadelizouGame` **30 de 30** `Success`, `EXIT CODE: 0`. Partidas simuladas com render: Casual `4-1` (409,8 s) e Padrão `4-0` (258,2 s); Tick do ator com média de 0,06 a 0,07 ms. Os picos (1,3 a 2,0 ms) coincidem com os quadros de captura de tela e com a previsão do Casual a cada quadro (até 360 passos).
- **Pegadinha:** `M_Cor` precisou de `used_with_instanced_static_meshes` (no `Ferramentas/criar_materiais.py`), senão o rastro saía escuro.
- **Limites:** o Padrão tira o rastro no quadro seguinte à batida (até alguns passos depois do evento); o Pro não mostra nada; o 2×2 local com controles e os 6 golpes com animação continuam para as próximas etapas (precisam do Felipe e de captura).
