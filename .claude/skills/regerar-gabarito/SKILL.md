---
name: regerar-gabarito
description: Regera os CSVs do gabarito (Testes/PadelSim/Gabarito) a partir do Padel.Core, quando a física, a IA, o placar ou a partida mudaram no motor C#. Use quando o Felipe pedir mudança de regra ou de física, quando o Padel.Core mudar, ou quando um teste de gabarito reprovar depois de uma mudança intencional no Core.
---
# Regerar o gabarito

O gabarito é a verdade: trajetórias, golpes, golpes especiais, partidas inteiras e o PassoFixo, gravados pelo
`Padel.Core` (C#) do repositório `Felipebonamigo/padelizou`, pasta `padelizou-arena/`. O C++ daqui reproduz bit a bit.

## Ordem (nunca o contrário)

1. Mude o `Padel.Core` primeiro, com teste no `Padel.Core.Tests` visto falhar antes (regras do CLAUDE.md de lá).
2. Regere, a partir do repositório `padelizou`, gravando direto aqui:
   ```bash
   dotnet run --project padelizou-arena/ferramentas/Gabarito -c Release -- ../padelizou-game/Testes/PadelSim/Gabarito $(git rev-parse --short HEAD)
   ```
   A primeira linha de cada CSV diz o commit do Core. O gerador é determinístico: rodar de novo sem mudar o Core não
   muda nenhum byte além dessa linha.
3. Rode a skill `verificar`: o C++ deve REPROVAR onde o Core mudou. É a prova de que o teste enxerga a mudança.
4. Porte a mudança para o C++ (mesma ordem de operações em float) até zerar as falhas.
5. Commite o gabarito e o C++ juntos, citando o commit do Core.

## O que cada arquivo do gerador grava

| Arquivo em `ferramentas/Gabarito/` | CSVs |
|---|---|
| `Program.cs` | `casos.csv`, `trajetorias.csv`, `eventos.csv` (bola), `golpes.csv` (golpes::Calcular), `constantes.csv` |
| `GabaritoDosGolpesEspeciais.cs` | `golpes-especiais.csv`, `velocidade-de.csv` |
| `GabaritoDaPartida.cs` | `partidas.csv`, `partidas-retratos.csv`, `partidas-eventos.csv` |
| `GabaritoDoPassoFixo.cs` | `passo-fixo.csv` |
| `GabaritoDoPlacarEDoArbitro.cs` | `placar.csv`, `arbitro.csv` |
