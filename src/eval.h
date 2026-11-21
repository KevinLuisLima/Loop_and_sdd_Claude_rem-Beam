#ifndef EVAL_H
#define EVAL_H
#include "board.h"

typedef struct {
    int n;
    float *w;
} Weights;

int eval_load(Weights*w,const char*path);
void eval_free(Weights*w);
int evaluate(const Board*b,const Weights*w);

#endif