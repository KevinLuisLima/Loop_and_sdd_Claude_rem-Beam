#ifndef SEARCH_H
#define SEARCH_H
#include "position.h"
#include "types.h"
#include "eval.h"

typedef struct {
    u64 key;
    int depth;
    int score;
    int flag; /* 0 exact,1 lower,2 upper */
    Move best;
    int used;
} TTEntry;

typedef struct {
    int max_time_ms;
    long start_ms;
    int stop;
    int nodes;
    int depth_reached;
    TTEntry *tt;
    int tt_size;
    Weights *weights;
} SearchContext;

typedef struct {
    int best_move_found;
    Move best_move;
    int score;
} SearchResult;

long now_ms(void);
void search_init_context(SearchContext *ctx,int time_ms, Weights *w);
Move search_best_move(Position *p, SearchContext *ctx, int max_depth);

#endif