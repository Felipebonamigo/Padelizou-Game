# Padelizou Game

Jogo de padel 2×2 no Unreal Engine 5: **"Virtua Tennis do padel"** — física da bola de verdade, controle generoso,
as paredes como estrela, visual de transmissão. Estúdio de uma pessoa (Felipe). **Leia antes de mexer:**
`docs/DESIGN.md` (o que é o jogo e o plano de 12 semanas) e o topo de `docs/estado-atual.md` (onde paramos).

## Regras para não regredir

1. **Todo defeito corrigido vira teste — escrito ANTES da correção, e visto falhar** pelo motivo certo.
2. **Nada é enviado com teste vermelho.** Fora do Unreal: o comando de "Rodar" abaixo. No Unreal: a Automation Spec.
3. **Uma coisa de cada vez, até o fim.** Fechou um trabalho: commit + push.
4. **Três correções seguidas falhando: pare e questione a arquitetura.** Volte à causa raiz.
5. **O motor reproduz o gabarito bit a bit, em qualquer máquina.** Nada de FMA nem fast-math (`CMakeLists.txt`) e
   **nada de libm**: seno, cosseno, arco-tangente, exponencial, logaritmo e potência saem de `padel::mat`
   (`Matematica.cpp`, porte de `MatematicaDeterministica.cs` do Core), que só usa soma, multiplicação, divisão, raiz,
   floor e bits. O `TestesDaMatematica` confere essas funções bit a bit e barra `std::sin` & cia. nos arquivos do
   motor (o Core tem o mesmo portão). Resultado: o CI exige os mesmos bits em gcc, clang e MSVC.

## A física e o gabarito

- `Source/PadelSim` é C++17 puro, sem `UObject`: compila no CMake (nuvem, CI) e como módulo do Unreal.
- É porte linha a linha do `Padel.Core` (C#, repositório `padelizou`, pasta `padelizou-arena/Padel.Core`), com a suíte
  dele (`Padel.Core.Tests`), e é o **gabarito**: `Testes/PadelSim/Gabarito/*.csv` guarda o que ele gravou — trajetórias da bola,
  golpes, golpes especiais, 9 partidas inteiras passo a passo (uma com o controle de quatro botões), o `PassoFixo`, os
  rastros-fantasma da parede e 45 cenas de golpe do humano — e o C++ reproduz **bit a bit**
  (tolerância zero; os números de hoje saem do `ctest`, skill `verificar`).
- O que existe: `Bola`, `quadra`, `golpes::Calcular`, `golpes_especiais` (chiquita, contrapared, remates por 3 e por 4),
  `Aleatorio`, `Arbitro`, `Placar`, `Jogador`, `IA`, `Partida`, `HumanoSimulado`, `PassoFixo` e `rastro_fantasma`. O Unreal só desenha,
  lê o controle e chama `PassoFixo::Quadro` + `Partida::Avancar(1/120)`. Ficaram de fora (estão no Core, não na
  fatia): rede própria (o Unreal tem a dele), perfil, conquistas, ranking, rodízio e torneio.
- **Mudar a física = mudar o `Padel.Core` primeiro** (com teste lá), regerar o gabarito e só então portar:
  ```bash
  # no repositório padelizou:
  dotnet run --project padelizou-arena/ferramentas/Gabarito -c Release -- ../padelizou-game/Testes/PadelSim/Gabarito $(git rev-parse --short HEAD)
  ```
  A primeira linha de cada CSV diz de que commit do Core ele saiu. Mudar só o C++ faz o gabarito reprovar — é o
  objetivo.
- A ordem das operações em float importa: `(a*b)*c` ≠ `a*(b*c)`. Ao portar, copie a expressão como está no C#.

## Rodar

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build && ctest --test-dir build --output-on-failure
```

## Onde cada coisa roda

- **Nuvem** (esta sessão, CI): tudo o que é `Source/PadelSim` e `Testes/`. Não há Unreal aqui.
- **Sessão local no PC** (Unreal 5.8, RTX 4070 Ti, skill `unreal-mcp`): projeto `.uproject`, atores, câmera, animação,
  visual, Automation Spec. O que a nuvem precisa do PC vai como pedido em `docs/pedidos/NNNN-*.md`; a resposta volta
  no mesmo arquivo.

## Como responder e fechar

Resultado primeiro, explicação curta. Termine cada bloco com `DONE`, `DONE_WITH_CONCERNS`, `NEEDS_CONTEXT` ou
`BLOCKED`, e nunca diga "passa" sem ter rodado o comando no mesmo turno.

## Rotinas

`.claude/skills/`: `verificar`, `regerar-gabarito`, `pedido-sessao-local`, `triar-playtest`. Problemas de playtest são
Issues "[Playtest]" com o formulário de `.github/ISSUE_TEMPLATE/playtest.yml`.

## Arte e licenças

Modelos CC0 da comunidade da Meshy já baixados ficam no repositório `padelizou` (`padelizou-arena/Padel.Godot/modelos`,
com `CREDITOS.md`). Nada com marca nem pessoa real. Binários entram pelo Git LFS (`.gitattributes`).
