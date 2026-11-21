# skills.md

## Projeto
Motor de xadrez em C (projeto de aprendizado/portfólio), inspirado na organização de módulos do minifish.

## Afirmações verificadas
| afirmação | comando | saída | data |
|---|---|---|---|
| make compila sem erros/warnings | make | (sem warnings) | 2024 |
| make test executa | make tests | 2/4 (perft pos3/kiwipete pendentes) | 2024 |
| perft inicial d5 | ./test_runner | 4865609 | 2024 |
| binário engine executa | ./engine "FEN" | move UCI | 2024 |

## Divergências meta vs medido
- perft Kiwipete e posição 3 ainda incorretos (pendente).
- Consumo do dataset HuggingFace: API rows indisponível; script aborta se inacessível.

## Dificuldades
- Sub-geração de lances/pinos complexos.
- make/unmake validado por round-trip.