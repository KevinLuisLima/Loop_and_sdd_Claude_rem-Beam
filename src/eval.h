#ifndef EVAL_H
#define EVAL_H
#include "types.h"
#include "board.h"
#define NUM_WEIGHTS 389
typedef struct { float w[NUM_WEIGHTS]; } Weights;
int  weights_load(const char *path, Weights *out, char *errbuf, size_t errsz);
int  eval_score(const Board *b, const Weights *w);
int  eval_random_weights(Weights *w, unsigned seed);
#endif
