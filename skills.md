# Skills - Beam8 Efficient Chess AI

## Projeto
Motor de xadrez em C (bitboards), busca alpha-beta, quiescencia, tabela de transposicao, avaliação material+PST+mobilidade. Wrappers Python e torneio.

## Limites medidos
- binario: 30344 bytes
- pacote compactado (zip do diretorio): 174405 bytes (ATENCAO: acima de 64KB se incluir binario/cache; medir apenas fontes+pesos+configs recomendado)
- profundidade perft inicial: 4865609 (depth5) OK
- testes: 7/7 passaram com e sem sanitizers

## Interface Kaggle
Wrapper Python `py/kaggle_agent.py` recebe FEN (argv ou stdin) e retorna lance UCI. Erros retornam contingencia e2e4 apenas quando necessario.

## Divergencias meta vs medido
- Limite 64KB nao confirmado por medicao do pacote completo de submissao; zip atual inclui binario. Aconfirmar removendo artefatos.
- RAM e tempo por lance nao medidos automaticamente ainda.
- Perft Kiwipete depth4 (4.085.603) nao testado nesta iteracao.
- Torneio nao calcula movimentos sub-otimos com juiz independente.
- Tuning da base de dados e aproximado: consumo da API registrado (sample_summary), mas parsing do formato binario nao implementado; pesos base sao referencias + variacao.

## Dificuldades
- Dataset no formato .zst compactado e binario desconhecido; API do HF acessada por streaming parcial.
- make/unmake corrigidos apos erros de promocao.
- Variacao aleatoria rotulada como variacao, nao aprendizado.
EOF