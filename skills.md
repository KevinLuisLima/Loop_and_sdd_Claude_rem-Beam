# skills.md — Beam7 Efficient Chess AI Challenge (portfolio/learning)

## Project
Learning/portfolio chess engine based on linrock/minifish organization, FIDE & Google Efficient Chess AI Challenge.

## Verified assertions (command → measured output → date)

| Afirmação | Comando | Saída medida | Data |
|---|---|---|---|
| make compila sem erros | make | sucesso (sem warnings) | 2025-11-09 |
| make test passa todos | make test | 3/3 testes passaram | 2025-11-09 |
| CMake funciona | cmake/make | Built target engine | 2025-11-09 |
| Pico RAM do motor | /usr/bin/time -v | 4232 kB (≤5MB) | 2025-11-09 |
| Tamanho binário | ls -l engine | 35128 bytes | 2025-11-09 |
| Profundidade/nós | teste perft | inicial prof5: 4865609; Kiwipete prof4: 4085603 | 2025-11-09 |
| Lances | engine invocado | b1c3 na posição inicial | 2025-11-09 |

## Divergências meta vs medido
- Limite de pacote ≤64 KB: motor + um modelo ~37 KB não comprimido; conjunto completo do repositório 116 KB compactado (inclui todos os modelos e código-fonte) excede, mas o pacote do agente submetido pode usar um único modelo.
- Limite de RAM medido 4.23 MB ≤5 MB: OK, mas próximo. Base de dados não foi parseada em posições rotuladas; apenas 65536 bytes brutos lidos.

## Padrão de projeto
Template Method não foi adotado como esqueleto verificável; o projeto usa Strategy (ponteiros/structs de contexto) e avaliação por vetor de pesos. Sem variáveis globais mutáveis.

## Componentes
- src/: bitboards, position, movegen, eval, search, main (engine)
- engine: lê FEN como argumento, carrega pesos binários, imprime lance UCI.
- python/gen_weights.py: CLI para gerar modelos variados via estrutura CHW1 little-endian.
- tests/test_all.c: perft e carga de pesos.
- tournament/: não implementado completamente (pendente).

## Limitações / não verificado
- Torneio round-robin e minimização de movimentos sub-ótimos: não implementado.
- Dataset: apenas 65536 bytes lidos, 0 posições rotuladas parseadas; pesos não treinados por Texel.
- Valgrind/Stockfish não instalados: etapas dependentes puladas.
- Kaggle: contrato não validado contra simulador oficial.

## Dificuldades encontradas
- Compilação warning-free exigiu remoção de funções não usadas.
- Movegen complexo e promover índices exigiu padronização dos tipos de peça.
- Sanitizer detectou uso de ponteiro nulo em ordenação; corrigido.