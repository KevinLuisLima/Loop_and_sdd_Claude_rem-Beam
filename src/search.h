#ifndef SEARCH_H
#define SEARCH_H
#include "types.h"
Move search_bestmove(SearchContext *ctx, int depth_limit, double time_limit);
void search_init_context(SearchContext *ctx, Position *pos, TranspositionTable *tt, Weights *w, double time_limit);
#endif