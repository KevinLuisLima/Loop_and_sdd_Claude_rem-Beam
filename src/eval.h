#ifndef EVAL_H
#define EVAL_H
#include "chess.h"
int eval_position(const Position *p, const float *weights);
void eval_features(const Position *p, float *features);
#endif