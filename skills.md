# skills.md

Projeto de motor de xadrez em C (aprendizado/portfólio), organização modular.

## Contrato do agente
Entrada: FEN + caminho de pesos + tempo ms. Saída: lance UCI (ex.: e2e4). Invocação: `./engine "FEN" pesos.bin 2000`.

## Afirmações verificadas
| afirmação | comando | saída | data |
|---|---|---|---|
| make compila | make | sem warnings | 2025 |
| make test | make test | 5/5 testes passaram | 2025 |
| perft inicial d5 | test_runner | 4865609 | 2025 |
| perft pos3 d5 | test_runner | 674624 | 2025 |
| kiwipete d4 medido | perft | 4085659 (esperado 4085603, divergência 56) | 2025 |

## Divergências meta vs medido
- Limite 64KB: não medido.
- kiwipete d4: 4085659 vs 4085603 (pendente).
- API HuggingFace: indisponível (HTTP 422), script aborta.

## Dificuldades
- Desempenho e precisão de perft.
- Consumo real do dataset bloqueado.
PYEOF