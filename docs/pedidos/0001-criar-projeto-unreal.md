# Pedido 0001 — criar o projeto Unreal e provar o PadelSim dentro dele

> Da nuvem para a sessão local do PC, 14/10/2026. Responda no fim deste arquivo, commit e push no mesmo branch.

## Contexto

`Source/PadelSim` é o motor do jogo em C++ puro, testado fora do Unreal contra o gabarito do `Padel.Core`: a física
da bola, os golpes especiais, a partida inteira (placar, árbitro, jogadores, IA, humano simulado) e o `PassoFixo`.
Rode `ctest --test-dir build` (skill `verificar`) para ver os números de hoje. Leia `CLAUDE.md` e `docs/DESIGN.md`.

## O que fazer

1. **Antes de tudo, espaço.** O PC tinha 128 GB livres para três projetos Unreal. Se não houver ~60 GB livres no
   disco onde o projeto vai ficar, pare e avise o Felipe (o SSD externo é pré-requisito, decisão dele).
2. **Git LFS:** `git lfs install` no PC, se ainda não estiver.
3. **Criar o projeto** `PadelizouGame` (C++, em branco, sem conteúdo inicial) **na raiz deste repositório** com o
   Unreal 5.8, sem apagar nada que já existe (`Source/PadelSim`, `Testes/`, `docs/`, `CMakeLists.txt`). O módulo do
   jogo fica em `Source/PadelizouGame/`.
4. **Ligar o módulo `PadelSim`:**
   - No `.uproject`, em `Modules`: `{ "Name": "PadelSim", "Type": "Runtime", "LoadingPhase": "Default" }`.
   - No `PadelizouGame.Build.cs`: `PublicDependencyModuleNames.Add("PadelSim")`.
   - `Source/PadelSim/PadelSim.Build.cs` e `Private/PadelSimModulo.cpp` foram escritos na nuvem **sem compilar**.
     Corrija o que o UBT reclamar.
   - **Ponto flutuante:** descubra como o UBT do 5.8 compila o módulo (`/fp:fast`? FMA?). Se for "rápido", deixe o
     `PadelSim` preciso (opção do `ModuleRules` desta versão, ou `/fp:precise` no módulo). Diga no relatório o que
     encontrou.
5. **Automation Spec `PadelSim.Gabarito`** em `Source/PadelizouGame/Private/Testes/` (ou num módulo de testes): lê
   os CSVs de `Testes/PadelSim/Gabarito/` e faz as mesmas conferências dos quatro `Testes/PadelSim/Testes*.cpp`
   (reaproveite o código; não reescreva as regras). A bola e o `PassoFixo` precisam dar **0 falhas**. As partidas e os
   golpes especiais usam funções da libm (`sinf`, `cosf`, `logf`…), e a do Windows dá bits diferentes da glibc em que
   o gabarito foi gravado: rode e **relate onde a primeira diferença aparece** (partida, passo, campo) — é o dado da
   decisão pendente em `docs/estado-atual.md`. Zero falhas ali também é possível; se acontecer, diga. Rode pela linha de comando
   (`UnrealEditor-Cmd ... -ExecCmds="Automation RunTests PadelSim; Quit"`) e cole a saída.
6. **Fumaça:** um mapa vazio com uma esfera que segue `padel::Bola` a 60 quadros por segundo, lançada de
   (0, 9, 1) rumo à rede. Sem arte, sem câmera especial. Só para ver o eixo e a escala certos: o PadelSim mede em
   metros com z para cima; o Unreal mede em centímetros com Z para cima, e o eixo Y do PadelSim (comprimento da
   quadra) vira o X do Unreal. Escreva a conversão num lugar só (`PadelizouGame/Coordenadas.h`) e diga qual ficou.
7. Commit e push no branch `claude/padel-game-new-9yl0ga`. Nada de `Binaries/`, `Intermediate/`, `Saved/`
   (`.gitignore` já cobre).

## Fora deste pedido

Câmera de TV, jogadores, golpes, arte. Isso é a etapa 2, que vem como pedido 0002 depois desta resposta.

## Resposta da sessão local

(vazio)
