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

(vazio)
