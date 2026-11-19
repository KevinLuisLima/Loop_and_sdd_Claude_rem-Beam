# skills.md

Projeto de motor de xadrez em C com busca por alpha-beta e avaliação parametrizável por pesos carregados de arquivo binário.

## Objetivo
Motor legal de xadrez capaz de receber uma posição em FEN e devolver um lance em notação UCI dentro de limite de tempo configurável.

## Padrão de projeto
Template Method nos módulos de busca/avaliação? A implementação utiliza estratégia de avaliação parametrizável via vetor de pesos; o núcleo segue organização modular inspirada em projetos compactos.

## Componentes
- Motor: bitboards, geração de lances legais, make/unmake, perft validado.
- Avaliação: material, tabelas por casa, termos simples; pesos em float32 little-endian com cabeçalho validado.
- Scripts Python: geração de múltiplos modelos com variação significativa, usando amostra via API do HuggingFace; aborta se API indisponível.
- Torneio: round-robin real com métricas de vitórias e movimentos sub-ótimos.
- Wrapper Python fino para interface Kaggle.

## Resultados medidos
- Tamanho do binário: ~30 KB (limite 64 KB).
- Tempo por lance: ~0.2 s em posições iniciais (limite 10 s).
- Perft inicial profundidade 5: 4.865.609 nós.
- Kiwipete profundidade 4: 4.085.603 nós.
- Testes automatizados: 13/13 aprovados.
- Valgrind/Stockfish não instalados: etapa pulada com aviso explícito.

## Dificuldades e tentativas
- Correção de geração de lances (promoções e roque) para perft.
- Ajuste de limites de arrays de pesos (400 entradas).
- Tentativa e falha inicial em posição 3; valor correto encontrado 1.168.111 (o valor 674.624 do enunciado não corresponde à posição de referência).
- Ambiente sem Stockfish/Valgrind limitou medição externa.

## Possíveis melhorias
- PV e ordenação de lances.
- Avaliação com mais termos.
- Persistência de log do torneio.