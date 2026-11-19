# skills.md

Projeto: motor de xadrez em C para ambiente Kaggle (estilo FIDE & Google Efficient Chess AI Challenge).

## Módulos

- `src/chess.c`: representação do tabuleiro, bitboards, geração de lances, make/unmake, perft.
- `src/eval.c`: avaliação com tabelas piece-square e termos simples.
- `src/weights.c`: leitura/validação do vetor de pesos binário (little-endian, float32, cabeçalho versão+quantidade).
- `src/main.c`: interface de agente: recebe FEN e imprime lance UCI.

## CLI

- `./engine "FEN"` -> lance UCI.
- `./engine --applymove "FEN" "e2e4"` -> nova FEN.
- `python3 agent.py "FEN"`.

## Pesos

- `scripts/generate_weights.py --models N --variation f --seed s --out dir`.
- `scripts/fetch_dataset.py`: consome API do HuggingFace; aborta se inacessível.

## Build

- `make`, `make test`, CMake disponível em CMakeLists.txt.

## Métricas medidas

- Perft inicial prof.5 = 4865609.
- Kiwipete e posição 3 ainda divergem dos valores de referência (pendente).
- Execução sob recursos a medir.

## Limitações

- Busca alpha-beta básica; torneio funcional mínimo.
- Sem Stockfish/Valgrind no ambiente.