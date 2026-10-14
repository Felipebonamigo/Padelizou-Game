---
name: pedido-sessao-local
description: Escreve um pedido para a sessão local do PC (a que tem o Unreal 5.8, a placa de vídeo e o unreal-mcp) e lê a resposta. Use quando a tarefa precisar do editor do Unreal, compilar o projeto, jogar, capturar a tela do jogo ou medir fps, e esta sessão estiver na nuvem.
---
# Pedido à sessão local

A nuvem não tem Unreal. O que precisa dele vira um arquivo `docs/pedidos/NNNN-assunto.md`, commitado no branch de
trabalho; o Felipe abre a sessão local, que lê, faz, responde no mesmo arquivo e dá push.

## Como escrever

- Numeração seguida (`ls docs/pedidos`), um assunto por pedido.
- Seções: **Contexto** (o que já existe, com caminhos), **O que fazer** (passos numerados, cada um verificável),
  **Fora deste pedido**, **Resposta da sessão local** (vazia).
- Diga o que não foi compilado na nuvem: a sessão local corrige o que o compilador do Unreal reclamar e conta no relatório.
- Peça a prova: saída do comando colada, captura salva em `docs/imagens/`, número medido. "Funcionou" não basta.
- Nada que dependa de login, compra ou aceite de termos sem dizer que é o Felipe quem faz.

## Como ler a resposta

`git fetch` e leia a seção de resposta. Confira as provas (rode o que dá para rodar aqui). Registre no
`docs/estado-atual.md` o que mudou e o próximo pedido.
