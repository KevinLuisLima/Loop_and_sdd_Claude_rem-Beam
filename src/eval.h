#ifndef EVAL_H
#define EVAL_H
#include "position.h"
/* weights loaded from binary file */
typedef struct { int n; float *w; } Weights;
int weights_load(const char *path, Weights *out);
void weights_free(Weights *w);
int evaluate(Position *p, Weights *w); /* relative to side to move */
#endif