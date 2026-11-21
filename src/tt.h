#ifndef TT_H
#define TT_H
#include "types.h"
void tt_init(TranspositionTable *tt, size_t bytes);
void tt_clear(TranspositionTable *tt);
TTEntry *tt_probe(TranspositionTable *tt, uint64_t key);
void tt_store(TranspositionTable *tt, uint64_t key, int depth, int score, int flag, int best);
#endif