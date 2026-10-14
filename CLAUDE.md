# Padelizou Game

Jogo de padel 2×2 no Unreal Engine 5: **"Virtua Tennis do padel"** — física da bola de verdade, controle generoso,
as paredes como estrela, visual de transmissão. Estúdio de uma pessoa (Felipe). **Leia antes de mexer:**
`docs/DESIGN.md` (o que é o jogo e o plano de 12 semanas) e o topo de `docs/estado-atual.md` (onde paramos).

## Regras para não regredir

1. **Todo defeito corrigido vira teste — escrito ANTES da correção, e visto falhar** pelo motivo certo.
2. **Nada é enviado com teste vermelho.** Fora do Unreal: o comando de "Rodar" abaixo. No Unreal: a Automation Spec.
3. **Uma coisa de cada vez, até o fim.** Fechou um trabalho: commit + push.
4. **Três correções seguidas falhando: pare e questione a arquitetura.** Volte à causa raiz.
5. **A física é determinística.** `Source/PadelSim` só usa float, soma, multiplicação, divisão e raiz — nada de
   sin/cos/random, FMA ou fast-math. O resultado tem de sair igual em gcc, clang e MSVC (o CI confere os três) e
   igual ao gabarito.

## A física e o gabarito

- `Source/PadelSim` é C++17 puro, sem `UObject`: compila no CMake (nuvem, CI) e como módulo do Unreal.
- É porte linha a linha do `Padel.Core` (C#, repositório `padelizou`, pasta `padelizou-arena/Padel.Core`), que tem
  642 testes e é o **gabarito**: `Testes/PadelSim/Gabarito/*.csv` guarda trajetórias gravadas por ele, e o C++
  reproduz **bit a bit** (desvio zero em 115.573 verificações, 14/10/2026).
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

## Arte e licenças

Modelos CC0 da comunidade da Meshy já baixados ficam no repositório `padelizou` (`padelizou-arena/Padel.Godot/modelos`,
com `CREDITOS.md`). Nada com marca nem pessoa real. Binários entram pelo Git LFS (`.gitattributes`).
