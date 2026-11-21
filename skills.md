# skills.md

## Objetivo
Projeto de aprendizado/portfólio inspirado no FIDE & Google Efficient Chess AI Challenge (Kaggle). Sucesso medido por métricas locais, não por ranking; o desafio original já encerrou.

## Arquitetura
- `src/`: código C (board, move, eval, search, main).
- `tests/`: testes automatizados (`tests/run_tests.py`).
- `scripts/`: geração de modelos (`generate_models.py`), torneio (`run_tournament.py`).
- `agent.py`: wrapper fino que recebe FEN e retorna lance UCI.

## Contrato do agente (Kaggle)
- Entrada: FEN via argumento para `agent.py` (ex.: `python3 agent.py "<FEN>" models/model0.weights`).
- Saída: lance em notação UCI (ex.: `e2e4`) em stdout.
- FEN inválida: código de saída != 0 e mensagem de erro, sem lance fictício.
- Posição sem lances legais: reporta erro (ou contingência legal quando existe).

## Padrão de projeto
Template Method na sequência de busca (IDDFS/alfa-beta/quiescência) com passos substituíveis (avaliação e ordenação). Strategy não foi usado como rótulo.

## Regras de recursos (premissa)
Limites confirmados apenas parcialmente; página oficial não foi lida. Premissa: binário <= 64 KB, RAM <= 5 MB, CPU 1 núcleo, tempo <= 10 s/lance.

## Base de dados
Endpoint usado: https://huggingface.co/datasets/linrock/bullet-training-data/resolve/main/S1/UHO.pdist.iter-1.bullet.bin.zst
Número de registros efetivamente lidos no ajuste: 2000 (32 bytes/registro, conforme script).
API inacessível: script aborta com mensagem clara.

## Algoritmos
- Motor: bitboards, geração de lances legais, make/unmake, perft.
- Busca: IDDFS + alfa-beta + quiescência de capturas + ordenação MVV-LVA + transposition table no contexto.
- Avaliação: material + PST + mobilidade (contagem por bitboards) + estrutura de peões.
- Pesos: arquivo binário little-endian com cabeçalho `version,count,type` (type 0=float32), validação de NaN/Inf.

## Tabela de afirmações verificadas
| Afirmação | Comando | Saída medida | Data |
|---|---|---|---|
| Compilação sem warnings | `make clean && make` | sucesso, sem warnings | 2026-11-09 |
| Testes | `make test` | `6/6 testes passaram` | 2026-11-09 |
| Perft posição inicial prof.5 | `./chess_engine --perft 5` | `4865609` | 2026-11-09 |
| Perft Kiwipete prof.4 | `./chess_engine --fen "r3k2r/..." --perft 4` | `4085603` | 2026-11-09 |
| Perft posição 3 prof.5 | com FEN padrão | `674624` | 2026-11-09 |
| Sanitizers | `make sanitize` + execução | sucesso | 2026-11-09 |
| Tamanho binário | `ls -l chess_engine` | 30640 bytes | 2026-11-09 |
| Pacote compactado | `zip` do projeto | 41 KB | 2026-11-09 |
| Pico de RAM | `/usr/bin/time -v` | 4096 KB | 2026-11-09 |
| Tempo por lance | `--time 1000` | ~0.7 s parede | 2026-11-09 |
| Geração de modelos | `python3 scripts/generate_models.py ...` | 4 modelos distintos | 2026-11-09 |

## Divergências entre meta e medido
- Pacote compactado 41 KB <= 64 KB (margem).
- Pico de RAM medido 4096 KB <= 5 MB.
- Perda de ajuste de pesos: não medida separadamente; script apenas registra endpoint e registros.
- Nós por segundo: não instrumentado em saída.

## Tentativas e dificuldades
- Dataset em formato binário compactado; consumido em streaming sem download completo.
- Implementação inicial de make/unmake precisou ser refeita.
- Torneio com juiz independente pendente de validação em larga escala (pode ser lento).
- Sem Stockfish/Valgrind no ambiente: etapas externas foram puladas com aviso.
- Cada modelo gerado difere por variação; diferença comportamental depende da execução do torneio.
PY
echo done