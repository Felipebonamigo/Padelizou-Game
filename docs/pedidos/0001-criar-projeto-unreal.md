# Pedido 0001 — criar o projeto Unreal e provar o PadelSim dentro dele

> Da nuvem para a sessão local do PC, 14/10/2026. Responda no fim deste arquivo, commit e push no mesmo branch.

## Contexto

`Source/PadelSim` é o motor do jogo em C++ puro, testado fora do Unreal contra o gabarito do `Padel.Core`: a física
da bola, os golpes especiais, a partida inteira (placar, árbitro, jogadores, IA, humano simulado, controle de quatro
botões, assistência), o `PassoFixo` e o rastro-fantasma.
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
   os CSVs de `Testes/PadelSim/Gabarito/` e faz as mesmas conferências de todos os `Testes/PadelSim/Testes*.cpp` (o
   glob é a lista; em 15/10 eram 8, de matemática a golpes do humano) — reaproveite o código, não reescreva as regras. **Todos com 0 falhas**: o motor não usa libm, então o resultado
   no Windows é o mesmo do Linux — o CI já prova isso com o MSVC. Uma falha aqui é o Unreal compilando com outras
   opções de ponto flutuante (passo 4). Rode pela linha de comando
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

Respondido em 15/10/2026, sessão local do PC (Unreal 5.8.3, MSVC 14.51.36252, RTX 4070 Ti). **DONE_WITH_CONCERNS** — duas
ressalvas no fim ("O que ficou diferente").

### Ambiente

- Espaço: C: 218 GB livres, D: 271 GB (passo 1 ok). O projeto ficou em C:.
- A pasta do PC era só os arquivos, sem `.git`. Fiz `git init` no lugar, `fetch` do branch e `reset` mantendo os
  arquivos: `git status` limpo, nada sobrescrito. Git LFS 3.7.1 (`git lfs install --local` feito; `.gitattributes` já
  manda `*.umap` para o LFS, conferido com `git check-attr`).
- CMake/Ninja não existiam no PC; instalados por `winget` (CMake 4.4.4). Fora do Unreal, MSVC, Release:
  `ctest --test-dir build -C Release` → `100% tests passed out of 8` (antes e depois das mudanças abaixo).

### Projeto e módulos

- `PadelizouGame.uproject` (UE 5.8, C++), criado à mão a partir do `TP_Blank` da engine, com `Config/`, `Content/`,
  `Source/PadelizouGame.Target.cs`, `PadelizouGameEditor.Target.cs` (`DefaultBuildSettings = V7`).
  Módulos: `PadelizouGame` (Runtime, depende de `PadelSim`), `PadelSim` (Runtime, como pedido) e `PadelSimTestes`
  (Editor, só os testes).
- `PadelSim.Build.cs` e `PadelSimModulo.cpp` compilaram sem correção além da opção de ponto flutuante (abaixo).

### Ponto flutuante: o que o UBT 5.8 faz

- Lendo `VCToolChain.cs` do 5.8: `FPSemantics` padrão = `Default` = **`/fp:fast`**. Só o alvo Editor/Program com
  `DefaultBuildSettings >= V7` vira `/fp:precise`. O alvo **Game** sairia `/fp:fast`.
- Fixei `FPSemantics = FPSemanticsMode.Precise` no `PadelSim.Build.cs` (vale para Editor e Game).
- **Prova (mutante):** com `FPSemanticsMode.Imprecise` no módulo, a Spec reprova: Matemática 987 falhas, GolpesEspeciais
  196, RastroFantasma 400, Partida 72 (287 verificações até parar), GolpesDoHumano 11; `EXIT CODE: -1`. Com `Precise`:
  0 falhas em tudo, `EXIT CODE: 0`. Ou seja, o `/fp:fast` do UBT quebra o gabarito e o `Precise` basta; nenhuma
  outra opção foi necessária (o UBT liga `/arch:AVX2`, que no MSVC inclui FMA, mas com `/fp:precise` o MSVC não
  contraiu nada: o gabarito bate bit a bit).

### Automation Spec `PadelSim.Gabarito`

`Source/PadelSimTestes/Private/Testes/PadelSimGabarito.spec.cpp`: um `It` por `Testes*.cpp` (os 8). Cada
`Private/Gabarito/RodaTestes*.cpp` faz `#define main <Nome>_Principal` + `#include` do teste original em `Testes/PadelSim/`
(sem copiar nem reescrever regra); a Spec captura o stdout do teste, joga no log e exige código de saída 0.
`PASTA_DO_GABARITO` e `PASTA_DO_MOTOR` saem do `Build.cs`.

```
UnrealEditor-Cmd PadelizouGame.uproject -ExecCmds="Automation RunTests PadelSim; Quit" -unattended -nullrhi
```

Saída (resumida; 8 de 8 `Result={Success}`, `**** TEST COMPLETE. EXIT CODE: 0 ****`):

