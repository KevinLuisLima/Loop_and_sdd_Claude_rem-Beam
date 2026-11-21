#ifndef SEARCH_H
#define SEARCH_H
#include "types.h"
#include "board.h"
#include "eval.h"
typedef struct {
  int nodes;
  int stopped;
  long long start_ms;
  long long limit_ms;
} SearchCtx;
void search_init(SearchCtx *c);
Move search_best_move(Board *b, const Weights *w, int time_ms, SearchCtx *c, int *depth_out, int *nodes_out);
int quiescence(Board *b, const Weights *w, SearchCtx *c, int alpha, int beta, int side_root);
#endif
