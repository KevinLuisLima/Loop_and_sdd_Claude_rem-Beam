#ifndef SEARCH_H
#define SEARCH_H
#include "board.h"
#include "move.h"
#include "eval.h"

typedef struct {
    /* timing */
    long start_ms;
    long budget_ms;
    int stop;
    /* nodes */
    unsigned long nodes;
    int depth_reached;
    /* transposition */
    int tt_size;
    void *tt;
    /* weights */
    Weights *weights;
    /* result */
    Move best_move;
    int best_score;
} SearchContext;

void search_init(SearchContext*ctx, Weights*w, int time_ms);
void search_free(SearchContext*ctx);
Move search_best_move(Board*b, SearchContext*ctx, int*score_out);

#endif