```
Golpes do humano: 45 cenas, 0 falha(s).
GolpesEspeciais: 4742 verificações, 0 falha(s).
Matemática: 25664 verificações, 0 falha(s).
PadelSim: 115573 verificações, 0 falha(s).     (pior desvio nas trajetórias: posição 0 m, velocidade 0 m/s, spin 0 rad/s)
Partidas: 21969 verificações, 0 falha(s).      (partidas 0 a 11: "igual ao Padel.Core")
PassoFixo: 31034 verificações, 0 falha(s).
Placar e árbitro: 8256 verificações, 0 falha(s).
Rastros: 1505 verificações, 0 falha(s).
```

Os números são os mesmos do `ctest` no MSVC.

### Fumaça

- **`Coordenadas.h`** (`Source/PadelizouGame/Public/`), `PadelCoordenadas::ParaUnreal(x, y, z)` / `ParaPadel(...)`:
  **Unreal X = y do PadelSim × 100, Unreal Y = x do PadelSim × 100, Unreal Z = z × 100** (m → cm). Trocar x e y
  inverte a mão do sistema, que é o que leva o PadelSim (destro) ao Unreal (canhoto) sem espelhar a quadra.
  Ex.: (0, 9, 1) m → (900, 0, 100) cm; "rumo à rede" = X do Unreal diminuindo.
- **Spec `PadelizouGame.Fumaca`** (3 testes, escrita antes: falhou por "`Coordenadas.h` não existe"; o teste da bola
  falhou uma vez por parâmetro meu — Vz=3 batia na rede — e passou com Vy=-10, Vz=5). Confere a conversão (ida e
  volta) e que a `padel::Bola` lançada de (0, 9, 1) a 60 quadros por segundo só diminui o X do Unreal e cruza a rede
  em menos de 1 s.
- **`APadelFumacaAtor`** + mapa **`/Game/Mapas/Fumaca`** (chão 20×10 m, luz, câmera lateral). O mapa sai de
  `Ferramentas/criar_mapa_fumaca.py` (reproduzível; `Content/Mapas/Fumaca.umap` vai pelo LFS). Rodado sem tela
  (`UnrealEditor.exe ... /Game/Mapas/Fumaca -game -nullrhi`), o log do ator mostra o motor a 60 passos/s e o eixo certo:

```
t=0.03 s, quadros=2,   UE(cm)=(866.8, 0.0, 116.0),   PadelSim(m)=(0.000, 8.668, 1.160)
t=1.00 s, quadros=60,  UE(cm)=(-6.2, 0.0, 90.6),     PadelSim(m)=(0.000, -0.062, 0.906)   ← passa a rede
t=3.02 s, quadros=181, UE(cm)=(-948.3, 0.0, 33.5),   PadelSim(m)=(0.000, -9.483, 0.335)   ← vidro de fundo
```

### O que ficou diferente do pedido e por quê

1. **Mudei `Source/PadelSim/Public` (só forma, nenhuma regra):** o módulo vira DLL no Editor e o link falhou (50
   símbolos não resolvidos). Adicionei `PadelSim/Exportar.h` (`PADELSIM_API`, vazia fora do Unreal), pus a macro nas
   classes/structs e funções livres de 15 headers e separei as estáticas de declaração múltipla (`Bola.h`, `HumanoSimulado.h`)
   (`A = 1, B = 2;` → uma por linha), que o MSVC recusa em classe exportada (C2487). ctest MSVC: 8/8 depois.
   **A nuvem precisa saber:** classe ou função livre nova do motor leva `PADELSIM_API`, senão o CMake passa e o link
   do Unreal falha; e nada de `static constexpr float A = .., B = ..;`. Se preferir, revise antes de mesclar.
2. **Spec em módulo próprio (`PadelSimTestes`)**, não em `PadelizouGame/Private/Testes/`: os testes precisam de
   exceções e RTTI (`std::regex`) e de unity desligado; num módulo de Editor isso não contamina o jogo. A Spec da
   fumaça ficou em `PadelizouGame/Private/Testes/`.
3. **O projeto não passou pelo assistente do Unreal** (cópia do `TP_Blank` à mão). `ModelingToolsEditorMode` ficou como
   no template.
4. **Compilação com `-NoHotReloadFromIDE`:** há dois editores meus abertos (Detonauto e Zombie) usando o mesmo
   `UnrealEditor.exe`, e o UBT recusa compilar com Live Coding ativo ("Unable to build while Live Coding is active").
   Não fechei nenhum. Só o alvo **Editor** foi compilado; o alvo Game (`PadelizouGame Win64`) não foi.
5. **Não anotei nenhum defeito do motor.** MSVC 14.51 avisa "newer than latest preferred version 14.50"; sem efeito.

### Como abrir

Duplo clique em `PadelizouGame.uproject` (ou `UnrealEditor.exe PadelizouGame.uproject`), abra `Mapas/Fumaca` e dê Play.
Automation: Window → Test Automation → `PadelSim` e `PadelizouGame`.
