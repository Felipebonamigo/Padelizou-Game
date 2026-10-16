---
name: verificar
description: O que rodar no Padelizou Game antes de dizer que uma mudança está pronta — os testes do PadelSim contra o gabarito do Padel.Core (CMake, na nuvem) e, no PC, a Automation Spec do Unreal. Use ao terminar qualquer mudança, antes do commit, ou quando perguntarem "está pronto?", "os testes passam?" ou "roda a verificação".
---
# Verificar o Padelizou Game

Nada de "pronto" sem rodar no mesmo turno e ler a saída. Zero teste vermelho para commitar.

## Na nuvem (sem o Unreal)

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build && ctest --test-dir build --output-on-failure
```

Cada executável termina com uma linha `<Área>: N verificações, 0 falha(s).` — leia as seis:
`TestesDaMatematica` (as funções sem libm, bit a bit, e o portão que barra a libm no motor), `TestesDoPadelSim` (bola, quadra, golpes::Calcular), `TestesDosGolpesEspeciais`, `TestesDaPartida` (7 partidas
inteiras, passo a passo), `TestesDoPassoFixo` e `TestesDoPlacarEDoArbitro` (tie-break, sets, falta, let).

- Tolerância é ZERO: o C++ reproduz o Padel.Core bit a bit. Uma diferença de 1 ulp é defeito, não arredondamento.
- Mexeu em física ou regra? A mudança nasce no `Padel.Core` (repositório `padelizou`) com teste lá, e o gabarito
  é regerado (skill `regerar-gabarito`). Mudar só o C++ faz o gabarito reprovar — é o objetivo.
- O CI roda gcc, clang e MSVC e exige os mesmos bits nos três. Se só um cair, procure o que depende da máquina:
  libm que voltou (o `TestesDaMatematica` acusa), FMA ou fast-math ligados, ou ordem de operação diferente do C#.

## No PC (Unreal)

A Automation Spec `PadelSim.Gabarito` roda os mesmos CSVs dentro do editor (pedido 0001), e `PadelizouGame.*` cobre a casca (passo fixo,
painéis, controle, fumaça, indicador). Compile antes (comando no `CLAUDE.md`, editor do PadelizouGame fechado) e rode:
`UnrealEditor-Cmd PadelizouGame.uproject -ExecCmds="Automation RunTests PadelSim+PadelizouGame; Quit" -unattended -nullrhi`.
Leia `Test Completed ... Result={Success|Fail}` de cada teste e `**** TEST COMPLETE. EXIT CODE: 0 ****`; 22 testes hoje.
