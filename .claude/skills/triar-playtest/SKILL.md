---
name: triar-playtest
description: Ler, organizar e resolver os problemas de playtest do Padelizou Game que chegam como Issues "[Playtest]" neste repositório. Use quando o Felipe disser que jogou, mandar uma lista de problemas do jogo ou pedir para ver as Issues.
---
# Triar o playtest

As Issues de playtest usam o formulário `.github/ISSUE_TEMPLATE/playtest.yml` (título "[Playtest] ...", rótulo `bug`).
Se o Felipe mandar a lista pelo chat, abra uma Issue por item antes de começar.

1. Liste as Issues abertas com "[Playtest]" neste repositório.
2. Ordene pela gravidade: trava, quebra a partida, atrapalha, incômodo, sugestão. Sugestão vira item do
   `docs/DESIGN.md` ou do `docs/estado-atual.md`, não defeito.
3. Reproduza antes de mexer. Regra de jogo e física se reproduzem sem Unreal, com a semente da partida, num teste do
   `Padel.Core` (se o defeito é de regra, ele está lá também) ou do PadelSim. Visual e controle: pedido à sessão local.
4. Corrija com teste visto falhar antes. Defeito de regra ou física se corrige no `Padel.Core` primeiro (skill
   `regerar-gabarito`).
5. Verifique (skill `verificar`), commit pequeno com "fecha #N", comente na Issue o commit numa frase com o rodapé do
   Claude Code.
6. O playtest da fatia pergunta "parece padel?" e "pede mais uma?" antes de "é bonito?" (`docs/DESIGN.md`).
