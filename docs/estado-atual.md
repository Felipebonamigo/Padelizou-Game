# Estado atual

Diário em ordem cronológica reversa: o mais novo em cima.

## 14/10/2026 — etapa 1 (física) feita na nuvem; o projeto Unreal espera o PC

**Feito**
- Repositório criado pelo Felipe; design aprovado (`docs/DESIGN.md`).
- `Source/PadelSim`: bola (gravidade, arrasto, Magnus, quique com spin, vidro, grade irregular por hash, rede, portas,
  saída por cima), quadra (painéis = física) e `golpes::Calcular` — porte do `Padel.Core`.
- Gabarito: 120 lançamentos (672 eventos, inclusive 7 saídas pela porta) e 90 golpes com os 9 efeitos, gravados pelo
  `Padel.Core` em `105ca75`. Resultado: **desvio zero** em posição, velocidade e spin; 115.573 verificações, 0 falhas,
  em gcc e clang, Release e Debug. Uma mutação de 0,01 na restituição do vidro gera 2.879 falhas (o teste enxerga).
- CI com gcc, clang e **MSVC** (o compilador do Unreal no Windows).
- Git LFS para o conteúdo binário (`.gitattributes`).

**Pendente**
- Pedido 0001 à sessão local: criar o projeto Unreal, ligar o módulo `PadelSim` e a Automation Spec do gabarito.
- Na nuvem, a seguir: jogador (corrida, balanço, contato no instante), árbitro e placar (ponto de ouro, set até 4,
  super tie-break) — mesmo método: gabarito do Core primeiro.
